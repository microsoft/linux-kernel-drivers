// SPDX-License-Identifier: GPL-2.0-only
#include <linux/auxiliary_bus.h>
#include "../driver/drivers/infiniband/hw/mana/mana_ib.h"

#include "mana_ib_compat_callbacks.h"

static const struct auxiliary_device_id mana_ib_compat_ids[] = {
	{ .name = "mana_compat.rdma" },
	{ .name = "mana_compat.eth" },
	{}
};
MODULE_DEVICE_TABLE(auxiliary, mana_ib_compat_ids);

static void mana_ib_fixup_driver(struct auxiliary_driver *driver)
{
	driver->id_table = mana_ib_compat_ids;
}

/* Keep the original table for source fidelity but advertise only our pair. */
#pragma push_macro("MODULE_DEVICE_TABLE")
#undef MODULE_DEVICE_TABLE
#define MODULE_DEVICE_TABLE(type, name)
#undef module_auxiliary_driver
#define module_auxiliary_driver(driver)					\
	static int __init mana_ib_compat_init(void)			\
	{								\
		mana_ib_fixup_driver(&(driver));				\
		return auxiliary_driver_register(&(driver));		\
	}								\
	static void __exit mana_ib_compat_exit(void)			\
	{								\
		auxiliary_driver_unregister(&(driver));			\
	}								\
	module_init(mana_ib_compat_init);				\
	module_exit(mana_ib_compat_exit)

#include "../driver/drivers/infiniband/hw/mana/device.c"
#pragma pop_macro("MODULE_DEVICE_TABLE")

