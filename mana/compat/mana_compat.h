/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * MANA out-of-tree compat umbrella.
 *
 * Force-included at the top of every TU via `ccflags-y += -include ...` in
 * Kbuild. The driver body under ../driver/ is a verbatim upstream snapshot;
 * every kernel-version adaptation lives here or in the sibling compat/
 * headers, added by later commits just before the driver commit that needs
 * them.
 */
#ifndef MANA_COMPAT_H
#define MANA_COMPAT_H

#include "autogen/mana_compat_autoconf.h"
#include "mana_compat_features.h"

/* Symbol export suppression: the driver exports symbols with
 * EXPORT_SYMBOL_NS(sym, "NET_MANA") whose meaning changes across v6.13. */
#include "mana_compat_export.h"

/* page_pool overlay: the anchor driver commit uses page_pool_create() etc.
 * without an explicit #include, relying on transitive inclusion via
 * <net/xdp.h> that only holds on kernels < v6.6 (before the header split).
 * Force-including <net/page_pool/helpers.h> here resolves the symbols on
 * every kernel; the backport-include overlay handles the split. */
#include <net/page_pool/helpers.h>

/* netdev back-compat: netif_napi_add_weight / netif_napi_add_tx bridges and
 * v6.15 <net/netdev_lock.h> forward-include. */
#include "mana_compat_netdev.h"

/* Core kernel back-compat: hop_jumbo helpers (v5.19) and
 * dma_set_max_seg_size return-type (v6.13). */
#include "mana_compat_kernel.h"

/* XDP back-compat: xdp_set_features_flag() + NETDEV_XDP_ACT_* on kernels
 * < v6.3. */
#include "mana_compat_xdp.h"

/* ethtool wrapper TU + register_netdev interceptor: adapts the driver's
 * ops-table to kernel-version-varying signatures. Renames register_netdev
 * to our interceptor. */
#include "mana_compat_ethtool_ops.h"

/* cleanup + cpumask back-compat (needed as later driver commits adopt
 * __free() attribute-based cleanup at v6.4 and cpumask_weight_andnot at
 * v6.3). */
#include "mana_compat_cleanup.h"
#include "mana_compat_cpumask.h"

/* Force-include the ethtool interceptor rename BEFORE the driver body's
 * register_netdev call resolves. */
#define register_netdev mana_compat_register_netdev

/* Additional shim headers appended here by future commits, each just before
 * the driver commit that consumes it. */

#endif /* MANA_COMPAT_H */
