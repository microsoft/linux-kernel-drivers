/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Out-of-tree ethtool ops-table augmentation (declarations).
 *
 * The verbatim body registers its ethtool_ops via
 *
 *     ndev->ethtool_ops = &mana_ethtool_ops;
 *     ...
 *     register_netdev(ndev);
 *
 * upstream mana_ethtool_ops does NOT carry:
 *   1. .get_priv_flags / .set_priv_flags -- the out-of-tree private-flags plane
 *      (rx_cqe_coalesce_4) that gives < v7.1 kernels a portable door to the
 *      device state the native rx-cqe uAPI reaches only on v7.1+.
 *   2. ETH_SS_PRIV_FLAGS handling in .get_sset_count / .get_strings.
 *   3. A .get_rxnfc(ETHTOOL_GRXRINGS) legacy ring-count fallback for kernels
 *      before v7.0 gained ethtool_ops.get_rx_ring_count.
 *
 * The old design added all of these to mana_ethtool_ops via a marked hunk.
 * That's out now: this header interposes register_netdev() so we can:
 *
 *   * memcpy the driver's real ops-table into on-stack staging,
 *   * inject our additive ops-table entries into the staging (priv-flags,
 *     ETH_SS_PRIV_FLAGS-aware sset/strings wrappers, legacy get_rxnfc,
 *     OLD-sig adapters where applicable),
 *   * publish staging to a shared static shadow (single memcpy),
 *   * point ndev->ethtool_ops at the shadow,
 *   * hand off to the real register_netdev().
 *
 * The sset/strings wrappers and the OLD-sig ethtool adapters live in
 * mana_compat_ethtool_wrapper.c (in the same TU as the included driver
 * body), so they can call the driver's file-static mana_get_sset_count()
 * / mana_get_strings() / mana_get_rxfh() / mana_get_ringparam() / etc.
 * directly by name. No cached function pointers, no runtime cast of
 * ndev->ethtool_ops back to the shim type -- the interceptor here just
 * installs the extern-visible symbols at the kernel-visible slots.
 *
 * The shadow is a single per-module static -- MANA's ports all share the same
 * ops-table upstream, so one shadow serves every port, refreshed on every
 * probe with byte-identical content. The staging-then-publish pattern means
 * no locks and no ready-flag are needed even with concurrent multi-device
 * probing: see the concurrency-model comment in mana_compat_ethtool_ops.c.
 * This keeps driver/mana_ethtool.c much closer to verbatim: the priv-flags
 * wiring, the sset/strings wrappers, and the get_rxnfc fallback body all
 * leave the driver body entirely.
 *
 * Only the DECLARATION and the register_netdev override live here so this
 * header is safe to force-include from the umbrella. Bodies live in
 * mana_compat_ethtool_ops.c, which #undefs the macro before defining the
 * wrapper so the wrapper itself can call the kernel's real register_netdev().
 */
#ifndef MANA_COMPAT_ETHTOOL_OPS_H
#define MANA_COMPAT_ETHTOOL_OPS_H

#include <linux/types.h>

struct net_device;
struct ethtool_ringparam;

int mana_compat_register_netdev(struct net_device *ndev);

#define register_netdev mana_compat_register_netdev

/*
 * ETH_SS_PRIV_FLAGS delegates. Defined in mana_compat_ethtool_wrapper.c so
 * they can call the driver's file-static mana_get_sset_count() and
 * mana_get_strings() directly for non-PRIV_FLAGS string sets. Signatures
 * match struct ethtool_ops.{get_sset_count,get_strings}.
 */
int  mana_compat_get_sset_count(struct net_device *dev, int stringset);
void mana_compat_get_strings(struct net_device *dev, u32 stringset, u8 *data);

/*
 * OLD-sig ethtool adapters. Defined in mana_compat_ethtool_wrapper.c (after
 * the driver body is included, so they can call the driver's file-static
 * mana_get_rxfh() / mana_set_rxfh() / mana_get_ringparam() / mana_set_ringparam()
 * directly by name). Declared here (extern) so the register_netdev interceptor
 * in mana_compat_ethtool_ops.c can install them at the kernel-visible
 * ops-table slots.
 *
 * The signatures below match the kernel's OLD field types (pre-v6.8 for
 * rxfh, pre-v6.0 for ringparam); they are what the kernel dereferences with
 * the OLD calling convention. Inside each adapter, the extra NEW-sig
 * parameters are synthesised (NULL for kernel_ethtool_ringparam / extack,
 * on-stack struct for ethtool_rxfh_param) and the driver's NEW-sig function
 * is called with them.
 */
#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
int  mana_compat_get_rxfh_old(struct net_device *dev, u32 *indir, u8 *key,
			      u8 *hfunc);
int  mana_compat_set_rxfh_old(struct net_device *dev, const u32 *indir,
			      const u8 *key, const u8 hfunc);
#endif

#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
void mana_compat_get_ringparam_old(struct net_device *dev,
				   struct ethtool_ringparam *ring);
int  mana_compat_set_ringparam_old(struct net_device *dev,
				   struct ethtool_ringparam *ring);
#endif

/*
 * Coalesce bridges. Defined in mana_compat_ethtool_wrapper.c (where the
 * driver body's static mana_get_coalesce / mana_set_coalesce are visible
 * AND kernel_ethtool_coalesce has been renamed to the shim, so a stack-
 * local shim can be built and fed to them). Called from OLD-sig adapters
 * in mana_compat_ethtool_ops.c that receive the REAL
 * kernel_ethtool_coalesce from the kernel core (which the driver's
 * shim-typed handlers must never touch).
 *
 * User path on <v7.1:
 *   ethtool -c <if>              -> mana_compat_get_coalesce_old ->
 *                                   mana_compat_coalesce_bridge_get ->
 *                                   mana_get_coalesce (with stack shim)
 *   ethtool -C <if> rx-usecs N   -> mana_compat_set_coalesce_old ->
 *                                   mana_compat_coalesce_bridge_set ->
 *                                   mana_set_coalesce (with stack shim)
 *
 * The rx_cqe_frames plane has no uAPI on those kernels; the OOT private
 * flag rx_cqe_coalesce_4 is its portable door. See the bridge comments.
 */
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
struct ethtool_coalesce;
struct netlink_ext_ack;
int mana_compat_coalesce_bridge_get(struct net_device *dev,
				    struct ethtool_coalesce *ec,
				    struct netlink_ext_ack *extack);
int mana_compat_coalesce_bridge_set(struct net_device *dev,
				    struct ethtool_coalesce *ec,
				    struct netlink_ext_ack *extack);
#endif

#endif /* MANA_COMPAT_ETHTOOL_OPS_H */
