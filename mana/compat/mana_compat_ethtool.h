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

/* struct kernel_ethtool_ringparam: extended ringparam bundle (v5.16). The
 * verbatim get/set_ringparam take it by pointer but never dereference it, so
 * an incomplete type is sufficient where the kernel lacks it; the adapter
 * passes NULL on the old 2-argument prototype.
 */
#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
struct kernel_ethtool_ringparam;
#endif

/* NL_SET_ERR_MSG_FMT: netlink error-message-with-format helper (v6.0+).
 * Falls back to NL_SET_ERR_MSG_MOD on older kernels (drops the format
 * expansion but preserves the message). */
#if !MANA_HAVE_NL_SET_ERR_MSG_FMT
#include <linux/netlink.h>
#define NL_SET_ERR_MSG_FMT(extack, msg, ...) \
	NL_SET_ERR_MSG_MOD(extack, msg)
#endif

/* ethtool_puts(): appends a string to the strings buffer, advancing the
 * cursor (v6.8+). Older kernels expose ethtool_sprintf() which does the
 * same via a printf-style format; "%s" reproduces puts semantics. */
#if !MANA_HAVE_ETHTOOL_PUTS
static inline void ethtool_puts(u8 **data, const char *str)
{
	ethtool_sprintf(data, "%s", str);
}
#endif

#endif /* MANA_COMPAT_ETHTOOL_H */
