/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Scope-based cleanup class for cpumask_var_t.
 *
 * gdma_main.c's irq_setup() declares:
 *
 *     cpumask_var_t cpus __free(free_cpumask_var);
 *
 * The free_cpumask_var cleanup class was added to <linux/cpumask.h> in v6.12
 * (it pairs __free() with free_cpumask_var()). On older kernels the generic
 * cleanup infrastructure (__free/DEFINE_FREE in <linux/cleanup.h>) is present
 * but this specific class is not, so define it exactly as upstream does. Gated
 * on the feature flag to avoid redefining __free_free_cpumask_var on 6.12+.
 */
#ifndef MANA_COMPAT_CLEANUP_H
#define MANA_COMPAT_CLEANUP_H

#include <linux/cleanup.h>
#include <linux/cpumask.h>

#if !MANA_HAVE_CPUMASK_VAR_CLEANUP
DEFINE_FREE(free_cpumask_var, struct cpumask *, if (_T) free_cpumask_var(_T));
#endif

#endif /* MANA_COMPAT_CLEANUP_H */
