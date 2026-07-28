/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * cpumask back-compat: cpumask_weight_andnot (v6.3+) used in gdma_main.c.
 */
#ifndef MANA_COMPAT_CPUMASK_H
#define MANA_COMPAT_CPUMASK_H

#include <linux/cpumask.h>

#if !MANA_HAVE_CPUMASK_WEIGHT_ANDNOT
static inline unsigned int
cpumask_weight_andnot(const struct cpumask *src1p, const struct cpumask *src2p)
{
	cpumask_var_t tmp;
	unsigned int n;

	if (!alloc_cpumask_var(&tmp, GFP_KERNEL))
		return 0;
	cpumask_andnot(tmp, src1p, src2p);
	n = cpumask_weight(tmp);
	free_cpumask_var(tmp);
	return n;
}
#endif

#endif /* MANA_COMPAT_CPUMASK_H */
