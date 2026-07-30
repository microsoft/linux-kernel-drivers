// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Wrapper TU that compiles driver/mana_ethtool.c against a shimmed view of
 * struct ethtool_ops, so the driver body stays byte-for-byte upstream.
 *
 * This file is the ONLY compilation unit that ever preprocesses
 * driver/mana_ethtool.c. Kbuild lists compat/mana_compat_ethtool_wrapper.o
 * in mana-compat-objs (not driver/mana_ethtool.o).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>

#include "mana_compat_ethtool_shim.h"

/* Rename struct ethtool_ops to our shim so the driver's
 * `struct ethtool_ops mana_ethtool_ops` definition lands in the shim type. */
#define ethtool_ops __mana_ethtool_ops_shim

/* Rename the .get_rxfh / .set_rxfh field designators to trailing shim slots on
 * kernels where the kernel-visible sig differs from the driver's NEW sig. */
#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
#define get_rxfh __get_rxfh_new
#define set_rxfh __set_rxfh_new
#endif

/* Same for ringparam: rename designators when the driver's NEW-sig ops
 * assignment does not match the kernel's field type. */
#if !MANA_HAVE_ETHTOOL_RINGPARAM_EXT
#define get_ringparam __get_ringparam_new
#define set_ringparam __set_ringparam_new
#endif

/* mana.h must see the same renamed types the driver body will use. */
#include <net/mana/mana.h>

/* --- compile the pristine upstream body -------------------------------- */

#include "../driver/mana_ethtool.c"

/* --- OLD-sig rxfh adapters --------------------------------------------- */

#if !MANA_HAVE_ETHTOOL_RXFH_PARAM
int mana_compat_get_rxfh_old(struct net_device *ndev, u32 *indir, u8 *key,
			     u8 *hfunc)
{
	struct ethtool_rxfh_param rxfh = {
		.indir = indir,
		.key   = key,
	};
	int ret = mana_get_rxfh(ndev, &rxfh);

	if (hfunc)
		*hfunc = rxfh.hfunc;
	return ret;
}

int mana_compat_set_rxfh_old(struct net_device *ndev, const u32 *indir,
			     const u8 *key, const u8 hfunc)
{
	struct ethtool_rxfh_param rxfh = {
		.indir = (u32 *)indir,
		.key   = (u8 *)key,
		.hfunc = hfunc,
	};

	return mana_set_rxfh(ndev, &rxfh, NULL);
}
#endif
