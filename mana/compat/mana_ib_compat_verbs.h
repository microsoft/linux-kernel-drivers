/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_VERBS_H
#define MANA_IB_COMPAT_VERBS_H

#include <rdma/ib_verbs.h>

#if !MANA_IB_HAVE_UDP_SPORT
static inline u16 rdma_get_udp_sport(u32 fl, u32 lqpn, u32 rqpn)
{
	if (!fl)
		fl = rdma_calc_flow_label(lqpn, rqpn);
	return rdma_flow_label_to_udp_sport(fl);
}
#endif

#endif
