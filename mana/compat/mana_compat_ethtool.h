/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * ethtool back-compat: types the verbatim mana_ethtool.c mentions but the
 * target kernel may lack.
 *
 * At the anchor commit the only such type is struct ethtool_rxfh_param
 * (added upstream in v6.8). Later shim commits extend this header when
 * the driver adopts other later-arriving types (kernel_ethtool_ringparam
 * @ upstream #126, ethtool_puts @ #136, ETHTOOL_OP_NEEDS_RTNL_* @ #254).
 */
#ifndef MANA_COMPAT_ETHTOOL_H
#define MANA_COMPAT_ETHTOOL_H

#include <linux/ethtool.h>

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
struct ethtool_rxfh_param {
	u8	hfunc;
	u32	indir_size;
	u32	*indir;
	u32	key_size;
	u8	*key;
	u32	rss_context;
	u8	rss_delete;
	u8	input_xfrm;
};
#endif

#endif /* MANA_COMPAT_ETHTOOL_H */
