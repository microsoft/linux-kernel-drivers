// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2026, Microsoft Corporation. */
#include <linux/module.h>

/* Keep the module out of initcall_blacklist=mana_driver_init. */
#define mana_driver_init mana_compat_driver_init

/* module_init() stringifies its argument, so register the renamed init below. */
#pragma push_macro("module_init")
#undef module_init
#define module_init(initfn)
#include "../driver/drivers/net/ethernet/microsoft/mana/gdma_main.c"
#pragma pop_macro("module_init")
#undef mana_driver_init

module_init(mana_compat_driver_init);
