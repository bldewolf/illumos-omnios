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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

#
# This makefile drives the production of iSCSI initiator Driver
# sparc architecture dependent
#

#
#	Paths to the base of the uts directory trees
#
UTSBASE   = ../..

#
# Define the module and object file sets.
#
MODULE		= iscsi
OBJECTS		= $(ISCSI_INITIATOR_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(ISCSI_INITIATOR_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/io/scsi/adapters/iscsi

#
# Include common rules.
#
include $(UTSBASE)/sparc/Makefile.sparc

#
# Define targets.
#
ALL_TARGET	= $(BINARY) $(SRC_CONFILE)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE) $(ROOT_CONFFILE)

# includes
INC_PATH	+= -I$(UTSBASE)/common/io/scsi/adapters/iscsi
INC_PATH        += -I$(SRC)/common/hdcrc

#
# Note dependancy on misc/scsi.
#
LDFLAGS += -dy -N"misc/scsi" -N"fs/sockfs" -N"sys/doorfs" -N"misc/md5" -Nmisc/ksocket
LDFLAGS += -N"misc/idm"

LINTFLAGS += -a -erroff=E_BAD_PTR_CAST_ALIGN -erroff=E_PTRDIFF_OVERFLOW
LINTFLAGS64 += -a -erroff=E_BAD_PTR_CAST_ALIGN -erroff=E_PTRDIFF_OVERFLOW

CERRWARN	+= -_gcc=-Wno-switch
CERRWARN	+= -_gcc=-Wno-unused-function
CERRWARN	+= -_gcc=-Wno-parentheses
CERRWARN	+= -_gcc=-Wno-type-limits
CERRWARN	+= -_gcc=-Wno-uninitialized

#
# Default build targets.
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
# Include common targets.
#
include $(UTSBASE)/sparc/Makefile.targ
