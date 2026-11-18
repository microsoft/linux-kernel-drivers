/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_LEGACY_H
#define MANA_IB_COMPAT_LEGACY_H

#include <net/mana/mana.h>

/* Preserve the original fixed indirection-table limit. */
#ifndef MANA_INDIRECT_TABLE_SIZE
#define MANA_INDIRECT_TABLE_SIZE 64
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
