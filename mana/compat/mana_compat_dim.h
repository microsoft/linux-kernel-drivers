/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Dynamic Interrupt Moderation (DIM) shims.
 *
 * The captive driver drives net_dim() for adaptive RX/TX interrupt
 * moderation. Two things about that library vary across the kernels we
 * support, and neither is expressible from the driver body:
 *
 *   1. lib/dim is a Kconfig-gated facility (CONFIG_DIMLIB). The in-tree
 *      driver simply `select DIMLIB`s it; an out-of-tree module cannot
 *      select a Kconfig symbol, and on a target kernel built without it
 *      net_dim() & friends are not in vmlinux at all -- the module would
 *      fail modpost with "Unknown symbol". ./configure probes the target
 *      kernel's CONFIG_DIMLIB and we degrade gracefully when it is off:
 *      the net_dim() entry points become inert stubs, so the driver keeps
 *      *static* interrupt moderation (which is pure doorbell traffic and
 *      needs no library) and only the adaptive mode is unavailable. The
 *      register_netdev interceptor in mana_compat_ethtool_ops.c withdraws
 *      the ETHTOOL_COALESCE_USE_ADAPTIVE_{RX,TX} advertisement and clears
 *      apc->{rx,tx}_dim_enabled so the state the driver reports back
 *      through `ethtool -c` is truthful rather than a silent no-op.
 *
 *   2. net_dim() took its end sample BY VALUE until v6.13 (commit
 *      61bf0009a765 "dim: pass dim_sample to net_dim() by reference"),
 *      which switched the parameter to a const pointer. The driver body
 *      uses the new pointer form; on older kernels we adapt it back.
 *
 * <linux/dim.h> is pulled in FIRST so the real declarations are captured
 * before the macro renames below are installed -- otherwise the renames
 * would rewrite the prototypes themselves.
 */
#ifndef MANA_COMPAT_DIM_H
#define MANA_COMPAT_DIM_H

#include <linux/dim.h>
#include <linux/types.h>

#if !MANA_HAVE_DIMLIB

/*
 * CONFIG_DIMLIB=n: no net_dim symbols exist on this kernel. Provide inert
 * stubs so the verbatim body still links. They are never reached at
 * runtime either -- mana_update_{rx,tx}_dim() bail out on the
 * apc->{rx,tx}_dim_enabled flags, which the register_netdev interceptor
 * forces to false on such kernels.
 */
static inline void mana_compat_net_dim(struct dim *dim,
				       const struct dim_sample *end_sample)
{
}

static inline struct dim_cq_moder
mana_compat_net_dim_get_rx_moderation(u8 cq_period_mode, int ix)
{
	struct dim_cq_moder moder = {};

	return moder;
}

static inline struct dim_cq_moder
mana_compat_net_dim_get_tx_moderation(u8 cq_period_mode, int ix)
{
	struct dim_cq_moder moder = {};

	return moder;
}

#define net_dim				mana_compat_net_dim
#define net_dim_get_rx_moderation	mana_compat_net_dim_get_rx_moderation
#define net_dim_get_tx_moderation	mana_compat_net_dim_get_tx_moderation

#elif !MANA_HAVE_NET_DIM_SAMPLE_PTR

/*
 * Pre-v6.13 net_dim() takes the end sample by value. Dereference at the
 * call site; the sample the driver passes is always an initialised
 * stack object, so this is a plain copy, not a lifetime change.
 */
static inline void mana_compat_net_dim(struct dim *dim,
				       const struct dim_sample *end_sample)
{
	net_dim(dim, *end_sample);
}

#define net_dim mana_compat_net_dim

#endif /* !MANA_HAVE_DIMLIB / !MANA_HAVE_NET_DIM_SAMPLE_PTR */

#endif /* MANA_COMPAT_DIM_H */
