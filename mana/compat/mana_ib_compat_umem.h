/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_UMEM_H
#define MANA_IB_COMPAT_UMEM_H

#include <rdma/ib_umem.h>

#if !MANA_IB_HAVE_UMEM_GET_VA
#define ib_umem_get_va ib_umem_get
#endif

#if !MANA_IB_HAVE_DMABUF_PINNED
struct ib_umem_dmabuf *
mana_ib_compat_dmabuf_get_pinned(struct ib_device *device, unsigned long offset,
			       size_t size, int fd, int access);
void mana_ib_compat_umem_release(struct ib_umem *umem);
#define ib_umem_dmabuf_get_pinned mana_ib_compat_dmabuf_get_pinned
#define ib_umem_release mana_ib_compat_umem_release
#endif

#endif
