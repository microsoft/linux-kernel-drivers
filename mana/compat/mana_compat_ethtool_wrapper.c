// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Wrapper TU that compiles driver/mana_ethtool.c against a shimmed view of
 * the kernel-owned ethtool structs, so the driver body stays byte-for-byte
 * upstream v7.1-rc7 -- no marked-hunk patch, no build-time source
 * transformation.
 *
 * This file is the ONLY compilation unit that ever preprocesses
 * driver/mana_ethtool.c. Kbuild lists compat/mana_compat_ethtool_wrapper.o
 * in mana-compat-objs (not driver/mana_ethtool.o).
 *
 * Order-of-operations MATTERS here:
 *
 *   1. Include the kernel headers first so struct ethtool_ops,
 *      struct kernel_ethtool_coalesce, and every other real type is fully
 *      defined with its real name. Their field declarations (e.g. the field
 *      `ethtool_ops` in struct net_device) are captured at this point.
 *
 *   2. Include the shim struct definitions -- they anonymously embed the
 *      real kernel types, which requires those types to already be
 *      completely defined. Applying our `#define ethtool_ops __shim` before
 *      this would break the embed (the shim would self-reference).
 *
 *   3. Apply the two families of renames:
 *        (a) type name renames -- redirect the driver body's
 *            `struct ethtool_ops mana_ethtool_ops` and
 *            `struct kernel_ethtool_coalesce *kernel_coal` to our shim
 *            types, so trailing-slot field accesses compile.
 *        (b) field-designator renames -- redirect the driver body's
 *            `.get_rxfh = mana_get_rxfh` etc. to the shim's trailing
 *            __get_rxfh_new slot on kernels where the kernel-visible
 *            .get_rxfh field has a signature that doesn't match the
 *            driver's function. The kernel-visible slot stays NULL until
 *            the register_netdev interceptor installs an OLD-sig adapter.
 *
 *        These renames rely on the fact that `mana_ethtool.c` (unlike
 *        `mana_en.c`) never accesses `ndev->ethtool_ops` -- it only
 *        DEFINES `mana_ethtool_ops`. The field-name collision that made
 *        `#define ethtool_ops ...` unsafe globally therefore has no
 *        occurrences to trip over in this TU.
 *
 *   4. #include the pristine driver body.
 *
 * See compat/mana_compat_ethtool_ops.c for the register_netdev interceptor
 * that publishes the shim to the kernel with adapters and safety NULLs
 * installed at kernel-visible slots.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>

/* NOTE: <net/mana/mana.h> is included AFTER the type renames below, not here.
 * It declares `extern const struct ethtool_ops mana_ethtool_ops;`, and we need
 * that extern to see the shim type on kernels that require the shim so the
 * declaration matches the definition emitted by the included driver body.
 * The shim struct definition itself (in mana_compat_ethtool_shim.h) still
 * needs the real, unrenamed `struct ethtool_ops` to embed, which is why the
 * shim.h include comes BEFORE the type-rename defines.
 */

#include "mana_compat_ethtool_shim.h"

/* --- (3a) type name renames ----------------------------------------------- */

#define ethtool_ops __mana_ethtool_ops_shim
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
#define kernel_ethtool_coalesce __mana_kernel_ethtool_coalesce_shim
#endif

/* --- (b) macro shim for ETHTOOL_COALESCE_RX_CQE_FRAMES -------------------- */
/*
 * The driver body's `.supported_coalesce_params = ETHTOOL_COALESCE_RX_CQE_FRAMES`
 * needs to compile on kernels that don't define the macro. Setting it to 0
 * is exactly what the old kernel would have advertised anyway (no coalesce
 * params supported), and the interceptor NULLs the .{get,set}_coalesce ops on
 * those kernels so nothing depends on the value.
 */
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
#define ETHTOOL_COALESCE_RX_CQE_FRAMES 0
#endif

/* --- (3b) field-designator renames --------------------------------------- */
/*
 * The C preprocessor tokenizes identifiers as whole tokens; `get_rxfh` and
 * `mana_get_rxfh` and `get_rxfh_key_size` are three DIFFERENT tokens even
 * though they share a substring. `#define get_rxfh __get_rxfh_new` therefore
 * rewrites only the exact identifier `get_rxfh`, which in this TU appears
 * exclusively as the field designator in the ops-table initializer -- the
 * driver's functions (`mana_get_rxfh`) and the surrounding fields
 * (`get_rxfh_key_size`, `get_rxfh_indir_size`) are untouched.
 */
