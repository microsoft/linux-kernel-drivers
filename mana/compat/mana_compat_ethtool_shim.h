/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * ethtool_ops shim (declarations).
 *
 * Shared between mana_compat_ethtool_wrapper.c (which #includes the pristine
 * driver/mana_ethtool.c against this shim view) and mana_compat_ethtool_ops.c
 * (whose runtime interceptor reads the kernel-visible embed of ndev->ethtool_ops).
 * Defining the shim once in a header keeps the two TUs in lockstep -- any
 * field added here is visible to both.
 *
 * The shim starts with an anonymous embed of the real struct ethtool_ops
 * (requires -fplan9-extensions, set in Kbuild). Kernel-visible fields at
 * kernel-known offsets stay reachable via that embed.
 *
 * The trailing slots below (`__get_rxfh_new`, `__get_ringparam_new`,
 * `__get_coalesce_new`, and their setters) are TYPING ANCHORS ONLY: they
 * exist so that the wrapper's `#define get_rxfh __get_rxfh_new` (etc.)
 * routes the driver body's `.get_rxfh = mana_get_rxfh` assignment to a
 * field whose function-pointer type matches the driver's NEW-sig function
 * type on kernels where the kernel-visible field has an incompatible
 * (older) type. The values stored in these trailing slots are NEVER
 * dereferenced at runtime -- the OLD-sig adapters in
 * mana_compat_ethtool_wrapper.c call the driver's static functions
 * (mana_get_rxfh, mana_get_ringparam, etc.) directly by name, since the
 * adapters live in the same TU as the driver-body include.
 *
 * `get_rx_ring_count` is different: it is a real trailing slot for a
 * kernel field that simply doesn't exist on <v7.0. Its stored value is
 * also never dereferenced (the OOT priv-flags + get_rxnfc fallback serve
 * the same purpose on older kernels), so calling code that would have
 * used it on <v7.0 uses those instead.
 */
#ifndef MANA_COMPAT_ETHTOOL_SHIM_H
#define MANA_COMPAT_ETHTOOL_SHIM_H

#include <linux/ethtool.h>
#include <linux/netdevice.h>

/* mana_compat_ethtool.h supplies struct ethtool_rxfh_param and a
 * struct kernel_ethtool_ringparam forward decl on kernels that lack them.
 */
#include "mana_compat_ethtool.h"

/*
 * struct kernel_ethtool_coalesce shim -- adds .rx_cqe_frames and .rx_cqe_nsecs
 * as trailing fields on kernels < v7.1. The driver's coalesce handlers write
 * these members through a shim-typed pointer (see the wrapper's
 * `#define kernel_ethtool_coalesce`). Runtime safety: the shim's
 * `__get_coalesce_new` / `__set_coalesce_new` trailing pointers are never
 * called (the kernel-visible .get_coalesce/.set_coalesce are NULL on <v7.1),
 * so no one ever dereferences a real kernel_ethtool_coalesce as a shim.
 *
 * Declared BEFORE __mana_ethtool_ops_shim because the latter's coalesce
 * trailing slots reference this type by name.
 */
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
struct __mana_kernel_ethtool_coalesce_shim {
	struct kernel_ethtool_coalesce;
	u32 rx_cqe_frames;
	u32 rx_cqe_nsecs;
};
#endif

