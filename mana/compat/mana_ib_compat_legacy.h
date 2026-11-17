/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_LEGACY_H
#define MANA_IB_COMPAT_LEGACY_H

#include <net/mana/mana.h>

typedef u64 gdma_obj_handle_t;

/* Preserve the limits used by the original 0266a177631d driver. */
#ifndef MAX_SEND_BUFFERS_PER_QUEUE
#define MAX_SEND_BUFFERS_PER_QUEUE 256
#endif
#ifndef MANA_INDIRECT_TABLE_SIZE
#define MANA_INDIRECT_TABLE_SIZE 64
#endif

/* V1 wire layout, removed from Ethernet headers after the V2 transition. */
struct mana_cfg_rx_steer_req {
	struct gdma_req_hdr hdr;
	mana_handle_t vport;
	u16 num_indir_entries;
	u16 indir_tab_offset;
	u32 rx_enable;
	u32 rss_enable;
	u8 update_default_rxobj;
	u8 update_hashkey;
	u8 update_indir_tab;
	u8 reserved;
	mana_handle_t default_rxobj;
	u8 hashkey[MANA_HASH_KEY_SIZE];
};

#if MANA_IB_SOURCE_RX_STEER_V1
struct mana_ib_v1_rx_steer_resp {
	struct gdma_resp_hdr hdr;
};
#define mana_cfg_rx_steer_resp mana_ib_v1_rx_steer_resp
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
