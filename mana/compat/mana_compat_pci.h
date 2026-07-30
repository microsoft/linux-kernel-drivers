/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Dynamic MSI-X back-compat: pci_msix_can_alloc_dyn() / pci_msix_alloc_irq_at()
 * and struct msi_map (v6.2, dynamic MSI-X allocation).
 *
 * gdma_main.c branches on pci_msix_can_alloc_dyn(): when it returns false the
 * driver takes the static "allocate all vectors up front" path, and
 * pci_msix_alloc_irq_at() is never called (it is guarded by the same predicate)
 * but must still compile. So on kernels without the dynamic API we make
 * can_alloc_dyn() return false (selecting the static path, the correct
 * behaviour there) and provide a never-taken alloc stub plus the small struct
 * msi_map the body reads (.virq/.index). All gated together on the feature flag
 * since none of these exist on pre-6.2 kernels.
 */
#ifndef MANA_COMPAT_PCI_H
#define MANA_COMPAT_PCI_H

#include <linux/interrupt.h>
#include <linux/pci.h>

#if !MANA_HAVE_PCI_MSIX_DYN
struct msi_map {
	int index;
	int virq;
};

static inline bool pci_msix_can_alloc_dyn(struct pci_dev *dev)
{
	(void)dev;
	return false;
}

static inline struct msi_map
pci_msix_alloc_irq_at(struct pci_dev *dev, unsigned int index,
		      const struct irq_affinity_desc *affdesc)
{
	struct msi_map map = { .index = -ENOSYS, .virq = 0 };

	(void)dev;
	(void)index;
	(void)affdesc;
	return map;
}

/* pci_msix_free_irq(): companion of pci_msix_alloc_irq_at() -- returns a
 * dynamically-allocated vector to the pool. Only called by the verbatim
 * body from inside branches gated on either pci_msix_can_alloc_dyn()
 * (false above) or an irq_map.virq that came from pci_msix_alloc_irq_at()
 * (which returns .virq = 0 above), so this stub is unreachable at runtime;
 * it exists only so the driver compiles.
 */
static inline void pci_msix_free_irq(struct pci_dev *dev, struct msi_map map)
{
	(void)dev;
	(void)map;
}
#endif

#endif /* MANA_COMPAT_PCI_H */