struct __mana_ethtool_ops_shim {
	struct ethtool_ops;			/* anonymous embed (-fplan9-extensions) */
#if !MANA_HAVE_ETHTOOL_OPS_RX_RING_COUNT	/* kernel field added @v7.0 */
	u32 (*get_rx_ring_count)(struct net_device *dev);
#endif
#if !MANA_HAVE_ETHTOOL_RXFH_PARAM		/* driver's NEW sig vs kernel's OLD @v6.8 */
	/* Typing anchor only. Populated by the driver's `.get_rxfh = mana_get_rxfh`
	 * assignment (via the wrapper's `#define get_rxfh __get_rxfh_new`) so the
	 * driver's ops-table initializer compiles. NEVER dereferenced at runtime:
	 * the OLD-sig adapters in mana_compat_ethtool_wrapper.c call mana_get_rxfh()
	 * directly by name.
	 */
	int (*__get_rxfh_new)(struct net_device *dev,
			      struct ethtool_rxfh_param *rxfh);
	int (*__set_rxfh_new)(struct net_device *dev,
			      struct ethtool_rxfh_param *rxfh,
			      struct netlink_ext_ack *extack);
#endif
#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT		/* driver's NEW sig vs kernel's OLD @v6.0 */
	/* Typing anchor only. Same pattern as __get_rxfh_new above -- populated
	 * by `.get_ringparam = mana_get_ringparam` through the wrapper's rename,
	 * never dereferenced at runtime (adapters call mana_get_ringparam directly).
	 */
	void (*__get_ringparam_new)(struct net_device *dev,
				    struct ethtool_ringparam *ring,
				    struct kernel_ethtool_ringparam *kring,
				    struct netlink_ext_ack *extack);
	int  (*__set_ringparam_new)(struct net_device *dev,
				    struct ethtool_ringparam *ring,
				    struct kernel_ethtool_ringparam *kring,
				    struct netlink_ext_ack *extack);
#endif
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES	/* driver's kernel_coal arg type
						 * gets renamed to the shim on
						 * kernels where .rx_cqe_frames
						 * doesn't exist; the resulting
						 * function-pointer type is not
						 * compatible with the embed's
						 * .get_coalesce field type, so
						 * route the designator to a
						 * trailing slot instead.
						 *
						 * Typing anchor: these fields
						 * hold pointers to mana_get_
						 * coalesce / mana_set_coalesce
						 * so the driver's ops-table
						 * initializer type-checks. They
						 * are NEVER called through this
						 * indirection. The driver body
						 * is #include'd into
						 * mana_compat_ethtool_wrapper.c,
						 * so its file-static
						 * mana_{get,set}_coalesce
						 * symbols are directly callable
						 * from the coalesce-bridge
						 * functions in the same TU,
						 * which build a stack-local
						 * __mana_kernel_ethtool_coalesce_shim
						 * (safe -- has the trailing
						 * rx_cqe_frames the driver
						 * writes) and translate to/from
						 * ec->rx_max_coalesced_frames
						 * for the kernel-visible OLD-sig
						 * adapters in
						 * mana_compat_ethtool_ops.c.
						 */
	int (*__get_coalesce_new)(struct net_device *dev,
				  struct ethtool_coalesce *ec,
				  struct __mana_kernel_ethtool_coalesce_shim *kern,
				  struct netlink_ext_ack *extack);
	int (*__set_coalesce_new)(struct net_device *dev,
				  struct ethtool_coalesce *ec,
				  struct __mana_kernel_ethtool_coalesce_shim *kern,
				  struct netlink_ext_ack *extack);
#endif
#if !MANA_HAVE_ETHTOOL_OPS_NEEDS_RTNL		/* kernel field added @v7.2 */
	/* Typing anchor only. Populated by the driver's
	 * `.op_needs_rtnl = ETHTOOL_OP_NEEDS_RTNL_* | ...` assignment via the
	 * wrapper's `#define op_needs_rtnl __op_needs_rtnl_new`. Never
	 * dereferenced at runtime: on kernels < v7.2 the ethtool netlink SET
	 * path always holds rtnl unconditionally, so there is no rtnl-shedding
	 * path for the bits to gate. All ETHTOOL_OP_NEEDS_RTNL_* macros
	 * expand to 0 on those kernels (see mana_compat_ethtool.h), so the
	 * stored value is also always 0.
	 */
	u32 __op_needs_rtnl_new;
#endif
};

#endif /* MANA_COMPAT_ETHTOOL_SHIM_H */
