/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Overlay for <net/net_shaper.h>.
 *
 * Resolved ahead of the real kernel header via NOSTDINC_FLAGS so the verbatim
 * driver's `#include <net/net_shaper.h>` (mana.h) keeps working everywhere.
 *
 * The net_shaper API landed in v6.13. On 6.13+ we defer to the real header. On
 * older kernels we provide the type surface the verbatim body mentions. The
 * shaper *ops* (mana_shaper_*) and their registration via
 * net_device_ops.net_shaper_ops are compiled only when MANA_HAVE_NET_SHAPER
 * (a marked hunk in mana_en.c), because that net_device_ops member does not
 * exist pre-6.13. struct net_shaper_handle, however, is an *ungated* member of
 * struct mana_port_context, so these types must always be defined.
 *
 * Detection uses MANA_HAVE_NET_SHAPER (from configure/version), never
 * __has_include: once this overlay dir is on the include path,
 * __has_include(<net/net_shaper.h>) is unconditionally true.
 */
#ifndef MANA_BACKPORT_NET_SHAPER_H
#define MANA_BACKPORT_NET_SHAPER_H

#if MANA_HAVE_NET_SHAPER

#include_next <net/net_shaper.h>

#else /* !MANA_HAVE_NET_SHAPER */

#include <linux/types.h>

struct net_device;
struct devlink;
struct netlink_ext_ack;

/* Mirrors uapi/linux/net_shaper.h (enumerator order is ABI; keep verbatim). */
enum net_shaper_scope {
	NET_SHAPER_SCOPE_UNSPEC,
	NET_SHAPER_SCOPE_NETDEV,
	NET_SHAPER_SCOPE_QUEUE,
	NET_SHAPER_SCOPE_NODE,
	__NET_SHAPER_SCOPE_MAX,
	NET_SHAPER_SCOPE_MAX = (__NET_SHAPER_SCOPE_MAX - 1)
};

enum net_shaper_metric {
	NET_SHAPER_METRIC_BPS,
	NET_SHAPER_METRIC_PPS,
};

/* Mirrors net/net_shaper.h. */
enum net_shaper_binding_type {
	NET_SHAPER_BINDING_TYPE_NETDEV,
};

struct net_shaper_binding {
	enum net_shaper_binding_type type;
	union {
		struct net_device *netdev;
		struct devlink *devlink;
	};
};

struct net_shaper_handle {
	enum net_shaper_scope scope;
	u32 id;
};

struct net_shaper {
	struct net_shaper_handle parent;
	struct net_shaper_handle handle;
	enum net_shaper_metric metric;
	u64 bw_min;
	u64 bw_max;
	u64 burst;
	u32 priority;
	u32 weight;

	u32 leaves;
	struct rcu_head rcu;
};

struct net_shaper_ops {
	int (*group)(struct net_shaper_binding *binding, int leaves_count,
		     const struct net_shaper *leaves,
		     const struct net_shaper *node,
		     struct netlink_ext_ack *extack);
	int (*set)(struct net_shaper_binding *binding,
		   const struct net_shaper *shaper,
		   struct netlink_ext_ack *extack);
	int (*delete)(struct net_shaper_binding *binding,
		      const struct net_shaper_handle *handle,
		      struct netlink_ext_ack *extack);
	void (*capabilities)(struct net_shaper_binding *binding,
			     enum net_shaper_scope scope, unsigned long *cap);
};

/* Mirrors uapi/linux/net_shaper.h. Only the two attributes mana_shaper_cap()
 * asserts (BW_MAX and METRIC_BPS) matter to the driver body; keep the rest for
 * enum-value stability so future re-vendors that widen the capability mask
 * don't drift.
 */
enum {
	NET_SHAPER_A_CAPS_UNSPEC,
	NET_SHAPER_A_CAPS_IFINDEX,
	NET_SHAPER_A_CAPS_SCOPE,
	NET_SHAPER_A_CAPS_SUPPORT_METRIC_BPS,
	NET_SHAPER_A_CAPS_SUPPORT_METRIC_PPS,
	NET_SHAPER_A_CAPS_SUPPORT_NESTING,
	NET_SHAPER_A_CAPS_SUPPORT_BW_MIN,
	NET_SHAPER_A_CAPS_SUPPORT_BW_MAX,
	NET_SHAPER_A_CAPS_SUPPORT_BURST,
	NET_SHAPER_A_CAPS_SUPPORT_PRIORITY,
	NET_SHAPER_A_CAPS_SUPPORT_WEIGHT,
	__NET_SHAPER_A_CAPS_MAX,
	NET_SHAPER_A_CAPS_MAX = (__NET_SHAPER_A_CAPS_MAX - 1)
};

#endif /* MANA_HAVE_NET_SHAPER */

/*
 * net_device_ops shim for the .net_shaper_ops member (v6.13+).
 *
 * On kernels < v6.13 the kernel-owned `struct net_device_ops` has no
 * .net_shaper_ops member, so the verbatim body's initializer
 *
 *     static const struct net_device_ops mana_devops = {
 *         ...,
 *         .net_shaper_ops = &mana_shaper_ops,
 *     };
 *
 * would fail to compile. We wrap the kernel struct in an anonymous embedding
 * (a GCC/Clang extension enabled for this module by `-fplan9-extensions` in
 * Kbuild) and add the missing member as a trailing field. Two consequences
 * make this drop-in with no driver-body change:
 *
 *   1. Designated initializers written against the outer shim reach the
 *      embedded struct's members transparently — .ndo_open etc. still
 *      resolve to the kernel struct's fields, and .net_shaper_ops resolves
 *      to our trailing slot.
 *   2. Because the anonymous member is the first (and only kernel-visible)
 *      member of the shim, `-fplan9-extensions` permits the implicit pointer
 *      conversion at `ndev->netdev_ops = &mana_devops`. The address is
 *      identical to the address of the embedded struct, so the kernel reads
 *      every field it knows about at the correct offset; the trailing
 *      .net_shaper_ops slot lies past `sizeof(struct net_device_ops)` and is
 *      never touched by the kernel.
 *
 * With this in place, driver/mana_en.c stays byte-identical to upstream —
 * no marked hunk in the driver body is needed for the ops-table registration.
 */
#if !MANA_HAVE_NET_SHAPER

struct net_device_ops_shim {
	struct net_device_ops;			/* anonymous embed */
	const struct net_shaper_ops *net_shaper_ops;
};

#define net_device_ops net_device_ops_shim

#endif /* !MANA_HAVE_NET_SHAPER */

#endif /* MANA_BACKPORT_NET_SHAPER_H */
