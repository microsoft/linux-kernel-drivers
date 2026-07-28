/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * netdevice back-compat used at the anchor commit:
 *   - netif_napi_add_weight() weighted NAPI-add (introduced v5.19; on 5.15
 *     the weight is the 4th arg of netif_napi_add itself).
 *   - netif_napi_add_tx() TX-only NAPI-add (introduced v5.19; on 5.15
 *     resolves to netif_napi_add_weight + NAPI_POLL_WEIGHT).
 *   - netdev_lockdep_set_classes() lockdep-classes helper. Kernels <= v6.14
 *     provide it in <linux/netdevice.h>; kernels v6.15+ moved it to
 *     <net/netdev_lock.h>. Force-include the newer header when present.
 */
#ifndef MANA_COMPAT_NETDEV_H
#define MANA_COMPAT_NETDEV_H

#include <linux/netdevice.h>

/* v6.15+ moved netdev_lockdep_set_classes to <net/netdev_lock.h>. Pull it
 * in unconditionally where present so the driver's plain call resolves. */
#if defined(__has_include)
# if __has_include(<net/netdev_lock.h>)
#  include <net/netdev_lock.h>
# endif
#endif

/* netif_napi_add_weight(): weighted NAPI-add. On < v5.19, weight is the 4th
 * arg of netif_napi_add() itself. */
#if !MANA_HAVE_NETIF_NAPI_ADD_WEIGHT
static inline void netif_napi_add_weight(struct net_device *dev,
					 struct napi_struct *napi,
					 int (*poll)(struct napi_struct *, int),
					 int weight)
{
	netif_napi_add(dev, napi, poll, weight);
}
#endif

/* netif_napi_add_tx(): TX-only NAPI-add helper. On < v5.19, delegate to
 * netif_napi_add_weight with the standard NAPI_POLL_WEIGHT (64). */
#if !MANA_HAVE_NETIF_NAPI_ADD_TX
static inline void netif_napi_add_tx(struct net_device *dev,
				     struct napi_struct *napi,
				     int (*poll)(struct napi_struct *, int))
{
	netif_napi_add_weight(dev, napi, poll, NAPI_POLL_WEIGHT);
}
#endif

#endif /* MANA_COMPAT_NETDEV_H */
