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

#if !defined(MANA_HAVE_EXPORT_SYMBOL_NS_STRING) || \
	!defined(MANA_HAVE_PAGE_POOL_HELPERS) || \
	!defined(MANA_HAVE_PAGE_POOL_PARAMS_NAPI) || \
	!defined(MANA_HAVE_PAGE_POOL_PARAMS_NETDEV) || \
	!defined(MANA_HAVE_PAGE_POOL_FREE_VA) || \
	!defined(MANA_HAVE_PP_FLAG_PAGE_FRAG) || \
	!defined(MANA_HAVE_NETIF_NAPI_ADD_WEIGHT) || \
	!defined(MANA_HAVE_NETIF_NAPI_ADD_TX)
#error "mana-compat: incomplete generated feature map; rerun ./configure"
#endif

#endif /* MANA_COMPAT_FEATURES_H */
