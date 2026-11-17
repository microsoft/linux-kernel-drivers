/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_UDATA_H
#define MANA_IB_COMPAT_UDATA_H

#include <rdma/uverbs_ioctl.h>

#if !MANA_IB_HAVE_EMPTY_INPUT
static inline int ib_is_udata_in_empty(struct ib_udata *udata)
{
	int err;

	if (!udata || !udata->inlen)
		return 0;
	err = copy_struct_from_user(NULL, 0, udata->inbuf, udata->inlen);
	return err == -E2BIG ? -EOPNOTSUPP : err;
}
#endif

#if !MANA_IB_HAVE_EMPTY_OUTPUT
static inline int ib_respond_empty_udata(struct ib_udata *udata)
{
	if (udata && udata->outlen && clear_user(udata->outbuf, udata->outlen))
		return -EFAULT;
	return 0;
}
#endif

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

#ifndef ib_respond_udata
static inline int mana_ib_respond_udata(struct ib_udata *udata,
				       const void *response, size_t size)
{
	size_t len = min(size, udata->outlen);

	if (copy_to_user(udata->outbuf, response, len))
		return -EFAULT;
	if (len < udata->outlen &&
	    clear_user(udata->outbuf + len, udata->outlen - len))
		return -EFAULT;
	return 0;
}

#define ib_respond_udata(udata, response) \
	mana_ib_respond_udata(udata, &(response), sizeof(response))
#endif

#endif
