/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Generated feature-map completeness check.
 *
 * Every build must run ./configure against its exact target kernel. If the
 * generator and the glue drift apart, fail rather than treating an unknown
 * feature as absent and risking a redefinition of a distro-backported API.
 */
#ifndef MANA_COMPAT_FEATURES_H
#define MANA_COMPAT_FEATURES_H

/* Transitional anchor-era feature flags (present only across the commits whose
 * driver body still uses these APIs; dropped by the HEAD reconcile). */
#if !defined(MANA_HAVE_NETIF_NAPI_ADD_TX) || \
	!defined(MANA_HAVE_IPV6_HOPOPT_JUMBO_REMOVE)
#error "mana-compat: incomplete transitional feature map; re-run ./configure"
#endif

#if !defined(MANA_COMPAT_AUTOCONF_GENERATED)
#error "mana-compat: missing generated feature map; build through the top-level Makefile"
#endif

#if !defined(MANA_HAVE_EXPORT_SYMBOL_NS_STRING) || \
	!defined(MANA_HAVE_PAGE_POOL_HELPERS) || \
	!defined(MANA_HAVE_PAGE_POOL_PARAMS_NAPI) || \
	!defined(MANA_HAVE_PAGE_POOL_PARAMS_NETDEV) || \
	!defined(MANA_HAVE_PAGE_POOL_FREE_VA) || \
	!defined(MANA_HAVE_PAGE_POOL_DMA_SYNC_FOR_CPU) || \
	!defined(MANA_HAVE_PP_FLAG_PAGE_FRAG) || \
	!defined(MANA_HAVE_NET_SHAPER) || \
	!defined(MANA_HAVE_NETDEV_LOCK) || \
	!defined(MANA_HAVE_NETDEV_LOCK_FN) || \
	!defined(MANA_HAVE_ETHTOOL_RXFH_PARAM) || \
	!defined(MANA_HAVE_ETHTOOL_RINGPARAM_EXT) || \
	!defined(MANA_HAVE_ETHTOOL_PUTS) || \
	!defined(MANA_HAVE_ETHTOOL_OPS_RX_RING_COUNT) || \
	!defined(MANA_HAVE_ETHTOOL_COALESCE_RX_CQE_FRAMES) || \
	!defined(MANA_HAVE_ETHTOOL_OPS_NEEDS_RTNL) || \
	!defined(MANA_HAVE_CPUMASK_WEIGHT_ANDNOT) || \
	!defined(MANA_HAVE_CPUMASK_VAR_CLEANUP) || \
	!defined(MANA_HAVE_NETIF_NAPI_ADD_WEIGHT) || \
	!defined(MANA_HAVE_PCI_MSIX_DYN) || \
	!defined(MANA_HAVE_NETIF_SET_TSO_MAX_SIZE) || \
	!defined(MANA_HAVE_NETDEV_HOLD) || \
	!defined(MANA_HAVE_DISABLE_WORK_SYNC) || \
	!defined(MANA_HAVE_SKB_TCP_ALL_HEADERS) || \
	!defined(MANA_HAVE_XDP_FEATURES) || \
	!defined(MANA_HAVE_BPF_WARN_XDP_3ARG)
#error "mana-compat: incomplete generated feature map; rerun ./configure"
#endif

#endif /* MANA_COMPAT_FEATURES_H */
