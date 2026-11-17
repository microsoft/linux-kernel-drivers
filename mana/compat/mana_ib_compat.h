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
#include <rdma/ib_verbs.h>
#if MANA_IB_HAVE_ITER_HEADER
#include <rdma/iter.h>
#endif
#if !MANA_IB_HAVE_DRIVER_ID
#define RDMA_DRIVER_MANA ((enum rdma_driver_id)20)
#endif

#include "mana_ib_compat_legacy.h"
#include "mana_ib_compat_verbs.h"
#include "mana_ib_compat_umem.h"

/* Paired MANA exports use private names, not the in-tree NET_MANA namespace. */
#undef MODULE_IMPORT_NS
#define MODULE_IMPORT_NS(ns)

#endif
