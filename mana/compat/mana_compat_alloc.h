/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Typed allocation helpers: kmalloc_obj()/kzalloc_obj() family.
 *
 * The verbatim 7.0 body allocates with the "typed object" macros added to
 * <linux/slab.h> in 7.0. They are variadic: the GFP argument is optional and
 * defaults to GFP_KERNEL. The body uses both forms, e.g.
 *
 *     p   = kzalloc_obj(*p);                          // GFP_KERNEL
 *     w   = kzalloc_obj(*w, GFP_ATOMIC);              // explicit GFP
 *     arr = kmalloc_objs(int, nvec);                  // array, GFP_KERNEL
 *     sgl = kmalloc_objs(struct gdma_sge, n, GFP_ATOMIC);
 *     rxq = kzalloc_flex(*rxq, rx_oobs, count);       // trailing flex array
 *
 * On 7.0+ the real macros exist; the #ifndef guards below leave them untouched
 * (the kernel's variadic versions win, so explicit-GFP call sites still pass
 * their flag through). On older kernels we synthesize equivalents with the same
 * optional-GFP semantics.
 *
 * We intentionally do NOT reuse the upstream internals (default_gfp,
 * struct_size_t, size_mul, __set_flex_counter): they are not all present on the
 * kernels we target. The math here is self-contained and matches the pre-7.0
 * MANA allocation behaviour. GNU typeof() accepts either an expression or a
 * bare type, so typeof(*ptr) and typeof(struct foo) / typeof(int) all work.
 */
#ifndef MANA_COMPAT_ALLOC_H
#define MANA_COMPAT_ALLOC_H

#include <linux/slab.h>

/* Pick the supplied GFP argument, else GFP_KERNEL (mirrors default_gfp()). */
#define __mana_gfp(...)			__mana_gfp_(, ##__VA_ARGS__, GFP_KERNEL)
#define __mana_gfp_(_ignore, _gfp, ...)	(_gfp)

#define __mana_alloc_objs(KMALLOC, GFP, TYPE, COUNT)			\
({									\
	const size_t __mana_n = (size_t)(COUNT);			\
	(TYPE *)KMALLOC(sizeof(TYPE) * __mana_n, (GFP));		\
})

#define __mana_alloc_flex(KMALLOC, GFP, TYPE, FAM, COUNT)		\
({									\
	const size_t __mana_c = (size_t)(COUNT);			\
	const size_t __mana_sz = sizeof(TYPE) +			\
		(sizeof(((TYPE *)0)->FAM[0]) * __mana_c);		\
	(TYPE *)KMALLOC(__mana_sz, (GFP));				\
})

#ifndef kmalloc_obj
#define kmalloc_obj(P, ...) \
	__mana_alloc_objs(kmalloc, __mana_gfp(__VA_ARGS__), typeof(P), 1)
#endif
#ifndef kmalloc_objs
#define kmalloc_objs(P, COUNT, ...) \
	__mana_alloc_objs(kmalloc, __mana_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif
#ifndef kzalloc_obj
#define kzalloc_obj(P, ...) \
	__mana_alloc_objs(kzalloc, __mana_gfp(__VA_ARGS__), typeof(P), 1)
#endif
#ifndef kzalloc_objs
#define kzalloc_objs(P, COUNT, ...) \
	__mana_alloc_objs(kzalloc, __mana_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif
#ifndef kzalloc_flex
#define kzalloc_flex(P, FAM, COUNT, ...) \
	__mana_alloc_flex(kzalloc, __mana_gfp(__VA_ARGS__), typeof(P), FAM, COUNT)
#endif

/*
 * kvzalloc_* / kvmalloc_* typed-object variants. Landed in a later slab.h
 * update than the plain k{m,z}alloc_obj family (used starting from v7.2's
 * driver refresh -- see "net: mana: Use kvmalloc for large RX queue and
 * buffer allocations" and "net: mana: Use per-queue allocation for tx_qp").
 * Same optional-GFP semantics as the plain family; we synthesize with
 * kvmalloc()/kvzalloc(), which every supported kernel provides.
 */
#include <linux/mm.h>			/* kvmalloc / kvzalloc */

#define __mana_kvalloc_objs(KVFN, GFP, TYPE, COUNT)			\
({									\
	const size_t __mana_n = (size_t)(COUNT);			\
	(TYPE *)KVFN(sizeof(TYPE) * __mana_n, (GFP));			\
})

#define __mana_kvalloc_flex(KVFN, GFP, TYPE, FAM, COUNT)		\
({									\
	const size_t __mana_c = (size_t)(COUNT);			\
	const size_t __mana_sz = sizeof(TYPE) +				\
		(sizeof(((TYPE *)0)->FAM[0]) * __mana_c);		\
	(TYPE *)KVFN(__mana_sz, (GFP));					\
})

#ifndef kvmalloc_obj
#define kvmalloc_obj(P, ...) \
	__mana_kvalloc_objs(kvmalloc, __mana_gfp(__VA_ARGS__), typeof(P), 1)
#endif
#ifndef kvmalloc_objs
#define kvmalloc_objs(P, COUNT, ...) \
	__mana_kvalloc_objs(kvmalloc, __mana_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif
#ifndef kvzalloc_obj
#define kvzalloc_obj(P, ...) \
	__mana_kvalloc_objs(kvzalloc, __mana_gfp(__VA_ARGS__), typeof(P), 1)
#endif
#ifndef kvzalloc_objs
#define kvzalloc_objs(P, COUNT, ...) \
	__mana_kvalloc_objs(kvzalloc, __mana_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif
#ifndef kvzalloc_flex
#define kvzalloc_flex(P, FAM, COUNT, ...) \
	__mana_kvalloc_flex(kvzalloc, __mana_gfp(__VA_ARGS__), typeof(P), FAM, COUNT)
#endif

#endif /* MANA_COMPAT_ALLOC_H */
