/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * Copyright 2016 Joyent, Inc.
 */


#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/frame.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>

#include <sys/lx_brand.h>
#include <sys/lx_impl.h>

/*
 * We store the syscall number in the low 16 bits (which limits us to 64k
 * syscalls). The next bit indicates entry/return probe and the next bit
 * indicates 64bit/32bit syscall.
 */
#define	SCALL_MASK	0xffff
#define	ENTRY_FLAG	0x10000
#define	SYSC_64_BIT	0x100000

#define	LX_SYSTRACE_IS64BIT(x)	((int)(x) & SYSC_64_BIT)
#define	LX_SYSTRACE_ISENTRY(x)	((int)(x) & ENTRY_FLAG)
#define	LX_SYSTRACE_SYSNUM(x)	((int)(x) & SCALL_MASK)

#define	LX_SYSTRACE32_ENTRY(id)	(ENTRY_FLAG | (id))
#define	LX_SYSTRACE32_RETURN(id) (id)

#define	LX_SYSTRACE64_ENTRY(id)	(SYSC_64_BIT | ENTRY_FLAG | (id))
#define	LX_SYSTRACE64_RETURN(id) (SYSC_64_BIT | id)

#define	LX_SYSTRACE_ENTRY_AFRAMES	2
#define	LX_SYSTRACE_RETURN_AFRAMES	4

typedef struct lx_systrace_sysent {
	const char *lss_name;
	dtrace_id_t lss_entry;
	dtrace_id_t lss_return;
} lx_systrace_sysent_t;

static dev_info_t *lx_systrace_devi;
static dtrace_provider_id_t lx_systrace_id;
static kmutex_t lx_systrace_lock;
static uint_t lx_systrace_nenabled;

static int lx_systrace_nsysent32;
static lx_systrace_sysent_t *lx_systrace_sysent32;

#if defined(_LP64)
static int lx_systrace_nsysent64;
static lx_systrace_sysent_t *lx_systrace_sysent64;
#endif

/*ARGSUSED*/
static void
lx_systrace_entry(ulong_t sysnum, ulong_t arg0, ulong_t arg1, ulong_t arg2,
    ulong_t arg3, ulong_t arg4, ulong_t arg5)
{
	dtrace_id_t id;

#if defined(_LP64)
	if ((ttoproc(curthread))->p_model == DATAMODEL_NATIVE) {
		if (sysnum >= lx_systrace_nsysent64)
			return;
		id = lx_systrace_sysent64[sysnum].lss_entry;
	} else
#endif
	{
		if (sysnum >= lx_systrace_nsysent32)
			return;
		id = lx_systrace_sysent32[sysnum].lss_entry;
	}

	if (id == DTRACE_IDNONE)
		return;
	dtrace_probe(id, arg0, arg1, arg2, arg3, arg4);
}

/*ARGSUSED*/
static void
lx_systrace_return(ulong_t sysnum, ulong_t arg0, ulong_t arg1, ulong_t arg2,
    ulong_t arg3, ulong_t arg4, ulong_t arg5)
{
	dtrace_id_t id;

#if defined(_LP64)
	if ((ttoproc(curthread))->p_model == DATAMODEL_NATIVE) {
		if (sysnum >= lx_systrace_nsysent64)
			return;
		id = lx_systrace_sysent64[sysnum].lss_return;
	} else
#endif
	{
		if (sysnum >= lx_systrace_nsysent32)
			return;
		id = lx_systrace_sysent32[sysnum].lss_return;
	}

	if (id == DTRACE_IDNONE)
		return;
	dtrace_probe(id, arg0, arg1, arg2, arg3, arg4);
}

/*ARGSUSED*/
static void
lx_systrace_provide(void *arg, const dtrace_probedesc_t *desc)
{
	int i;

	if (desc != NULL)
		return;

	for (i = 0; i < lx_systrace_nsysent32; i++) {
		if (dtrace_probe_lookup(lx_systrace_id, "sys32",
		    lx_systrace_sysent32[i].lss_name, "entry") != 0)
			continue;

		(void) dtrace_probe_create(lx_systrace_id, "sys32",
		    lx_systrace_sysent32[i].lss_name, "entry",
		    LX_SYSTRACE_ENTRY_AFRAMES,
		    (void *)((uintptr_t)LX_SYSTRACE32_ENTRY(i)));

		(void) dtrace_probe_create(lx_systrace_id, "sys32",
		    lx_systrace_sysent32[i].lss_name, "return",
		    LX_SYSTRACE_RETURN_AFRAMES,
		    (void *)((uintptr_t)LX_SYSTRACE32_RETURN(i)));

		lx_systrace_sysent32[i].lss_entry = DTRACE_IDNONE;
		lx_systrace_sysent32[i].lss_return = DTRACE_IDNONE;
	}

#if defined(_LP64)
	for (i = 0; i < lx_systrace_nsysent64; i++) {
		if (dtrace_probe_lookup(lx_systrace_id, "sys64",
		    lx_systrace_sysent64[i].lss_name, "entry") != 0)
			continue;

		(void) dtrace_probe_create(lx_systrace_id, "sys64",
		    lx_systrace_sysent64[i].lss_name, "entry",
		    LX_SYSTRACE_ENTRY_AFRAMES,
		    (void *)((uintptr_t)LX_SYSTRACE64_ENTRY(i)));

		(void) dtrace_probe_create(lx_systrace_id, "sys64",
		    lx_systrace_sysent64[i].lss_name, "return",
		    LX_SYSTRACE_RETURN_AFRAMES,
		    (void *)((uintptr_t)LX_SYSTRACE64_RETURN(i)));

		lx_systrace_sysent64[i].lss_entry = DTRACE_IDNONE;
		lx_systrace_sysent64[i].lss_return = DTRACE_IDNONE;
	}
#endif
}

