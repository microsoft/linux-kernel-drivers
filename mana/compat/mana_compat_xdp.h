/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * XDP back-compat: xdp_set_features_flag() and the NETDEV_XDP_ACT_* bitmap
 * (v6.3). Before v6.3 there is no xdp_features field on struct net_device,
 * so advertising XDP capabilities is a correct no-op; the bits are provided
 * so the verbatim call expression type-checks and the helper is a stub.
 */
#ifndef MANA_COMPAT_XDP_H
#define MANA_COMPAT_XDP_H

#include <linux/netdevice.h>

#if !MANA_HAVE_XDP_SET_FEATURES_FLAG
#define NETDEV_XDP_ACT_BASIC		1
#define NETDEV_XDP_ACT_REDIRECT		2
#define NETDEV_XDP_ACT_NDO_XMIT		4
#define NETDEV_XDP_ACT_XSK_ZEROCOPY	8
#define NETDEV_XDP_ACT_HW_OFFLOAD	16
#define NETDEV_XDP_ACT_RX_SG		32
#define NETDEV_XDP_ACT_NDO_XMIT_SG	64

static inline void xdp_set_features_flag(struct net_device *dev, u64 val __maybe_unused)
{
	(void)dev;
	(void)val;
}
#endif

#endif /* MANA_COMPAT_XDP_H */