#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
#define get_rxfh __get_rxfh_new
#define set_rxfh __set_rxfh_new
#endif
#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
#define get_ringparam __get_ringparam_new
#define set_ringparam __set_ringparam_new
#endif
#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
#define get_coalesce __get_coalesce_new
#define set_coalesce __set_coalesce_new
#endif
#if !MANA_HAVE_ETHTOOL_OPS_NEEDS_RTNL
#define op_needs_rtnl __op_needs_rtnl_new
#endif

/* mana.h must see the same renamed types the driver body will use, so the
 * `extern const struct ethtool_ops mana_ethtool_ops;` declaration matches
 * the definition below after our #defines expand.
 */
#include <net/mana/mana.h>

/* --- (4) compile the pristine upstream body ------------------------------- */

#include "../driver/mana_ethtool.c"

/* --- (5) OOT-augmented ethtool ops that need driver-body-static access ---- */
/*
 * These are the kernel-facing ops installed by the register_netdev interceptor
 * at ndev->ethtool_ops. Living HERE (in the wrapper TU, AFTER the driver-body
 * include) lets them call the driver's file-static mana_get_sset_count() /
 * mana_get_strings() / mana_get_rxfh() / mana_get_ringparam() / etc.
 * DIRECTLY by name -- no cached function pointer to populate, no runtime
 * cast, no function-pointer indirection through a trailing shim slot.
 *
 * Two families of ops:
 *
 *   (a) ETH_SS_PRIV_FLAGS delegates -- mana_compat_get_sset_count and
 *       mana_compat_get_strings. They intercept the PRIV_FLAGS stringset (which
 *       the upstream body does not know about, since the OOT rx_cqe_coalesce_4
 *       priv-flag is additive) and forward every other stringset to the
 *       driver's originals unchanged.
 *
 *   (b) OLD-sig adapters -- mana_compat_get_rxfh_old / set_rxfh_old and
 *       mana_compat_get_ringparam_old / set_ringparam_old. Present only when
 *       the target kernel's struct ethtool_ops field type differs from the
 *       driver body's NEW-sig function type. The trailing __get_rxfh_new /
 *       __get_ringparam_new slots in struct __mana_ethtool_ops_shim exist so
 *       the wrapper's field-designator `#define get_rxfh __get_rxfh_new` etc.
 *       gives the driver's `.get_rxfh = mana_get_rxfh` assignment a
 *       type-compatible target, but those stored values are TYPING-ANCHORS
 *       ONLY -- never dereferenced at runtime. See the slot comments in
 *       mana_compat_ethtool_shim.h.
 *
 * Extern-visible so mana_compat_ethtool_ops.c's interceptor can install them.
 * Declarations in mana_compat_ethtool_ops.h use the same signatures.
 *
 * Identifier-tokenisation safety: the wrapper's #defines above rewrite only
 * the exact tokens `get_rxfh`, `set_rxfh`, `get_ringparam`, `set_ringparam`
 * (field designators). Function names like `mana_get_rxfh`,
 * `mana_get_sset_count`, and `mana_compat_get_ringparam_old` share substrings
 * with those tokens but are DIFFERENT tokens -- the preprocessor treats
 * identifiers as atomic -- so nothing here is accidentally renamed.
 */

int mana_compat_get_sset_count(struct net_device *ndev, int stringset)
{
	if (stringset == ETH_SS_PRIV_FLAGS)
		return mana_compat_priv_flags_count();

	return mana_get_sset_count(ndev, stringset);
}

void mana_compat_get_strings(struct net_device *ndev, u32 stringset, u8 *data)
{
	if (stringset == ETH_SS_PRIV_FLAGS) {
		mana_compat_priv_flags_strings(&data);
		return;
	}

	mana_get_strings(ndev, stringset, data);
}

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
int mana_compat_get_rxfh_old(struct net_device *ndev, u32 *indir, u8 *key,
			     u8 *hfunc)
{
	struct ethtool_rxfh_param rxfh = {
		.indir = indir,
		.key   = key,
	};
	int ret = mana_get_rxfh(ndev, &rxfh);

	if (hfunc)
		*hfunc = rxfh.hfunc;
	return ret;
}