/*ARGSUSED*/
static int
lx_systrace_enable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = LX_SYSTRACE_SYSNUM((uintptr_t)parg);

	mutex_enter(&lx_systrace_lock);
	if (lx_systrace_nenabled++ == 0)
		lx_brand_systrace_enable();
	mutex_exit(&lx_systrace_lock);

#if defined(_LP64)
	if (LX_SYSTRACE_IS64BIT((uintptr_t)parg)) {
		ASSERT(sysnum < lx_systrace_nsysent64);

		if (LX_SYSTRACE_ISENTRY((uintptr_t)parg)) {
			lx_systrace_sysent64[sysnum].lss_entry = id;
		} else {
			lx_systrace_sysent64[sysnum].lss_return = id;
		}
	} else
#endif
	{
		ASSERT(sysnum < lx_systrace_nsysent32);

		if (LX_SYSTRACE_ISENTRY((uintptr_t)parg)) {
			lx_systrace_sysent32[sysnum].lss_entry = id;
		} else {
			lx_systrace_sysent32[sysnum].lss_return = id;
		}
	}
	return (0);
}

/*ARGSUSED*/
static void
lx_systrace_disable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = LX_SYSTRACE_SYSNUM((uintptr_t)parg);

#if defined(_LP64)
	if (LX_SYSTRACE_IS64BIT((uintptr_t)parg)) {
		ASSERT(sysnum < lx_systrace_nsysent64);

		if (LX_SYSTRACE_ISENTRY((uintptr_t)parg)) {
			lx_systrace_sysent64[sysnum].lss_entry = DTRACE_IDNONE;
		} else {
			lx_systrace_sysent64[sysnum].lss_return = DTRACE_IDNONE;
		}
	} else
#endif
	{
		ASSERT(sysnum < lx_systrace_nsysent32);

		if (LX_SYSTRACE_ISENTRY((uintptr_t)parg)) {
			lx_systrace_sysent32[sysnum].lss_entry = DTRACE_IDNONE;
		} else {
			lx_systrace_sysent32[sysnum].lss_return = DTRACE_IDNONE;
		}
	}

	mutex_enter(&lx_systrace_lock);
	if (--lx_systrace_nenabled == 0)
		lx_brand_systrace_disable();
	mutex_exit(&lx_systrace_lock);
}

/*ARGSUSED*/
static void
lx_systrace_destroy(void *arg, dtrace_id_t id, void *parg)
{
}

/*ARGSUSED*/
static uint64_t
lx_systrace_getarg(void *arg, dtrace_id_t id, void *parg, int argno,
    int aframes)
{
	struct frame *fp = (struct frame *)dtrace_getfp();
	uintptr_t *stack;
	uint64_t val = 0;
	int i;

	if (argno >= 6)
		return (0);

	/*
	 * Walk the four frames down the stack to the entry or return callback.
	 * Our callback calls dtrace_probe() which calls dtrace_dif_variable()
	 * which invokes this function to get the extended arguments. We get
	 * the frame pointer in via call to dtrace_getfp() above which makes for
	 * four frames.
	 */
	for (i = 0; i < 4; i++) {
		fp = (struct frame *)fp->fr_savfp;
	}

	stack = (uintptr_t *)&fp[1];

	/*
	 * Skip the first argument to the callback -- the system call number.
	 */
	argno++;

#ifdef __amd64
	/*
	 * On amd64, the first 6 arguments are passed in registers while
	 * subsequent arguments are on the stack.
	 */
	argno -= 6;
#endif

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	val = stack[argno];
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);

	return (val);
}


