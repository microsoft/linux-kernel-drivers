/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Miscellaneous core-kernel back-compat for the anchor commit.
 */
#ifndef MANA_COMPAT_KERNEL_H
#define MANA_COMPAT_KERNEL_H

#include <linux/skbuff.h>

/*
 * __counted_by(): compiler attribute annotating a flexible-array-member's
 * count field (Clang / GCC support added Clang 15 / GCC 14; kernel adoption
 * v6.6). Purely diagnostic; safe to erase on kernels/toolchains that don't
 * know it.
 */
#ifndef __counted_by
#define __counted_by(m)
#endif

/*
 * PCI_VENDOR_ID_MICROSOFT: added to <linux/pci_ids.h> in v6.1 (upstream
 * commit 1c4f643e92af). Backfill on older kernels so the driver's static
 * PCI ID table compiles.
 */
#ifndef PCI_VENDOR_ID_MICROSOFT
#define PCI_VENDOR_ID_MICROSOFT		0x1414
#endif

/*
 * struct hop_jumbo_hdr / ipv6_has_hopopt_jumbo(): both landed in v5.19
 * (upstream commit d20df70cd12b, "ipv6: net: Add ipv6_has_hopopt_jumbo()"
 * plus the header-inline structure). Anchor driver uses these in
 * mana_start_xmit() to compute TCP header sizes for jumbo grams. Older
 * kernels (5.15) never have IPv6 hop-by-hop jumbogram support, so the
 * fallback safely returns false / provides an unused stub struct.
 */
#if !MANA_HAVE_IPV6_HAS_HOPOPT_JUMBO
struct hop_jumbo_hdr {
	u8	nexthdr;
	u8	hdrlen;
	u8	tlv_type;
	u8	tlv_len;
	__be32	jumbo_payload_len;
};

static inline int ipv6_has_hopopt_jumbo(const struct sk_buff *skb)
{
	(void)skb;
	return 0;
}
#endif

/*
 * dma_set_max_seg_size(): upstream commit 6b1b6bda8a58 (v6.13,
 * "dma-mapping: make dma_set_max_seg_size return void") changed the
 * return type from int to void. Kernels < v6.13 still return int and
 * the anchor driver assigns the result to `err`. Provide a wrapper that
 * discards the value on v6.13+, forwards it on older kernels.
 */
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/cpu.h>

/*
 * ipv6_hopopt_jumbo_remove(): remove hop-by-hop jumbo option from an skb
 * (v6.9+). Fallback to a no-op — the anchor+later driver body uses this to
 * strip the option before an on-wire pass; older kernels never had the
 * jumbogram TX offload path anyway.
 */
#if !MANA_HAVE_IPV6_HOPOPT_JUMBO_REMOVE
static inline int ipv6_hopopt_jumbo_remove(struct sk_buff *skb)
{
	(void)skb;
	return 0;
}
#endif

/*
 * for_each_numa_hop_mask(): iterate CPU masks by NUMA hop distance. Added
 * in v6.2 (upstream commit cc9f4f8b7b52). Anchor+later driver code uses
 * this in gdma_main.c irq_setup for NUMA-aware IRQ distribution. Older
 * kernels (5.15, 6.1) lack it; fall back to a single-hop iteration that
 * degenerates to cpu_online_mask.
 */
#if !MANA_HAVE_FOR_EACH_NUMA_HOP_MASK
#define for_each_numa_hop_mask(mask, node)			\
	for ((mask) = cpu_online_mask, node = -1;		\
	     (mask);						\
	     (mask) = NULL)
#endif

/*
 * skb_tcp_all_headers() / skb_inner_tcp_all_headers(): TCP header-sum
 * helpers added in v5.19 (upstream commit 504148fedb85b). The anchor
 * driver body calls both from mana_start_xmit to compute GSO segment
 * sizes. Older kernels (5.15) never gained these helpers; open-code the
 * pre-v5.19 expression they replaced.
 */
#if !MANA_HAVE_SKB_TCP_ALL_HEADERS
static inline unsigned int skb_tcp_all_headers(const struct sk_buff *skb)
{
	return skb_transport_offset(skb) + tcp_hdrlen(skb);
}

static inline unsigned int skb_inner_tcp_all_headers(const struct sk_buff *skb)
{
	return skb_inner_transport_offset(skb) + inner_tcp_hdrlen(skb);
}
#endif

/*
 * vcalloc(), vfree(): the anchor driver body calls them from hw_channel.c
 * without an explicit <linux/vmalloc.h> include, relying on transitive
 * inclusion via <linux/mm.h> that only holds on kernels < v6.12. Force-
 * include the header here so the calls resolve on every kernel.
 */

/*
 * bpf_warn_invalid_xdp_action(): grew (dev, prog, act) parameters in v6.0
 * (upstream commit c8064e5b4adac5). Kernels < v6.0 take a single (act)
 * argument. The anchor driver body calls the 3-arg form from
 * mana_bpf.c:mana_run_xdp; rewrite to the 1-arg form on old kernels.
 * <linux/filter.h> is included so the real single-argument prototype is
 * established before this rewrite; the rewrite is self-referential, so
 * the inner name resolves to the real function rather than re-expanding.
 */
#include <linux/filter.h>
#if !MANA_HAVE_BPF_WARN_XDP_3ARG
#define bpf_warn_invalid_xdp_action(dev, prog, act) \
	bpf_warn_invalid_xdp_action(act)
#endif

#if MANA_HAVE_DMA_SET_MAX_SEG_SIZE_VOID
#define dma_set_max_seg_size(dev, size) \
	({ (dma_set_max_seg_size)((dev), (size)); 0; })
#endif

#endif /* MANA_COMPAT_KERNEL_H */
