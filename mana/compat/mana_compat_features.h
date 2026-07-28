/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Bracket the required feature-map completeness check. Later shim commits
 * add MANA_HAVE_* requirements here alongside the probes they consume in
 * ../configure, so a build that skips ./configure fails loud instead of
 * treating a missing feature as absent.
 */
#ifndef MANA_COMPAT_FEATURES_H
#define MANA_COMPAT_FEATURES_H

#if !defined(MANA_COMPAT_AUTOCONF_GENERATED)
#error "mana-compat: missing generated feature map; build through the top-level Makefile"
#endif

/* No feature requirements at bootstrap. Each later shim commit adds its
 * MANA_HAVE_* here alongside the probe it registers in ../configure. */

#endif /* MANA_COMPAT_FEATURES_H */