static const dtrace_pattr_t lx_systrace_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t lx_systrace_pops = {
	lx_systrace_provide,
	NULL,
	lx_systrace_enable,
	lx_systrace_disable,
	NULL,
	NULL,
	NULL,
	lx_systrace_getarg,
	NULL,
	lx_systrace_destroy
};

static int
lx_systrace_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	int i;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	if (ddi_create_minor_node(devi, "lx_systrace", S_IFCHR,
	    0, DDI_PSEUDO, NULL) == DDI_FAILURE ||
	    dtrace_register("lx-syscall", &lx_systrace_attr,
	    DTRACE_PRIV_USER, 0, &lx_systrace_pops, NULL,
	    &lx_systrace_id) != 0) {
		ddi_remove_minor_node(devi, NULL);
		return (DDI_FAILURE);
	}

	ddi_report_dev(devi);
	lx_systrace_devi = devi;

	/*
	 * Initialize the 32-bit table.
	 */
	VERIFY(lx_nsysent32 > 0);
	lx_systrace_nsysent32 = lx_nsysent32;
	lx_systrace_sysent32 = kmem_zalloc(lx_systrace_nsysent32 *
	    sizeof (lx_systrace_sysent_t), KM_SLEEP);

	for (i = 0; i < lx_systrace_nsysent32; i++) {
		lx_systrace_sysent32[i].lss_name = lx_sysent32[i].sy_name;
		lx_systrace_sysent32[i].lss_entry = DTRACE_IDNONE;
		lx_systrace_sysent32[i].lss_return = DTRACE_IDNONE;
	}

#if defined(_LP64)
	/*
	 * Initialize the 64-bit table.
	 */
	VERIFY(lx_nsysent64 > 0);
	lx_systrace_nsysent64 = lx_nsysent64;
	lx_systrace_sysent64 = kmem_zalloc(lx_systrace_nsysent64 *
	    sizeof (lx_systrace_sysent_t), KM_SLEEP);

	for (i = 0; i < lx_systrace_nsysent64; i++) {
		lx_systrace_sysent64[i].lss_name = lx_sysent64[i].sy_name;
		lx_systrace_sysent64[i].lss_entry = DTRACE_IDNONE;
		lx_systrace_sysent64[i].lss_return = DTRACE_IDNONE;
	}
#endif

	/*
	 * Install probe triggers.
	 */
	lx_systrace_entry_ptr = lx_systrace_entry;
	lx_systrace_return_ptr = lx_systrace_return;

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
lx_systrace_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	if (dtrace_unregister(lx_systrace_id) != 0)
		return (DDI_FAILURE);

	/*
	 * Free tables.
	 */
	kmem_free(lx_systrace_sysent32, lx_systrace_nsysent32 *
	    sizeof (lx_systrace_sysent_t));
	lx_systrace_sysent32 = NULL;
	lx_systrace_nsysent32 = 0;

#if defined(_LP64)
	kmem_free(lx_systrace_sysent64, lx_systrace_nsysent64 *
	    sizeof (lx_systrace_sysent_t));
	lx_systrace_sysent64 = NULL;
	lx_systrace_nsysent64 = 0;
#endif

	/*
	 * Reset probe triggers.
	 */
	lx_systrace_entry_ptr = NULL;
	lx_systrace_return_ptr = NULL;

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
lx_systrace_open(dev_t *devp, int flag, int otyp, cred_t *cred_p)
{
	return (0);
}

static struct cb_ops lx_systrace_cb_ops = {
	lx_systrace_open,	/* open */
	nodev,			/* close */
	nulldev,		/* strategy */
	nulldev,		/* print */
	nodev,			/* dump */
	nodev,			/* read */
	nodev,			/* write */
	nodev,			/* ioctl */
	nodev,			/* devmap */
	nodev,			/* mmap */
	nodev,			/* segmap */
	nochpoll,		/* poll */
	ddi_prop_op,		/* cb_prop_op */
	0,			/* streamtab */
	D_NEW | D_MP		/* Driver compatibility flag */
};

static struct dev_ops lx_systrace_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* refcnt */
	ddi_getinfo_1to1,	/* get_dev_info */
	nulldev,		/* identify */
	nulldev,		/* probe */
	lx_systrace_attach,	/* attach */
	lx_systrace_detach,	/* detach */
	nodev,			/* reset */
	&lx_systrace_cb_ops,	/* driver operations */
	NULL,			/* bus operations */
	nodev,			/* dev power */
	ddi_quiesce_not_needed,		/* quiesce */
};

/*
 * Module linkage information for the kernel.
 */
static struct modldrv modldrv = {
	&mod_driverops,		/* module type (this is a pseudo driver) */
	"Linux Brand System Call Tracing", /* name of module */
	&lx_systrace_ops	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void *)&modldrv,
	NULL
};

int
_init(void)
{
	return (mod_install(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}
