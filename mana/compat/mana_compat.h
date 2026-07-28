/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright (c) 2026, Microsoft Corporation. */
/*
 * MANA out-of-tree compat umbrella.
 *
 * Force-included at the top of every TU via `ccflags-y += -include ...` in
 * Kbuild. The driver body under ../driver/ is a verbatim upstream snapshot;
 * every kernel-version adaptation lives here or in the sibling compat/
 * headers, added by later commits just before the driver commit that needs
 * them.
 */
#ifndef MANA_COMPAT_H
#define MANA_COMPAT_H

#include "autogen/mana_compat_autoconf.h"
#include "mana_compat_features.h"

/* Additional shim headers appended here by future commits, each just before
 * the driver commit that consumes it. */

#endif /* MANA_COMPAT_H */
