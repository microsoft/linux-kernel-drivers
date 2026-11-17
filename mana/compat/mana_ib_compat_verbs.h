/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_VERBS_H
#define MANA_IB_COMPAT_VERBS_H

#include <rdma/ib_verbs.h>

struct ib_dmah;

#if !MANA_IB_HAVE_UDP_SPORT
static inline u16 rdma_get_udp_sport(u32 fl, u32 lqpn, u32 rqpn)
{
	if (!fl)
		fl = rdma_calc_flow_label(lqpn, rqpn);
	return rdma_flow_label_to_udp_sport(fl);
}
#endif

#if !MANA_IB_HAVE_STAT_DESC
struct rdma_stat_desc {
	const char *name;
};

static inline struct rdma_hw_stats *
mana_ib_alloc_hw_stats(const struct rdma_stat_desc *desc, int count,
		      unsigned long lifespan)
{
	/* Name-only descriptors have the old static names array's layout and
	 * lifetime; no temporary name allocation may outlive this call.
	 */
	BUILD_BUG_ON(sizeof(*desc) != sizeof(const char *));
	return rdma_alloc_hw_stats_struct((const char * const *)desc,
					 count, lifespan);
}
#define rdma_alloc_hw_stats_struct mana_ib_alloc_hw_stats
#endif

#endif
