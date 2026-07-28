/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * EXPORT_SYMBOL_NS() suppression + namespace-as-string-literal back-compat.
 *
 * Two concerns handled here:
 *
 * (1) The verbatim 7.0 body exports its inter-module symbols with a
 *     string-literal namespace, e.g. EXPORT_SYMBOL_NS(mana_gd_ring_cq,
 *     "NET_MANA"). Upstream commit cdd30ebb1b9f ("module: Convert symbol
 *     namespace to string literal", v6.13) is the interface boundary: from
 *     6.13 the macro takes the namespace as a string literal; 6.12 and
 *     earlier expect a bare token and apply __stringify() internally,
 *     which would double-stringify our "NET_MANA" to "\"NET_MANA\"".
 *
 * (2) Even on kernels that accept our namespace form, the exports MUST NOT
 *     actually be emitted, for two reasons:
 *       * Modern Azure kernels (Ubuntu, Azure Linux, RHEL) build the in-tree
 *         mana driver as `=y`, so vmlinux already exports the same symbols
 *         (mana_gd_send_request, mana_cfg_vport, mana_create_wq_obj, ...).
 *         An OOT module that tried to export them too fails modpost with
 *         "'X' exported twice. Previous export was in vmlinux".
 *       * The mana-compat module has no external consumers -- it is the
 *         complete Ethernet driver in one .ko, and mana_ib (which would be
 *         the only real consumer) is out of scope and would bind
 *         mana.rdma, not mana_compat.rdma, anyway.
 *
 * Resolution: redefine EXPORT_SYMBOL_NS (and its GPL and PLAIN variants,
 * plus non-namespaced EXPORT_SYMBOL / EXPORT_SYMBOL_GPL) to nothing. The
 * verbatim driver body then compiles unchanged, produces no exports, and
 * cannot collide with any in-vmlinux driver. This preserves the driver's
 * ability to be loaded alongside a built-in mana (once the device is
 * unbound from the kernel driver and rebound to us) without needing the
 * upstream module-loader's blacklist machinery to be effective.
 */
#ifndef MANA_COMPAT_EXPORT_H
#define MANA_COMPAT_EXPORT_H

#include <linux/export.h>
#include <linux/stringify.h>

#undef EXPORT_SYMBOL
#undef EXPORT_SYMBOL_GPL
#undef EXPORT_SYMBOL_NS
#undef EXPORT_SYMBOL_NS_GPL
#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)
#define EXPORT_SYMBOL_NS(sym, ns)
#define EXPORT_SYMBOL_NS_GPL(sym, ns)

#endif /* MANA_COMPAT_EXPORT_H */
