/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MANA_IB_COMPAT_ITER_H
#define MANA_IB_COMPAT_ITER_H

#if MANA_IB_HAVE_ITER_HEADER
#include_next <rdma/iter.h>
#else
#include <rdma/ib_verbs.h>
#endif

#endif
