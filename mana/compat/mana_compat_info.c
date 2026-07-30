/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * Out-of-tree packaging metadata for the MANA DKMS build.
 *
 * The captive driver under driver/ carries upstream's MODULE_LICENSE,
 * MODULE_AUTHOR and MODULE_DESCRIPTION verbatim. This small, runtime-silent
 * translation unit adds only the one piece of metadata the in-tree build has no
 * reason to carry: a MODULE_VERSION, so that
 *
 *     modinfo mana-compat | grep ^version
 *
 * reports the DKMS package version (<MANA internal>+linux<source>, e.g.
 * 0.1.1+linux7.1.0~rc7) that this module was packaged at. Keeping it in its own
 * file means the verbatim body is never edited and nothing is printed at load
 * time -- the provenance lives purely in the module's metadata.
 *
 * MANA_COMPAT_VERSION is defined on the compile line by Kbuild (read from
 * dkms.conf). The fallback below only applies if this file is ever built outside
 * that wiring, so the version field is never left empty.
 */
#include <linux/module.h>

#ifndef MANA_COMPAT_VERSION
#define MANA_COMPAT_VERSION "0.0.0+unknown"
#endif

MODULE_VERSION(MANA_COMPAT_VERSION);
