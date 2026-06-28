#pragma once

#include "CodeM/native/mps/MPSAllocator.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

namespace pluss::codem::native::mps {

inline id<MTLBuffer> acquire_pooled_buffer_mm(id<MTLDevice> device, std::size_t nbytes, const char* tag) {
  return (__bridge_transfer id<MTLBuffer>)acquire_pooled_buffer((__bridge void*)device, nbytes, tag);
}

inline void release_pooled_buffer_mm(id<MTLBuffer> buffer) noexcept {
  release_pooled_buffer((__bridge void*)buffer);
}

inline void defer_release_pooled_buffer_mm(id<MTLBuffer> buffer, id<MTLCommandBuffer> cb) noexcept {
  defer_release_pooled_buffer((__bridge void*)buffer, (__bridge void*)cb);
}

} // namespace pluss::codem::native::mps
#endif
