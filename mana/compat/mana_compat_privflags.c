// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Out-of-tree ethtool private-flags control plane (definitions).
 *
 * See mana_compat_privflags.h for the rationale. In short: this is the
 * version-independent door to RX CQE coalescing on kernels where the native
 * v7.1 ethtool coalesce uAPI does not exist (every supported kernel below 7.1)
 * and cannot be shimmed (it is a netlink-core attribute, not an in-kernel
 * inline). It toggles exactly the same device state the native knob would --
 * mana_port_context::cqe_coalescing_enable -- and is wired into the captive
 * ethtool ops via small marked hunks in driver/mana_ethtool.c.
 *
 * This lives in its own translation unit so it can include the vendored driver
 * headers (for mana_port_context and mana_config_rss). The umbrella header,
 * which is force-included into every TU before <net/mana/mana.h>, only ever
 * sees the declarations in mana_compat_privflags.h.
 */
#include <linux/errno.h>
#include <linux/ethtool.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/types.h>

#include <net/mana/mana.h>

#include "mana_compat_privflags.h"

/*
 * Names reported by `ethtool --show-priv-flags`. The array index is the flag's
 * bit position, so this table is the single source of truth for both the
 * MANA_COMPAT_PRIV_FLAG_* bit values and the strings.
 */
static const char mana_compat_priv_flag_strings[][ETH_GSTRING_LEN] = {
	[0] = "rx_cqe_coalesce_4",
};

#define MANA_COMPAT_PRIV_FLAGS_NR ARRAY_SIZE(mana_compat_priv_flag_strings)

int mana_compat_priv_flags_count(void)
{
	return MANA_COMPAT_PRIV_FLAGS_NR;
}

void mana_compat_priv_flags_strings(u8 **data)
{
	unsigned int i;

	for (i = 0; i < MANA_COMPAT_PRIV_FLAGS_NR; i++)
		ethtool_puts(data, mana_compat_priv_flag_strings[i]);
}

u32 mana_compat_get_priv_flags(struct net_device *ndev)
{
	struct mana_port_context *apc = netdev_priv(ndev);

	return apc->cqe_coalescing_enable ?
		MANA_COMPAT_PRIV_FLAG_RX_CQE_COALESCE_4 : 0;
}

/*
 * Mirror mana_set_coalesce()'s device-reprogram + rollback dance: stash the old
 * value, apply the new one, and only push it to the hardware when the port is
 * up (RSS config carries the coalescing bit). Restore on failure. Unknown bits
 * are rejected; an unchanged request is a no-op so we never reconfigure RSS for
 * nothing.
 */
int mana_compat_set_priv_flags(struct net_device *ndev, u32 flags)
{
	struct mana_port_context *apc = netdev_priv(ndev);
	u8 saved_cqe_coalescing_enable;
	bool want;
	int err;

	if (flags & ~MANA_COMPAT_PRIV_FLAG_RX_CQE_COALESCE_4)
		return -EINVAL;

	want = !!(flags & MANA_COMPAT_PRIV_FLAG_RX_CQE_COALESCE_4);
	if (want == !!apc->cqe_coalescing_enable)
		return 0;

	saved_cqe_coalescing_enable = apc->cqe_coalescing_enable;
	apc->cqe_coalescing_enable = want;

	if (!apc->port_is_up)
		return 0;

	err = mana_config_rss(apc, TRI_STATE_TRUE, false, false);
	if (err)
		apc->cqe_coalescing_enable = saved_cqe_coalescing_enable;

	return err;
}
