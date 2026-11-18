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

static inline int mana_ib_compat_cfg_vport(struct mana_port_context *apc,
					  u32 pd, u32 doorbell)
{
	return mana_compat_cfg_vport(apc, pd, doorbell, true);
}

#undef mana_cfg_vport
#define MANA_IB_CFG_VPORT_SELECT(_1, _2, _3, _4, fn, ...) fn
#define mana_cfg_vport(...)						\
	MANA_IB_CFG_VPORT_SELECT(__VA_ARGS__, mana_compat_cfg_vport,	\
				mana_ib_compat_cfg_vport)(__VA_ARGS__)

#endif
