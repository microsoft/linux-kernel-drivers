/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Overlay for <net/netdev_lock.h>.
 *
 * Resolved ahead of the real kernel header via NOSTDINC_FLAGS so the verbatim
 * driver's `#include <net/netdev_lock.h>` (mana_en.c) keeps working everywhere.
 *
 * net/netdev_lock.h and the instance-locked NAPI family appeared in v6.15. On
 * 6.15+ we defer to the real header. On older kernels there is no per-instance
 * netdev lock, so each *_locked helper is exactly its non-locked counterpart.
 *
 * The only other primitive the body pulls from this header is
 * netdev_lockdep_set_classes(); before 7.0 that macro lives in
 * <linux/netdevice.h> (included below), so we must NOT redefine it here.
 *
 * Detection uses MANA_HAVE_NETDEV_LOCK (from configure/version), never
 * __has_include: once this overlay dir is on the include path,
 * __has_include(<net/netdev_lock.h>) is unconditionally true.
 */
#ifndef MANA_BACKPORT_NETDEV_LOCK_H
#define MANA_BACKPORT_NETDEV_LOCK_H

#if MANA_HAVE_NETDEV_LOCK

#include_next <net/netdev_lock.h>

#else /* !MANA_HAVE_NETDEV_LOCK */

#include <linux/netdevice.h>

static inline void napi_disable_locked(struct napi_struct *n)
{
	napi_disable(n);
}

static inline void napi_enable_locked(struct napi_struct *n)
{
	napi_enable(n);
}

static inline void netif_napi_del_locked(struct napi_struct *n)
{
	netif_napi_del(n);
}

/* netdev_lock_ops_to_full()/netdev_unlock_full_to_ops(): transitional helpers
 * present in the driver body from the "Fix potential deadlocks in mana napi
 * ops" change until the tree settled on the final locking model. They move a
 * held ops-lock to the full instance lock and back; on kernels without the
 * per-instance netdev lock there is nothing to transition, so both are no-ops.
 */
static inline void netdev_lock_ops_to_full(struct net_device *dev)
{
	(void)dev;
}

static inline void netdev_unlock_full_to_ops(struct net_device *dev)
{
	(void)dev;
}

/* netif_napi_add_weight() is normalised by mana_compat_netdev.h, so this
 * resolves correctly even on 5.15 where plain netif_napi_add() still takes the
 * weight as its fourth argument.
 */
static inline void
netif_napi_add_weight_locked(struct net_device *dev, struct napi_struct *napi,
			     int (*poll)(struct napi_struct *, int), int weight)
{
	netif_napi_add_weight(dev, napi, poll, weight);
}

static inline void
netif_napi_add_locked(struct net_device *dev, struct napi_struct *napi,
		      int (*poll)(struct napi_struct *, int))
{
	netif_napi_add_weight(dev, napi, poll, NAPI_POLL_WEIGHT);
}

#endif /* MANA_HAVE_NETDEV_LOCK */

#endif /* MANA_BACKPORT_NETDEV_LOCK_H */
