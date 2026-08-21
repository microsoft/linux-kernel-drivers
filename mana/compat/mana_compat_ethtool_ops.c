// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Out-of-tree ethtool ops-table augmentation (definitions).
 *
 * See mana_compat_ethtool_ops.h for the rationale.
 *
 * All the entries this file adds are additive-to-upstream: the OOT
 * priv-flags plane, the ETH_SS_PRIV_FLAGS wrappers around the driver's
 * (file-static) get_sset_count/get_strings, a legacy
 * .get_rxnfc(ETHTOOL_GRXRINGS) fallback for kernels < v7.0, and adapters
 * for the ethtool ops whose function-pointer signatures don't match the
 * driver's NEW-sig types on older kernels.
 *
 * This TU is compiled as a separate object because it must be able to
 * reach the kernel's real register_netdev(). The umbrella and
 * mana_compat_ethtool_ops.h install `#define register_netdev
 * mana_compat_register_netdev` for the rest of the module; we #undef it
 * AFTER all includes so the recursive-lookup problem doesn't fire when
 * this TU's own definition of mana_compat_register_netdev calls
 * register_netdev.
 */

#include <linux/errno.h>
#include <linux/ethtool.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <linux/types.h>

#include <net/mana/mana.h>

#include "mana_compat_ethtool_ops.h"
#include "mana_compat_ethtool_shim.h"
#include "mana_compat_privflags.h"

/*
 * Pop the umbrella's `#define register_netdev mana_compat_register_netdev`
 * so this TU's calls to register_netdev() at the end of the interceptor
 * resolve to the kernel's real function, not a recursive self-call. Placed
 * AFTER the include chain because mana_compat_ethtool_ops.h reinstalls the
 * macro whenever it's included.
 */
#undef register_netdev

/*
 * The kernel-facing ops-table entries that would once have lived here --
 * ETH_SS_PRIV_FLAGS-aware get_sset_count / get_strings wrappers, and the
 * OLD-sig rxfh / ringparam adapters -- are defined in
 * mana_compat_ethtool_wrapper.c instead. Living in the same TU as the
 * included driver body lets them call the driver's file-static
 * mana_get_sset_count / mana_get_strings / mana_get_rxfh / mana_set_rxfh /
 * mana_get_ringparam / mana_set_ringparam DIRECTLY by name -- no cached
 * function pointers, no shim-cast on ndev->ethtool_ops, no runtime
 * indirection through a trailing shim slot. Their extern declarations live
 * in mana_compat_ethtool_ops.h so the interceptor below can install them.
 */

/*
 * Legacy .get_rxnfc(ETHTOOL_GRXRINGS) fallback for kernels < v7.0. Upstream
 * v7.0 replaced the "shove ring count through get_rxnfc" pattern with a
 * dedicated .get_rx_ring_count op and dropped mana_get_rxnfc from the
 * driver, so on older kernels the core has no way to read the ring count
 * ("ethtool -X <if> equal N" and other RXFH sets fail with -EOPNOTSUPP).
 * This restores the pre-7.0 in-tree behaviour verbatim; only installed by
 * the interceptor below when the target kernel lacks .get_rx_ring_count.
 *
 * Stays in this TU (not the wrapper) because it does not need access to
 * any of the driver body's file-static functions -- it reads apc->num_queues
 * through the publicly-visible mana_port_context in <net/mana/mana.h>.
 */
#if !MANA_HAVE_ETHTOOL_OPS_RX_RING_COUNT
static int mana_compat_get_rxnfc(struct net_device *ndev,
				 struct ethtool_rxnfc *cmd, u32 *rules)
{
	struct mana_port_context *apc = netdev_priv(ndev);

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = apc->num_queues;
		return 0;
	}

	return -EOPNOTSUPP;
}
#endif

