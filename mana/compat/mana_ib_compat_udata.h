/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_UDATA_H
#define MANA_IB_COMPAT_UDATA_H

#include <rdma/uverbs_ioctl.h>

#ifndef ib_copy_validate_udata_in
static inline int mana_ib_copy_validate_udata(struct ib_udata *udata, void *req,
					     size_t size, size_t minimum)
{
	int err;

	if (udata->inlen < minimum)
		return -EINVAL;
	err = copy_struct_from_user(req, size, udata->inbuf, udata->inlen);
	return err == -E2BIG ? -EOPNOTSUPP : err;
}

#define ib_copy_validate_udata_in(udata, req, last)			\
	mana_ib_copy_validate_udata(udata, &(req), sizeof(req),		\
				   offsetofend(typeof(req), last))
#endif

#endif
