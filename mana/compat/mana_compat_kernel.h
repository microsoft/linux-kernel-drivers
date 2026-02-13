/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Miscellaneous core-kernel back-compat used across the verbatim body.
 *
 * Everything here either:
 *   - fills a macro that older kernels lack (#ifndef guarded, self-correcting),
 *     or
 *   - provides a static-inline fallback gated on a MANA_HAVE_* flag for symbols
 *     that are real functions (where #ifndef cannot detect presence), or
 *   - force-includes a core header the verbatim body needs but does not include
 *     directly (it relied on transitive inclusion that only holds on newer
 *     kernels).
 * None of these override a symbol the target kernel already provides.
 */
#ifndef MANA_COMPAT_KERNEL_H
#define MANA_COMPAT_KERNEL_H

#include <linux/compiler.h>
#include <linux/jiffies.h>
#include <linux/netlink.h>
#include <linux/pci.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/topology.h>
#include <linux/workqueue.h>

/*
 * cpus_read_lock()/cpus_read_unlock(): the verbatim body (gdma_main.c IRQ-setup
 * path) calls these but does not include <linux/cpu.h> directly, relying on
 * transitive inclusion that holds on 7.0 yet not on older kernels (e.g. 5.15,
 * which errors with -Werror=implicit-function-declaration). The symbols exist
 * on every kernel we target, so force the header in -- an include fix, not a
 * feature fallback.
 */
#include <linux/cpu.h>

/* __counted_by(): flexible-array bound annotation. The vendored mana headers
 * use it; it is a no-op attribute, so an empty definition is correct where the
 * kernel/compiler lacks it.
 */
#ifndef __counted_by
#define __counted_by(member)
#endif

/* NL_SET_ERR_MSG_FMT(): formatted extack message (v6.6). On older kernels fall
 * back to the unformatted static-string form; the error code path is preserved,
 * only the human-readable interpolation is dropped. The body always passes a
 * string-literal fmt, so NL_SET_ERR_MSG()'s static buffer is satisfied.
 */
#ifndef NL_SET_ERR_MSG_FMT
#define NL_SET_ERR_MSG_FMT(extack, fmt, ...) NL_SET_ERR_MSG((extack), fmt)
#endif

/* secs_to_jiffies(): macro helper (v6.13). */
#ifndef secs_to_jiffies
#define secs_to_jiffies(_secs) (unsigned long)((_secs) * HZ)
#endif

/* PCI_VENDOR_ID_MICROSOFT: not in older <linux/pci_ids.h>. */
#ifndef PCI_VENDOR_ID_MICROSOFT
#define PCI_VENDOR_ID_MICROSOFT 0x1414
#endif

/* for_each_numa_hop_mask(): NUMA-distance IRQ spreading (v6.6). The fallback
 * performs a single pass over cpu_online_mask, which yields correct (if not
 * NUMA-optimal) IRQ affinity on kernels that predate sched_numa_hop_mask().
 * Mirrors the real macro's (mask, node) shape and for-init declaration.
 */
#ifndef for_each_numa_hop_mask
#define for_each_numa_hop_mask(mask, node)				\
	for (unsigned int __mana_hops = 0;				\
	     __mana_hops == 0 &&					\
	     ((mask) = ((void)(node), cpu_online_mask), true);		\
	     __mana_hops++)
#endif

/* skb_tcp_all_headers()/skb_inner_tcp_all_headers(): header-span helpers
 * (v6.1). Reconstruct from the transport offset + TCP header length.
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

/* disable_work_sync()/disable_delayed_work_sync()/enable_work(): the workqueue
 * "disable counter" API (v6.10). On older kernels map onto cancel_work_sync(),
 * which is exactly what the in-tree MANA driver used before the new API
 * existed; enable_work() becomes a no-op since there is no disable barrier to
 * undo. The body ignores all three return values.
 */
#if !MANA_HAVE_DISABLE_WORK_SYNC
static inline bool disable_work_sync(struct work_struct *work)
{
	return cancel_work_sync(work);
}

static inline bool disable_delayed_work_sync(struct delayed_work *work)
{
	return cancel_delayed_work_sync(work);
}

static inline bool enable_work(struct work_struct *work)
{
	(void)work;
	return true;
}
#endif

#endif /* MANA_COMPAT_KERNEL_H */