/*
 * OLD-sig ops adapters. Live in this TU (not the wrapper) because their
 * signatures need to name the REAL struct kernel_ethtool_coalesce -- which in
 * the wrapper TU has been renamed to __mana_kernel_ethtool_coalesce_shim by
 * the wrapper's `#define kernel_ethtool_coalesce`. The bridge functions they
 * delegate to live in the wrapper TU where the driver's file-static
 * mana_{get,set}_coalesce are directly callable.
 *
 * On <v7.1 the real kernel_ethtool_coalesce lacks `.rx_cqe_frames`, so the
 * adapters IGNORE the incoming `kern` pointer -- the driver's shim-typed
 * handlers must never touch a kernel-allocated struct. The bridge builds its
 * own stack-local shim. `ec` and `extack` are passed straight through: every
 * field the legacy struct ethtool_coalesce carries keeps its upstream meaning.
 */
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
static int mana_compat_get_coalesce_old(struct net_device *ndev,
					struct ethtool_coalesce *ec,
					struct kernel_ethtool_coalesce *kern,
					struct netlink_ext_ack *extack)
{
	(void)kern;
	return mana_compat_coalesce_bridge_get(ndev, ec, extack);
}

static int mana_compat_set_coalesce_old(struct net_device *ndev,
					struct ethtool_coalesce *ec,
					struct kernel_ethtool_coalesce *kern,
					struct netlink_ext_ack *extack)
{
	(void)kern;
	return mana_compat_coalesce_bridge_set(ndev, ec, extack);
}
#endif

/*
 * The augmented shadow of mana_ethtool_ops. IS a shim (not just the real
 * struct) so trailing fields the driver body wrote via the wrapper's
 * field-designator renames survive the publishing memcpy without truncation.
 * The kernel dereferences ndev->ethtool_ops at kernel-visible offsets, i.e.
 * inside the anonymous embed, which is bit-identical to a real struct
 * ethtool_ops -- so as far as the kernel is concerned this is an ordinary
 * ops-table. The trailing slots stored in the shadow are TYPING-ANCHORS
 * ONLY -- never dereferenced at runtime; see the slot comments in
 * mana_compat_ethtool_shim.h. The kernel-facing wrappers and OLD-sig adapters
 * that would once have read them live in mana_compat_ethtool_wrapper.c and
 * reach the driver's functions directly by name.
 *
 * Concurrency model (no locks, no ready-flag by design):
 *
 * Every call to mana_compat_register_netdev() reads the SAME source
 * (ndev->ethtool_ops is always &mana_ethtool_ops -- one module-wide const)
 * and applies the SAME augmentations, so the fully-augmented target content
 * is byte-identical across every call. That makes concurrent publishing
 * safe: torn reads between two identical values resolve to that identical
 * value, so any reader (userspace ethtool ioctl on an already-registered
 * port) sees a consistent ops-table regardless of how many probes race.
 *
 * The one thing we MUST avoid is exposing the intermediate state in which
 * the shared shadow briefly holds the raw driver ops-table (from the memcpy
 * of the source) before per-field fix-ups install our wrappers, priv-flags,
 * and OLD-sig adapters. Such a window is not idempotent: it contains real
 * different values (NULL priv-flags, NEW-sig pointers in OLD-sig slots) that
 * a concurrent ethtool call on a live port from another MANA device would
 * observe and mis-dispatch (on <v6.8 kernels the sig mismatch would call a
 * NEW-sig function via an OLD-sig ABI -- a crash).
 *
 * To eliminate that window: build the fully-augmented shim in on-stack
 * `staging`, then publish it to the shared shadow with a single memcpy. The
 * publishing memcpy still isn't a single-word atomic, but it writes the same
 * bytes that are already there (after the first-ever call), so torn reads
 * are same-value.
 *
 * First-ever call has an initially-zero shadow, but there is no concurrent
 * reader: no port has been through register_netdev() yet, so no live
 * ndev->ethtool_ops points at the shadow.
 */
static struct __mana_ethtool_ops_shim mana_compat_augmented_ops;

