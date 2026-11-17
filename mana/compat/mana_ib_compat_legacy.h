/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_LEGACY_H
#define MANA_IB_COMPAT_LEGACY_H

#include <net/mana/mana.h>

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

static inline struct net_device *
mana_get_primary_netdev_rcu(struct mana_context *ac, u32 port)
{
	RCU_LOCKDEP_WARN(!rcu_read_lock_held(),
			"Taking primary netdev without holding the RCU read lock");
	if (port >= ac->num_ports)
		return NULL;
	if (ac->ports[port]->flags & IFF_SLAVE)
		return netdev_master_upper_dev_get_rcu(ac->ports[port]);
	return ac->ports[port];
}

#endif