int mana_compat_set_rxfh_old(struct net_device *ndev, const u32 *indir,
			     const u8 *key, const u8 hfunc)
{
	struct ethtool_rxfh_param rxfh = {
		.indir = (u32 *)indir,
		.key   = (u8 *)key,
		.hfunc = hfunc,
	};

	return mana_set_rxfh(ndev, &rxfh, NULL);
}
#endif

#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
void mana_compat_get_ringparam_old(struct net_device *ndev,
				   struct ethtool_ringparam *ring)
{
	mana_get_ringparam(ndev, ring, NULL, NULL);
}

int mana_compat_set_ringparam_old(struct net_device *ndev,
				  struct ethtool_ringparam *ring)
{
	return mana_set_ringparam(ndev, ring, NULL, NULL);
}
#endif

#if !MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES
/*
 * Coalesce bridge for kernels < v7.1 that lack
 * struct kernel_ethtool_coalesce.rx_cqe_frames.
 *
 * The driver's mana_get_coalesce / mana_set_coalesce write and read
 * kernel_coal->rx_cqe_frames -- which on <v7.1 lives only in our shim
 * `__mana_kernel_ethtool_coalesce_shim`, not in the real kernel struct.
 * The kernel-visible .get_coalesce / .set_coalesce ops slot expects the
 * REAL kernel_ethtool_coalesce type on those kernels; casting a kernel-
 * allocated struct to our shim type would read/write past the end of
 * the allocation (the trailing fields don't exist there).
 *
 * These bridge functions -- defined in this wrapper TU where the driver's
 * static mana_{get,set}_coalesce are visible AND kernel_ethtool_coalesce
 * has been renamed to __mana_kernel_ethtool_coalesce_shim -- allocate the
 * shim on their own stack and call the driver's own functions with it. The
 * OLD-sig adapters in mana_compat_ethtool_ops.c call these bridges, so the
 * kernel's real kernel_ethtool_coalesce is never accessed as if it were a
 * shim. Everything the *legacy* struct ethtool_coalesce carries (interrupt
 * moderation usecs/frames, adaptive RX/TX) is passed straight through in
 * `ec`, so it keeps its upstream meaning on every kernel.
 *
 * The rx_cqe_frames plane is the only thing that has no uAPI here: the
 * netlink attribute behind it is a v7.1 kernel-core addition and cannot be
 * shimmed. So on <v7.1 the bridge feeds the driver the CURRENT device
 * state, which makes the driver's own validation pass and its
 * reprogram-on-change test a no-op -- leaving that knob solely to the OOT
 * private flag `rx_cqe_coalesce_4` (compat/mana_compat_privflags.c), which
 * reaches the same mana_port_context::cqe_coalescing_enable on every
 * supported kernel.
 *
 * NOTE: this deliberately no longer borrows ec->rx_max_coalesced_frames as
 * a stand-in for rx_cqe_frames. Since upstream's interrupt-moderation
 * support that field means the RX moderation completion count and is
 * consumed by the driver body itself; hijacking it would both misprogram
 * moderation and silently toggle CQE coalescing.
 */
int mana_compat_coalesce_bridge_get(struct net_device *ndev,
				    struct ethtool_coalesce *ec,
				    struct netlink_ext_ack *extack)
{
	struct __mana_kernel_ethtool_coalesce_shim kern = {};

	/* kern is write-only here: its rx_cqe_frames / rx_cqe_nsecs have no
	 * uAPI to be reported through on this kernel. Everything the user
	 * can actually see is returned in ec.
	 */
	return mana_get_coalesce(ndev, ec, &kern, extack);
}

int mana_compat_coalesce_bridge_set(struct net_device *ndev,
				    struct ethtool_coalesce *ec,
				    struct netlink_ext_ack *extack)
{
	struct mana_port_context *apc = netdev_priv(ndev);
	/* Re-assert the current CQE-coalescing state so the driver's
	 * "must be 1 or MANA_RXCOMP_OOB_NUM_PPI" validation passes and its
	 * changed/unchanged test sees no change -- no RSS reprogram, no
	 * surprise toggle. Mirrors what mana_get_coalesce() would report.
	 */
	struct __mana_kernel_ethtool_coalesce_shim kern = {
		.rx_cqe_frames = apc->cqe_coalescing_enable ?
				 MANA_RXCOMP_OOB_NUM_PPI : 1,
		.rx_cqe_nsecs  = apc->cqe_coalescing_timeout_ns,
	};

	return mana_set_coalesce(ndev, ec, &kern, extack);
}
#endif