int mana_compat_register_netdev(struct net_device *ndev)
{
	struct __mana_ethtool_ops_shim staging;
	const struct __mana_ethtool_ops_shim *src =
		(const struct __mana_ethtool_ops_shim *)ndev->ethtool_ops;

	/* Snapshot the driver's ops-table (kernel-visible embed + trailing
	 * NEW-sig slots) into on-stack staging. Zero readers, no shared
	 * state.
	 */
	memcpy(&staging, src, sizeof(staging));

	/* Wrap sset/strings so ETH_SS_PRIV_FLAGS is served. The wrappers
	 * live in the wrapper TU and call the driver's file-static
	 * mana_get_sset_count / mana_get_strings directly by name -- no
	 * cached function pointer to populate here.
	 */
	staging.get_sset_count = mana_compat_get_sset_count;
	staging.get_strings = mana_compat_get_strings;

	/* Inject the OOT priv-flags plane. */
	staging.get_priv_flags = mana_compat_get_priv_flags;
	staging.set_priv_flags = mana_compat_set_priv_flags;

#if !MANA_HAVE_ETHTOOL_OPS_RX_RING_COUNT
	/* Restore the pre-v7.0 ring-count path. */
	staging.get_rxnfc = mana_compat_get_rxnfc;
#endif

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
	/* The driver's NEW-sig mana_get_rxfh went to the shim's trailing
	 * __get_rxfh_new slot; the kernel-visible .get_rxfh (OLD sig on
	 * <v6.8) was left NULL by the initializer. Install the OLD-sig
	 * adapter at the kernel-visible slot.
	 */
	staging.get_rxfh = mana_compat_get_rxfh_old;
	staging.set_rxfh = mana_compat_set_rxfh_old;
#endif

#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
	/* Same idea for the OLD 2-arg ringparam signature on <v6.0. */
	staging.get_ringparam = mana_compat_get_ringparam_old;
	staging.set_ringparam = mana_compat_set_ringparam_old;
#endif

#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
	/*
	 * On kernels < v7.1 the driver's `.get_coalesce = mana_get_coalesce`
	 * / `.set_coalesce = mana_set_coalesce` designators were renamed by
	 * the wrapper's `#define get_coalesce __get_coalesce_new` etc. so
	 * they compile against the shim `__mana_kernel_ethtool_coalesce_shim`
	 * (which has the .rx_cqe_frames field the driver body uses); the
	 * kernel-visible slots therefore stay NULL after the memcpy.
	 *
	 * Install OLD-sig adapters at the kernel-visible slots. They forward
	 * ec/extack unchanged to mana_compat_coalesce_bridge_{get,set} in the
	 * wrapper TU, which supplies the stack-local shim.
	 *
	 * Kernels < v7.1 also lack ETHTOOL_COALESCE_RX_CQE_FRAMES, which the
	 * wrapper defines to 0 -- so keep a floor under whatever the driver
	 * body advertises: the ethtool netlink core dispatches a SET only for
	 * bits present here, and an all-zero mask would make -c/-C return
	 * -EOPNOTSUPP outright. ETHTOOL_COALESCE_RX_MAX_FRAMES exists on every
	 * supported kernel and is a parameter the driver body implements, so
	 * OR-ing it in never claims something we do not honour.
	 */
	staging.supported_coalesce_params |= ETHTOOL_COALESCE_RX_MAX_FRAMES;
	staging.get_coalesce = mana_compat_get_coalesce_old;
	staging.set_coalesce = mana_compat_set_coalesce_old;
#endif

#if !MANA_HAVE_DIMLIB
	/*
	 * CONFIG_DIMLIB=n: net_dim() is stubbed out (compat/mana_compat_dim.h),
	 * so adaptive moderation cannot work. Withdraw the advertisement --
	 * the ethtool core then rejects `-C adaptive-rx/tx on` with
	 * -EOPNOTSUPP instead of silently accepting a knob that does nothing --
	 * and clear the per-port enables the driver set from
	 * MANA_ADAPTIVE_{RX,TX}_DEF, so `ethtool -c` reports the truth and the
	 * NAPI-side mana_update_{rx,tx}_dim() short-circuit before net_dim().
	 * Static interrupt moderation is unaffected: it is plain doorbell
	 * traffic and needs no library.
	 */
	staging.supported_coalesce_params &=
		~(u32)(ETHTOOL_COALESCE_USE_ADAPTIVE_RX |
		       ETHTOOL_COALESCE_USE_ADAPTIVE_TX);
	{
		struct mana_port_context *apc = netdev_priv(ndev);

		apc->rx_dim_enabled = false;
		apc->tx_dim_enabled = false;
	}
#endif

	/* Publish the fully-augmented staging to the shared shadow. Writes
	 * the same bytes that are already there after the first call, so
	 * concurrent readers see same-value torn reads (safe).
	 */
	memcpy(&mana_compat_augmented_ops, &staging,
	       sizeof(mana_compat_augmented_ops));

	ndev->ethtool_ops = (const struct ethtool_ops *)&mana_compat_augmented_ops;
	return register_netdev(ndev);
}
