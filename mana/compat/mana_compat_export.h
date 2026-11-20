/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Always export the paired RDMA interface under private mana_compat_* names.
 * Renaming avoids collisions with the in-tree MANA driver's exports, including
 * when that driver is built into vmlinux. Ethernet-only builds expose the same
 * interface so the paired mana_ib can be built afterward.
 *
 * Map namespaced exports to plain GPL exports for these private names. This
 * also avoids the namespace macro's token-to-string-literal API change across
 * supported kernels; the paired driver does not import NET_MANA.
 */
#ifndef MANA_COMPAT_EXPORT_H
#define MANA_COMPAT_EXPORT_H

#include <linux/export.h>
#include <linux/stringify.h>

#include "mana_ib_compat_exports.h"
#undef EXPORT_SYMBOL_NS
#undef EXPORT_SYMBOL_NS_GPL
#define EXPORT_SYMBOL_NS(sym, ns) EXPORT_SYMBOL_GPL(sym)
#define EXPORT_SYMBOL_NS_GPL(sym, ns) EXPORT_SYMBOL_GPL(sym)
#endif /* MANA_COMPAT_EXPORT_H */
