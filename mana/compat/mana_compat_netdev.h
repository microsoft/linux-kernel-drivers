/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * netdevice back-compat: reference tracker, netdev_hold(), TSO sizing and the
 * weighted NAPI-add helper.
 *
 * The instance-locked NAPI family (napi_*_locked, netif_napi_*_locked) lives in
 * the backport-include/net/netdev_lock.h overlay, because the verbatim body
 * reaches it via `#include <net/netdev_lock.h>`. This header provides the
 * plain (non-overlay) netdevice helpers the body needs.
 */
#ifndef MANA_COMPAT_NETDEV_H
#define MANA_COMPAT_NETDEV_H

#include <linux/netdevice.h>

/* netdevice_tracker: reference tracker type from <net/net_trackers.h> (v5.17,
 * pulled in by <linux/netdevice.h> when present). Detect via that header's own
 * include guard so we only fill the gap on kernels that predate it; the empty
 * struct matches the !CONFIG_NET_DEV_REFCNT_TRACKER form.
 */
#ifndef __NET_NET_TRACKERS_H
typedef struct {} netdevice_tracker;
#endif

/* netif_napi_add_weight(): weighted NAPI add (v6.1). Before 6.1, the weight was
 * the 4th argument to netif_napi_add() itself. Used by the netdev_lock overlay
 * shims as well as wherever the body adds NAPI instances.
 */
#if !MANA_HAVE_NETIF_NAPI_ADD_WEIGHT
static inline void netif_napi_add_weight(struct net_device *dev,
					 struct napi_struct *napi,
					 int (*poll)(struct napi_struct *, int),
					 int weight)
{
	netif_napi_add(dev, napi, poll, weight);
}
#endif

/* netdev_hold(): tracked reference acquire (v6.1). Older kernels take a plain
 * refcount; the tracker argument is ignored.
 */
#if !MANA_HAVE_NETDEV_HOLD
static inline void netdev_hold(struct net_device *dev,
			       netdevice_tracker *tracker, gfp_t gfp)
{
	(void)tracker;
	(void)gfp;
	dev_hold(dev);
}
#endif

/* netif_set_tso_max_size(): cap the TSO/GSO size (v5.19). Older kernels expose
 * the same cap as netif_set_gso_max_size().
 */
#if !MANA_HAVE_NETIF_SET_TSO_MAX_SIZE
static inline void netif_set_tso_max_size(struct net_device *dev,
					  unsigned int size)
{
	netif_set_gso_max_size(dev, size);
}
#endif

/* netdev_lock()/netdev_unlock(): per-instance ops-mutex acquire/release
 * (v6.15). The verbatim body calls these to serialise a small block of
 * link-config-error mutation on the resume path, while already holding
 * rtnl_lock(). On kernels that predate the per-instance netdev lock, RTNL
 * is the only cross-cpu barrier available and is already held, so the
 * netdev_lock() acquire is a no-op there.
 */
#if !MANA_HAVE_NETDEV_LOCK_FN
static inline void netdev_lock(struct net_device *dev)
{
	(void)dev;
}

static inline void netdev_unlock(struct net_device *dev)
{
	(void)dev;
}

/* netdev_assert_locked(): lockdep assertion that the caller holds the
 * per-instance netdev lock. Since we no-op the lock itself on older
 * kernels, no-op the assertion too. Callers of this in the driver body
 * are always already under rtnl_lock(), which is the correct barrier on
 * these kernels.
 */
static inline void netdev_assert_locked(struct net_device *dev)
{
	(void)dev;
}
#endif

#endif /* MANA_COMPAT_NETDEV_H */
