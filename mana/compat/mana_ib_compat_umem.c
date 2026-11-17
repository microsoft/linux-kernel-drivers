// SPDX-License-Identifier: GPL-2.0-only
#include "mana_ib_compat_umem.h"

#if !MANA_IB_HAVE_DMABUF_PINNED
#include <linux/dma-buf.h>
#include <linux/dma-resv.h>

#undef ib_umem_release

MODULE_INFO(import_ns, "DMA_BUF");

/* The old core lacks the pinned bit and corresponding release handling. */
static char mana_ib_pinned_marker;

static void mana_ib_pinned_move_notify(struct dma_buf_attachment *attach)
{
	WARN_ON_ONCE(1);
}

static const struct dma_buf_attach_ops mana_ib_pinned_ops = {
	.allow_peer2peer = true,
	.move_notify = mana_ib_pinned_move_notify,
};

void mana_ib_compat_umem_release(struct ib_umem *umem)
{
	if (umem && umem->is_dmabuf) {
		struct ib_umem_dmabuf *buf = to_ib_umem_dmabuf(umem);

		if (buf->private == &mana_ib_pinned_marker) {
			dma_resv_lock(buf->attach->dmabuf->resv, NULL);
			ib_umem_dmabuf_unmap_pages(buf);
			dma_buf_unpin(buf->attach);
			buf->private = NULL;
			dma_resv_unlock(buf->attach->dmabuf->resv);
		}
	}
	ib_umem_release(umem);
}

struct ib_umem_dmabuf *
mana_ib_compat_dmabuf_get_pinned(struct ib_device *device, unsigned long offset,
			       size_t size, int fd, int access)
{
	struct ib_umem_dmabuf *buf;
	int err;

	buf = ib_umem_dmabuf_get(device, offset, size, fd, access,
				&mana_ib_pinned_ops);
	if (IS_ERR(buf))
		return buf;
	dma_resv_lock(buf->attach->dmabuf->resv, NULL);
	err = dma_buf_pin(buf->attach);
	if (!err) {
		buf->private = &mana_ib_pinned_marker;
		err = ib_umem_dmabuf_map_pages(buf);
	}
	dma_resv_unlock(buf->attach->dmabuf->resv);
	if (err) {
		mana_ib_compat_umem_release(&buf->umem);
		return ERR_PTR(err);
	}
	return buf;
}
#endif
