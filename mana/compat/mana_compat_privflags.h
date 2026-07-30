/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Out-of-tree ethtool private-flags control plane (declarations).
 *
 * This is NOT a kernel-gap shim like its mana_compat_*.h siblings: it is an
 * additive, always-present control surface that the out-of-tree module owns.
 * The verbatim driver exposes RX CQE coalescing only through the v7.1 ethtool
 * coalesce uAPI (kernel_ethtool_coalesce::rx_cqe_frames). That uAPI is a
 * netlink-core addition, so it is absent on every kernel below 7.1 and cannot
 * be back-shimmed by an out-of-tree module. This plane provides a stable,
 * version-independent alternate door -- a stock ethtool private flag, the same
 * idiom amzn-drivers' ENA uses for local_page_cache -- that toggles the
 * identical device state (mana_port_context::cqe_coalescing_enable) on every
 * supported kernel (5.15..). It stays present on >=7.1 too, where it simply
 * mirrors the native knob.
 *
 * Declarations only; the definitions live out-of-line in
 * mana_compat_privflags.c so this header can be pulled into the umbrella and
 * force-included into every TU -- ahead of the vendored <net/mana/> headers --
 * without ever needing the driver's private types.
 */
#ifndef MANA_COMPAT_PRIVFLAGS_H
#define MANA_COMPAT_PRIVFLAGS_H

#include <linux/bits.h>
#include <linux/types.h>

struct net_device;

/*
 * The lone out-of-tree private flag: force the device's 4-deep RX CQE
 * coalescing mode (CQE_RX_COALESCED_4). The literal coalescing width is baked
 * into the user-visible name (see mana_compat_privflags.c) and the macro so the
 * flag never reads as the native ethtool `rx-frames` coalesce parameter on the
 * >=7.1 kernels where both controls coexist.
 */
#define MANA_COMPAT_PRIV_FLAG_RX_CQE_COALESCE_4	BIT(0)

int  mana_compat_priv_flags_count(void);
void mana_compat_priv_flags_strings(u8 **data);
u32  mana_compat_get_priv_flags(struct net_device *ndev);
int  mana_compat_set_priv_flags(struct net_device *ndev, u32 flags);

#endif /* MANA_COMPAT_PRIVFLAGS_H */
