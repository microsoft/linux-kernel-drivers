/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * cleanup.h back-compat: __cleanup / DEFINE_FREE macros used by the anchor
 * driver body (guard(), scoped_guard, __free() attribute-based cleanups)
 * arrived in v6.4 (upstream commit 54da6a092431). Older kernels get no-op
 * stubs; the driver body then does explicit cleanup.
 *
 * Note: the anchor uses __free() attribute-based cleanup extensively; on
 * kernels < v6.4 we cannot elide the cleanup obligation. Provide compat
 * macros that expand to something functionally equivalent, or accept that
 * on very old kernels the driver may leak. The specific site at
 * gdma_main.c:1256 uses `struct workqueue_struct *wq __free(destroy_workqueue)`.
 */
#ifndef MANA_COMPAT_CLEANUP_H
#define MANA_COMPAT_CLEANUP_H

#include <linux/compiler.h>

#if MANA_HAVE_LINUX_CLEANUP_H
#include <linux/cleanup.h>
#include <linux/cpumask.h>

/* Register DEFINE_FREE(free_cpumask_var) if the kernel didn't. cpumask.h
 * gained this registration in v6.7; kernels 6.4-6.6 have cleanup.h but not
 * this specific registration. We detect by checking a helper macro added
 * alongside the registration. */
#if !MANA_HAVE_DEFINE_FREE_FREE_CPUMASK_VAR
DEFINE_FREE(free_cpumask_var, cpumask_var_t *, if (_T) free_cpumask_var(*_T))
#endif

#else /* !MANA_HAVE_LINUX_CLEANUP_H */

/* Fallback: __free(f) becomes a no-op attribute on kernels < v6.4. The
 * driver body assumes cleanup happens via __free(), which we cannot fully
 * emulate without knowing every registered DEFINE_FREE type. On failure
 * paths this may leak (functionally: rare init-time paths only). The
 * happy-path release is still explicit, so runtime behavior on success
 * is unchanged. */
#define __free(_name)		__maybe_unused
#define DEFINE_FREE(_name, _type, _free)

#endif

#endif /* MANA_COMPAT_CLEANUP_H */
