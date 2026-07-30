/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * PCI back-compat: pci_msix_can_alloc_dyn (v6.2+), pci_msix_alloc_irq_at
 * (v6.2+), pci_msix_free_irq (v6.2+). All added upstream at v6.2 as part of
 * the dynamic MSI-X allocation series.
 */
#ifndef MANA_COMPAT_PCI_H
#define MANA_COMPAT_PCI_H

#include <linux/pci.h>

#if !MANA_HAVE_PCI_MSIX_DYN
struct msi_map { int index; int virq; };

static inline bool pci_msix_can_alloc_dyn(struct pci_dev *dev)
{
	(void)dev;
	return false;
}

static inline struct msi_map pci_msix_alloc_irq_at(struct pci_dev *dev,
	unsigned int index, const struct irq_affinity_desc *aff)
{
	struct msi_map m = { .index = -1, .virq = 0 };
	(void)dev; (void)index; (void)aff;
	return m;
}

static inline void pci_msix_free_irq(struct pci_dev *dev, struct msi_map map)
{
	(void)dev; (void)map;
}
#endif

#endif /* MANA_COMPAT_PCI_H */
