/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_LEGACY_H
#define MANA_IB_COMPAT_LEGACY_H

#include <net/mana/mana.h>

#if !MANA_IB_HAVE_NETDEV_PUT
static inline void netdev_put(struct net_device *dev, netdevice_tracker *tracker)
{
	dev_put(dev);
}
#endif

#endif
