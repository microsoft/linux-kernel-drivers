/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Extern declarations for the ethtool wrapper-TU functions the register_netdev
 * interceptor installs at kernel-visible ops-table slots.
 *
 * Also declares mana_compat_register_netdev(), the interceptor. The umbrella
 * #defines register_netdev to this wrapper so driver-body register_netdev()
 * calls route through us.
 */
#ifndef MANA_COMPAT_ETHTOOL_OPS_H
#define MANA_COMPAT_ETHTOOL_OPS_H

#include <linux/netdevice.h>
#include "mana_compat_ethtool.h"

int mana_compat_register_netdev(struct net_device *ndev);

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
int mana_compat_get_rxfh_old(struct net_device *ndev,
			     u32 *indir, u8 *key, u8 *hfunc);
int mana_compat_set_rxfh_old(struct net_device *ndev,
			     const u32 *indir, const u8 *key, const u8 hfunc);
#endif

#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
void mana_compat_get_ringparam_old(struct net_device *ndev,
				   struct ethtool_ringparam *ring);
int mana_compat_set_ringparam_old(struct net_device *ndev,
				  struct ethtool_ringparam *ring);
#endif

#endif /* MANA_COMPAT_ETHTOOL_OPS_H */
