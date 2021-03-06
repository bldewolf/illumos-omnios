#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
# Copyright 2017 Joyent, Inc.
#
#	This makefile drives the production of the kernel component of
#	the lx brand
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE		= ../..

#
#	Path to where brand common sources live
#
LX_CMN		= $(SRC)/common/brand/lx

#
#	Define the module and object file sets.
#
MODULE		= lx_brand
OBJECTS		= $(LX_BRAND_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(LX_BRAND_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(USR_BRAND_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/intel/Makefile.intel

#
#	Define targets
#
ALL_TARGET	= $(BINARY)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

INC_PATH	+= -I$(UTSBASE)/common/brand/lx -I$(LX_CMN) -I$(SRC)/common
INC_PATH	+= -I$(UTSBASE)/common/inet/sockmods -I$(UTSBASE)/common/io/bpf
INC_PATH	+= -I$(UTSBASE)/common/fs/sockfs
INC_PATH	+= -I$(UTSBASE)/common/fs/zfs
AS_INC_PATH	+= -I$(UTSBASE)/i86pc/genassym/$(OBJS_DIR)

#
# lint pass one enforcement
#
CFLAGS		+= $(CCVERBOSE)

LDFLAGS		+= -dy -Nexec/elfexec -Nfs/fifofs -Nfs/sockfs -Ndrv/ip \
		    -Nfs/zfs -Nmisc/klmmod

#
# For now, disable these lint checks; maintainers should endeavor
# to investigate and remove these for maximum lint coverage.
# Please do not carry these forward to new Makefiles.
#
LINTTAGS	+= -erroff=E_ASSIGN_NARROW_CONV
LINTTAGS	+= -erroff=E_SUSPICIOUS_COMPARISON
# Due to zfs_ioctl.h inlines.  gcc catches non-inline ones during compilation.
LINTTAGS	+= -erroff=E_STATIC_UNUSED

#
#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

#
#	Include common targets.
#
include $(UTSBASE)/intel/Makefile.targ

#
#	Include brand-specific rules
#

include $(UTSBASE)/intel/lx_brand/Makefile.rules
