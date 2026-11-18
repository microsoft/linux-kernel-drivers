// SPDX-License-Identifier: GPL-2.0-only
#include <linux/auxiliary_bus.h>
#include "../driver/drivers/infiniband/hw/mana/mana_ib.h"

#if !MANA_IB_CQ_CREATE_ATTRS
static int mana_ib_compat_create_cq(struct ib_cq *cq,
				    const struct ib_cq_init_attr *attr,
				    struct ib_udata *udata)
{
	struct uverbs_attr_bundle attrs = {};
	int ret;

	if (!udata) {
		return mana_ib_create_cq(cq, attr, NULL);
	}
	attrs.driver_udata = *udata;
	ret = mana_ib_create_cq(cq, attr, &attrs);
	*udata = attrs.driver_udata;
	return ret;
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

#if MANA_IB_DMABUF_DMAH
static struct ib_mr *mana_ib_compat_reg_dmabuf(struct ib_pd *pd, u64 start,
		u64 length, u64 iova, int fd, int access, struct ib_dmah *dmah,
		struct uverbs_attr_bundle *attrs)
{
	if (dmah)
		return ERR_PTR(-EOPNOTSUPP);
	return mana_ib_reg_user_mr_dmabuf(pd, start, length, iova, fd, access, attrs);
}
#define mana_ib_reg_user_mr_dmabuf mana_ib_compat_reg_dmabuf
#elif !MANA_IB_DMABUF_ATTRS
static struct ib_mr *mana_ib_compat_reg_dmabuf(struct ib_pd *pd, u64 start,
		u64 length, u64 iova, int fd, int access, struct ib_udata *udata)
{
	struct uverbs_attr_bundle attrs = {};
	struct ib_mr *mr;

	if (udata)
		attrs.driver_udata = *udata;
	mr = mana_ib_reg_user_mr_dmabuf(pd, start, length, iova, fd, access, &attrs);
	if (udata)
		*udata = attrs.driver_udata;
	return mr;
}
#define mana_ib_reg_user_mr_dmabuf mana_ib_compat_reg_dmabuf
#endif

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

