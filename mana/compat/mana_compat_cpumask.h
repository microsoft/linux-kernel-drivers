/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * cpumask_weight_andnot(): weight of (src1 & ~src2).
 *
 * Added to <linux/cpumask.h> in v6.9 (commit c1f5204efcbc) and used by
 * gdma_main.c's irq_setup(). Gated on the feature flag (it is a real function,
 * so #ifndef cannot detect it). The fallback counts directly with the most
 * basic cpumask primitives — no allocation, no large stack mask — which is
 * adequate for the cold IRQ-setup path.
 */
#ifndef MANA_COMPAT_CPUMASK_H
#define MANA_COMPAT_CPUMASK_H

#include <linux/cpumask.h>

#if !MANA_HAVE_CPUMASK_WEIGHT_ANDNOT
static inline unsigned int cpumask_weight_andnot(const struct cpumask *src1,
						 const struct cpumask *src2)
{
	unsigned int weight = 0;
	int cpu;

	for_each_cpu(cpu, src1)
		if (!cpumask_test_cpu(cpu, src2))
			weight++;

	return weight;
}
#endif

#endif /* MANA_COMPAT_CPUMASK_H */
