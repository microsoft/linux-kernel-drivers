/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * ethtool_ops shim (declarations).
 *
 * Shared between mana_compat_ethtool_wrapper.c (which #includes the pristine
 * driver/mana_ethtool.c against this shim view) and mana_compat_ethtool_ops.c
 * (whose runtime interceptor reads the kernel-visible embed of
 * ndev->ethtool_ops).
 *
 * The shim starts with an anonymous embed of the real struct ethtool_ops
 * (requires -fplan9-extensions, set in Kbuild). At the anchor commit the
 * only trailing slot pair is the rxfh typing anchor. Later shim commits
 * extend the trailing block at their trigger points.
 */
#ifndef MANA_COMPAT_ETHTOOL_SHIM_H
#define MANA_COMPAT_ETHTOOL_SHIM_H

#include <linux/ethtool.h>
#include <linux/netdevice.h>

#include "mana_compat_ethtool.h"

struct __mana_ethtool_ops_shim {
	struct ethtool_ops;

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
	int (*__get_rxfh_new)(struct net_device *dev,
			      struct ethtool_rxfh_param *rxfh);
	int (*__set_rxfh_new)(struct net_device *dev,
			      struct ethtool_rxfh_param *rxfh,
			      struct netlink_ext_ack *extack);
#endif

#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
	/* Typing anchor slots for the extended (v5.16+) ringparam ops. */
	void (*__get_ringparam_new)(struct net_device *dev,
				    struct ethtool_ringparam *ring,
				    struct kernel_ethtool_ringparam *kring,
				    struct netlink_ext_ack *extack);
	int (*__set_ringparam_new)(struct net_device *dev,
				   struct ethtool_ringparam *ring,
				   struct kernel_ethtool_ringparam *kring,
				   struct netlink_ext_ack *extack);
#endif
};

#endif /* MANA_COMPAT_ETHTOOL_SHIM_H */
