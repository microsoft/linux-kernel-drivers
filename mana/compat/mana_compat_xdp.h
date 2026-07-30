/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * XDP back-compat: bpf_warn_invalid_xdp_action() arity and the XDP feature
 * advertisement API. The core XDP datapath helpers the body uses
 * (xdp_init_buff, xdp_prepare_buff, xdp_do_flush, xdp_do_redirect) are present
 * on every supported kernel and need no shimming.
 */
#ifndef MANA_COMPAT_XDP_H
#define MANA_COMPAT_XDP_H

#include <linux/filter.h>
#include <linux/netdevice.h>

/* bpf_warn_invalid_xdp_action(): gained (dev, prog, act) parameters in v6.0;
 * older kernels take just (act). <linux/filter.h> is included above so the real
 * single-argument prototype is established before this rewrite; the rewrite is
 * self-referential, so the inner name resolves to that real function rather
 * than re-expanding the macro.
 */
#if !MANA_HAVE_BPF_WARN_XDP_3ARG
#define bpf_warn_invalid_xdp_action(dev, prog, act) \
	bpf_warn_invalid_xdp_action(act)
#endif

/* XDP feature advertisement (v6.3): net_device.xdp_features plus
 * xdp_set_features_flag() and the NETDEV_XDP_ACT_* bitmap. Before 6.3 there is
 * no xdp_features field, so advertising is a correct no-op; the bits are
 * provided only so the verbatim call expression type-checks.
 */
#if !MANA_HAVE_XDP_FEATURES
#define NETDEV_XDP_ACT_BASIC		1
#define NETDEV_XDP_ACT_REDIRECT		2
#define NETDEV_XDP_ACT_NDO_XMIT		4
#define NETDEV_XDP_ACT_XSK_ZEROCOPY	8
#define NETDEV_XDP_ACT_HW_OFFLOAD	16
#define NETDEV_XDP_ACT_RX_SG		32
#define NETDEV_XDP_ACT_NDO_XMIT_SG	64

static inline void xdp_set_features_flag(struct net_device *dev, u64 val)
{
	(void)dev;
	(void)val;
}
#endif

#endif /* MANA_COMPAT_XDP_H */
