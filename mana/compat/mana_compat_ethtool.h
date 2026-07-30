/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * ethtool back-compat: helper types and ethtool_puts().
 *
 * The verbatim mana_ethtool.c is written against the 7.0 ethtool surface. Two
 * of its ops have prototypes that changed shape across kernels (get/set_rxfh
 * and get/set_ringparam); those are bridged by small marked adapter hunks in
 * mana_ethtool.c itself, gated on MANA_HAVE_ETHTOOL_*. This header supplies the
 * *types* those verbatim functions mention so they keep compiling everywhere,
 * plus the ethtool_puts() helper.
 */
#ifndef MANA_COMPAT_ETHTOOL_H
#define MANA_COMPAT_ETHTOOL_H

#include <linux/ethtool.h>

/* struct ethtool_rxfh_param: get/set_rxfh argument bundle (v6.8). Mirror the
 * upstream layout so the verbatim mana_get_rxfh()/mana_set_rxfh() bodies (which
 * touch .hfunc/.indir/.key) compile on kernels that still use the old
 * (indir, key, hfunc) ops prototype. The adapter hunk translates between them.
 */
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
 * verbatim get/set_ringparam() take it by pointer but never dereference it, so
 * an incomplete type is sufficient where the kernel lacks it; the adapter hunk
 * passes NULL on the old 2-argument prototype.
 */
#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
struct kernel_ethtool_ringparam;
#endif

/* ethtool_puts(): append a string to the strings buffer (v6.8). Older kernels
 * provide the printf-style ethtool_sprintf(); "%s" reproduces puts semantics.
 */
#if !MANA_HAVE_ETHTOOL_PUTS
static inline void ethtool_puts(u8 **data, const char *str)
{
	ethtool_sprintf(data, "%s", str);
}
#endif

/* ETHTOOL_OP_NEEDS_RTNL_*: mask bits for struct ethtool_ops::op_needs_rtnl
 * (v7.2). Older kernels have no rtnl-shedding uAPI on the ethtool netlink
 * SET path -- rtnl is always held there -- so these bits are advisory only.
 * The driver body writes them into the (kernel-nonexistent) .op_needs_rtnl
 * field via the wrapper's field-designator rename to a trailing shim slot;
 * the values are never read at runtime, so defining the macros as 0 is
 * correct.
 */
#if !MANA_HAVE_ETHTOOL_OPS_NEEDS_RTNL
#define ETHTOOL_OP_NEEDS_RTNL_LINKSETTINGS	0
#define ETHTOOL_OP_NEEDS_RTNL_SPFLAGS		0
#define ETHTOOL_OP_NEEDS_RTNL_SRINGPARAM	0
#define ETHTOOL_OP_NEEDS_RTNL_SCHANNELS		0
#define ETHTOOL_OP_NEEDS_RTNL_SCOALESCE		0
#define ETHTOOL_OP_NEEDS_RTNL_GPAUSEPARAM	0
#define ETHTOOL_OP_NEEDS_RTNL_SPAUSEPARAM	0
#define ETHTOOL_OP_NEEDS_RTNL_RSS		0
#define ETHTOOL_OP_NEEDS_RTNL_GLINK		0
#endif

#endif /* MANA_COMPAT_ETHTOOL_H */
