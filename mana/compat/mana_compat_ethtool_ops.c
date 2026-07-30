// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * register_netdev() interceptor.
 *
 * Kbuild force-includes compat/mana_compat.h into every TU. The umbrella
 * #defines register_netdev to mana_compat_register_netdev; the redefinition
 * is #undef'd at the top of this TU so we can call the real kernel function
 * via its native name.
 *
 * The interceptor takes a net_device whose ->ethtool_ops points at the
 * driver's struct __mana_ethtool_ops_shim (the wrapper TU compiled the
 * driver body against the shim type via `#define ethtool_ops`). It
 * synthesises a real struct ethtool_ops from the shim: kernel-visible
 * fields are copied through, and adapters are installed at slots where
 * driver-sig and kernel-sig differ.
 *
 * The synthesised ops-table is written into a file-static shadow, then
 * ndev->ethtool_ops is repointed at the shadow. Every call produces
 * byte-identical shadow content, so concurrent register_netdev() from
 * different ports is safe without a lock.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>

#include "mana_compat_ethtool.h"
#include "mana_compat_ethtool_shim.h"
#include "mana_compat_ethtool_ops.h"

#undef register_netdev

static struct ethtool_ops mana_compat_shadow_ops;

int mana_compat_register_netdev(struct net_device *ndev)
{
	const struct __mana_ethtool_ops_shim *shim;
	struct ethtool_ops staged = {};

	if (!ndev || !ndev->ethtool_ops)
		return register_netdev(ndev);

	shim = (const struct __mana_ethtool_ops_shim *)ndev->ethtool_ops;

	staged = *(const struct ethtool_ops *)shim;

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
	staged.get_rxfh = mana_compat_get_rxfh_old;
	staged.set_rxfh = mana_compat_set_rxfh_old;
#endif

#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
	staged.get_ringparam = mana_compat_get_ringparam_old;
	staged.set_ringparam = mana_compat_set_ringparam_old;
#endif

	mana_compat_shadow_ops = staged;
	ndev->ethtool_ops = &mana_compat_shadow_ops;

	return register_netdev(ndev);
}
