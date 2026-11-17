// SPDX-License-Identifier: GPL-2.0-only
#include <linux/auxiliary_bus.h>
#include "../driver/drivers/infiniband/hw/mana/mana_ib.h"

#if MANA_IB_CQ_CREATE_ATTRS
static int mana_ib_compat_create_cq(struct ib_cq *cq,
				    const struct ib_cq_init_attr *attr,
				    struct uverbs_attr_bundle *attrs)
{
	return mana_ib_create_cq(cq, attr, attrs ? &attrs->driver_udata : NULL);
}
#define mana_ib_create_cq mana_ib_compat_create_cq
#endif

#if MANA_IB_REG_MR_DMAH
static struct ib_mr *mana_ib_compat_reg_user_mr(struct ib_pd *pd, u64 start,
		u64 length, u64 iova, int access, struct ib_dmah *dmah,
		struct ib_udata *udata)
{
	if (dmah)
		return ERR_PTR(-EOPNOTSUPP);
	return mana_ib_reg_user_mr(pd, start, length, iova, access, udata);
}
#define mana_ib_reg_user_mr mana_ib_compat_reg_user_mr
#endif

static const struct auxiliary_device_id mana_ib_compat_ids[] = {
	{ .name = "mana_compat.rdma" },
#if MANA_IB_HAVE_ETH_AUX
	{ .name = "mana_compat.eth" },
#endif
	{}
};
MODULE_DEVICE_TABLE(auxiliary, mana_ib_compat_ids);

/* Keep the original table for source fidelity but advertise only our pair. */
#pragma push_macro("MODULE_DEVICE_TABLE")
#undef MODULE_DEVICE_TABLE
#define MODULE_DEVICE_TABLE(type, name)
#undef module_auxiliary_driver
#define module_auxiliary_driver(driver)					\
	static int __init mana_ib_compat_init(void)			\
	{								\
		(driver).id_table = mana_ib_compat_ids;			\
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
