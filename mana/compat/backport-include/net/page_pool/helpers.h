/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Overlay for <net/page_pool/helpers.h>.
 *
 * Resolved ahead of the real kernel header via NOSTDINC_FLAGS so the verbatim
 * driver's `#include <net/page_pool/helpers.h>` (mana_en.c) keeps working
 * everywhere.
 *
 * net/page_pool.h was split into net/page_pool/{types,helpers}.h in v6.6. On
 * 6.6+ we defer to the real helpers.h (which also pulls in types.h, hence
 * struct page_pool_params). On older kernels the single net/page_pool.h carries
 * both the helpers and the params struct.
 *
 * page_pool_free_va() is handled separately: it arrived in v6.12, i.e. after
 * the header split, so its shim is gated only on MANA_HAVE_PAGE_POOL_FREE_VA
 * regardless of which base header was used above.
 *
 * The version-varying page_pool_params fields (.napi @6.6, .netdev/.queue_idx
 * @6.12) and the PP_FLAG_PAGE_FRAG opt-in flag (removed @6.7) are hidden here
 * as well, so the verbatim body of mana_create_page_pool() stays byte-identical
 * to upstream on every supported kernel:
 *
 *   - `struct page_pool_params` is redirected to a superset shim that always
 *     carries every field the driver writes. A static-inline wrapper
 *     `page_pool_create()` translates the shim into the real kernel struct,
 *     wrapping the version-varying assignments in the same MANA_HAVE_* gates
 *     that used to live as marked hunks in the driver body. On kernels where
 *     the wrapper is a straight field copy the compiler elides it.
 *   - PP_FLAG_PAGE_FRAG is set inside the wrapper on pre-6.7 kernels only,
 *     gated on `sp->max_len` — the same signal the verbatim body uses to
 *     indicate a frag pool (`if (rxq->frag_count > 1) pprm.max_len = ...`).
 *     Post-6.7 the flag was removed and fragment support is always-on, so
 *     nothing needs to happen there. The verbatim body never mentions the
 *     macro, so no shim of the macro name itself is required.
 *
 * The `page_pool_params` / `page_pool_create` #define renames are applied at
 * the END of this file, after the wrapper's own definition, so the wrapper
 * itself still resolves to the kernel's real struct and function. They are
 * effective for the rest of the translation unit; no kernel header included
 * by driver/mana_en.c after <net/page_pool/helpers.h> mentions
 * `page_pool_params`, so the rename is contained.
 *
 * Detection uses MANA_HAVE_* (from configure/version), never __has_include:
 * once this overlay dir is on the include path,
 * __has_include(<net/page_pool/helpers.h>) is unconditionally true.
 */
#ifndef MANA_BACKPORT_PAGE_POOL_HELPERS_H
#define MANA_BACKPORT_PAGE_POOL_HELPERS_H

#if MANA_HAVE_PAGE_POOL_HELPERS
#include_next <net/page_pool/helpers.h>
#else
#include <net/page_pool.h>
#endif

#if !MANA_HAVE_PAGE_POOL_FREE_VA
#include <linux/mm.h>		/* virt_to_head_page() */

static inline void page_pool_free_va(struct page_pool *pool, void *va,
				     bool allow_direct)
{
	page_pool_put_full_page(pool, virt_to_head_page(va), allow_direct);
}
#endif

/* page_pool_dma_sync_for_cpu(): sync an Rx page for CPU after DMA write
 * (added later than page_pool_free_va). Used by mana_refill_rx_oob() as of
 * upstream commit "net: mana: Sync page pool RX frags for CPU" (v7.2-rc*).
 * The pool must have been created with PP_FLAG_DMA_MAP (mana always does).
 * Older kernels lack the helper, so fall back to the underlying DMA sync
 * with the page's DMA address; page_pool_get_dma_addr() and
 * page_pool_get_dma_dir() are present on every supported kernel.
 */
#if !MANA_HAVE_PAGE_POOL_DMA_SYNC_FOR_CPU
#include <linux/dma-mapping.h>

static inline void
page_pool_dma_sync_for_cpu(const struct page_pool *pool, const struct page *page,
			   u32 offset, u32 dma_sync_size)
{
	/* On kernels that lack this helper the underlying accessors take a
	 * non-const struct page_pool * and struct page *. Casting away
	 * const is safe -- these accessors read fields only.
	 */
	struct page_pool *_pool = (struct page_pool *)pool;
	struct page *_page = (struct page *)page;

	dma_sync_single_range_for_cpu(_pool->p.dev,
				      page_pool_get_dma_addr(_page),
				      offset + _pool->p.offset,
				      dma_sync_size,
				      page_pool_get_dma_dir(_pool));
}
#endif

/* Superset shim mirroring the field surface of struct page_pool_params across
 * every supported kernel (5.15..7.1). Only the fields the verbatim body reads
 * or writes need to appear here; unlisted kernel fields stay implicit zero via
 * the driver's `= {}` initializer, matching upstream behaviour.
 */
struct mana_compat_page_pool_params {
	unsigned int			flags;
	unsigned int			order;
	unsigned int			pool_size;
	int				nid;
	struct device			*dev;
	struct napi_struct		*napi;		/* real @6.6+ */
	struct net_device		*netdev;	/* real @6.12+ */
	unsigned int			queue_idx;	/* real @6.12+ */
	enum dma_data_direction		dma_dir;
	unsigned int			max_len;
	unsigned int			offset;
};

/* Translate the shim into the kernel-visible struct page_pool_params, gating
 * the version-varying assignments the way the marked hunks used to. Called
 * indirectly via the `#define page_pool_create` below, so the driver body's
 * verbatim `page_pool_create(&pprm)` reaches this without any source change.
 *
 * PP_FLAG_PAGE_FRAG bridging: pre-v6.7 page_pool_alloc_frag() WARNs and
 * returns NULL unless the pool opted in to fragments via this flag; v6.7+
 * removed the flag and made fragment support always-on. The verbatim body
 * signals frag intent by setting .max_len (only the `frag_count > 1` branch
 * sets it), so mirror that condition here to enable the flag exactly when
 * the driver used to, without touching mana_en.c.
 */
static inline struct page_pool *
mana_compat_page_pool_create(const struct mana_compat_page_pool_params *sp)
{
	struct page_pool_params rp = {
		.flags		= sp->flags,
		.order		= sp->order,
		.pool_size	= sp->pool_size,
		.nid		= sp->nid,
		.dev		= sp->dev,
#if MANA_HAVE_PAGE_POOL_PARAMS_NAPI		/* @6.6+ */
		.napi		= sp->napi,
#endif
#if MANA_HAVE_PAGE_POOL_PARAMS_NETDEV		/* @6.12+ */
		.netdev		= sp->netdev,
		.queue_idx	= sp->queue_idx,
#endif
		.dma_dir	= sp->dma_dir,
		.max_len	= sp->max_len,
		.offset		= sp->offset,
	};
#if MANA_HAVE_PP_FLAG_PAGE_FRAG		/* pre-v6.7 only */
	if (rp.max_len)
		rp.flags |= PP_FLAG_PAGE_FRAG;
#endif
	return page_pool_create(&rp);
}

/* Apply the renames LAST so the wrapper above still sees the real kernel
 * types. From here to the end of the TU, `struct page_pool_params` names the
 * shim and `page_pool_create()` calls the wrapper. This is the whole reason
 * the three page-pool marked hunks no longer need to live in mana_en.c.
 */
#define page_pool_params mana_compat_page_pool_params
#define page_pool_create mana_compat_page_pool_create

#endif /* MANA_BACKPORT_PAGE_POOL_HELPERS_H */
