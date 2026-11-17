/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_H
#define MANA_IB_COMPAT_H

#include "autogen/mana_ib_compat_autoconf.h"
#ifndef MANA_IB_COMPAT_GENERATED
#error Run the checked MANA IB Makefile before building
#endif
#include "mana_ib_compat_exports.h"
#include "mana_compat.h"
#include <linux/module.h>

/* Paired MANA exports use private names, not the in-tree NET_MANA namespace. */
#undef MODULE_IMPORT_NS
#define MODULE_IMPORT_NS(ns)

#endif
