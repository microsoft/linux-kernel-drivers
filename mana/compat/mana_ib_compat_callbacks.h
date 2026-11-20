/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_CALLBACKS_H
#define MANA_IB_COMPAT_CALLBACKS_H

#if !MANA_IB_CQ_CREATE_ATTRS
static int mana_ib_compat_create_cq(struct ib_cq *cq,
				    const struct ib_cq_init_attr *attr,
				    struct ib_udata *udata)
{
	if (!udata)
		return mana_ib_create_cq(cq, attr, NULL);
	/* Preserve context, ufile, and attributes owned by the original core. */
	return mana_ib_create_cq(cq, attr, rdma_udata_to_uverbs_attr_bundle(udata));
}
#define mana_ib_create_cq mana_ib_compat_create_cq
#endif

#if !MANA_IB_REG_MR_DMAH
static struct ib_mr *mana_ib_compat_reg_user_mr(struct ib_pd *pd, u64 start,
		u64 length, u64 iova, int access, struct ib_udata *udata)
{
	return mana_ib_reg_user_mr(pd, start, length, iova, access, NULL, udata);
}
#define mana_ib_reg_user_mr mana_ib_compat_reg_user_mr
#endif

#if !MANA_IB_DMABUF_DMAH && MANA_IB_DMABUF_ATTRS
static struct ib_mr *mana_ib_compat_reg_dmabuf(struct ib_pd *pd, u64 start,
		u64 length, u64 iova, int fd, int access,
		struct uverbs_attr_bundle *attrs)
{
	return mana_ib_reg_user_mr_dmabuf(pd, start, length, iova, fd, access,
					NULL, attrs);
}
#define mana_ib_reg_user_mr_dmabuf mana_ib_compat_reg_dmabuf
#elif !MANA_IB_DMABUF_DMAH && !MANA_IB_DMABUF_ATTRS
static struct ib_mr *mana_ib_compat_reg_dmabuf(struct ib_pd *pd, u64 start,
		u64 length, u64 iova, int fd, int access, struct ib_udata *udata)
{
	return mana_ib_reg_user_mr_dmabuf(pd, start, length, iova, fd, access,
		NULL, udata ? rdma_udata_to_uverbs_attr_bundle(udata) : NULL);
}
#define mana_ib_reg_user_mr_dmabuf mana_ib_compat_reg_dmabuf
#endif

#endif
