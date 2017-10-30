// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/allocator_shim.h"
#include "base/allocator/allocator_shim_internals.h"
#include "third_party/tcmalloc/chromium/src/config.h"
#include "third_party/tcmalloc/chromium/src/gperftools/tcmalloc.h"

namespace {

using base::allocator::AllocatorDispatch;

void* TCMalloc(const AllocatorDispatch*, size_t size, void* context) {
  return tc_malloc(size);
}

void* TCCalloc(const AllocatorDispatch*, size_t n, size_t size, void* context) {
  return tc_calloc(n, size);
}

void* TCMemalign(const AllocatorDispatch*,
                 size_t alignment,
                 size_t size,
                 void* context) {
  return tc_memalign(alignment, size);
}

void* TCRealloc(const AllocatorDispatch*,
                void* address,
                size_t size,
                void* context) {
  return tc_realloc(address, size);
}

void TCFree(const AllocatorDispatch*, void* address, void* context) {
  tc_free(address);
}

size_t TCGetSizeEstimate(const AllocatorDispatch*,
                         void* address,
                         void* context) {
  return tc_malloc_size(address);
}

}  // namespace

const AllocatorDispatch AllocatorDispatch::default_dispatch = {
    &TCMalloc,          /* alloc_function */
    &TCCalloc,          /* alloc_zero_initialized_function */
    &TCMemalign,        /* alloc_aligned_function */
    &TCRealloc,         /* realloc_function */
    &TCFree,            /* free_function */
    &TCGetSizeEstimate, /* get_size_estimate_function */
    nullptr,            /* batch_malloc_function */
    nullptr,            /* batch_free_function */
    nullptr,            /* free_definite_size_function */
    nullptr,            /* next */
};

// In the case of tcmalloc we have also to route the diagnostic symbols,
// which are not part of the unified shim layer, to tcmalloc for consistency.

extern "C" {

// this bit of magic is used to workaround an issue
// with glibc 2.24 when replacing memory allocators. Jemalloc and tcmalloc
// both rely on providing implementations of malloc (calloc, free, etc.)
// so that the linker drops the need for glib's malloc.o and use
// jemalloc/tcmalloc's implementation instead. However, in glibc 2.24
// fork() now calls internal functions specific to glib's memory allocator.
// The functions below provide empty implementations so that the linker
// continues to drop the need for malloc.o and pickup jemalloc/tcmalloc
// insead. See https://github.com/jemalloc/jemalloc/issues/442#event-840687583
// and https://github.com/gperftools/gperftools/issues/856 for more context.
// Also this was fixed in glibc 2.25 but that is not widely deployed
// https://sourceware.org/bugzilla/show_bug.cgi?id=20432
SHIM_ALWAYS_EXPORT void __malloc_fork_lock_parent(void) {}
SHIM_ALWAYS_EXPORT void __malloc_fork_unlock_parent(void) {}
SHIM_ALWAYS_EXPORT void __malloc_fork_unlock_child(void) {}

SHIM_ALWAYS_EXPORT void malloc_stats(void) __THROW {
  return tc_malloc_stats();
}

SHIM_ALWAYS_EXPORT int mallopt(int cmd, int value) __THROW {
  return tc_mallopt(cmd, value);
}

#ifdef HAVE_STRUCT_MALLINFO
SHIM_ALWAYS_EXPORT struct mallinfo mallinfo(void) __THROW {
  return tc_mallinfo();
}
#endif

SHIM_ALWAYS_EXPORT size_t malloc_size(void* address) __THROW {
  return tc_malloc_size(address);
}

#if defined(__ANDROID__)
SHIM_ALWAYS_EXPORT size_t malloc_usable_size(const void* address) __THROW {
#else
SHIM_ALWAYS_EXPORT size_t malloc_usable_size(void* address) __THROW {
#endif
  return tc_malloc_size(address);
}

}  // extern "C"
