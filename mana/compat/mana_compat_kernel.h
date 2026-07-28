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
#if MANA_HAVE_DMA_SET_MAX_SEG_SIZE_VOID
#define dma_set_max_seg_size(dev, size) \
	({ (dma_set_max_seg_size)((dev), (size)); 0; })
#endif

#endif /* MANA_COMPAT_KERNEL_H */
