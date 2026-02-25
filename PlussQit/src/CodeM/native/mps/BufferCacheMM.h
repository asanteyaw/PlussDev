#pragma once

#include "CodeM/native/mps/BufferCache.h"
#include "CodeM/native/mps/MPSRuntimeMM.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

namespace pluss::codem::native::mps {

inline id<MTLBuffer> ensure_device_buffer_for_read_mm(const SeriesBase& s,
                                                      id<MTLDevice> device,
                                                      std::size_t nbytes) {
  return (__bridge id<MTLBuffer>)ensure_device_buffer_for_read(s, (__bridge void*)device, nbytes);
}

inline id<MTLBuffer> ensure_device_buffer_for_write_mm(const SeriesBase& s,
                                                       id<MTLDevice> device,
                                                       std::size_t nbytes) {
  return (__bridge id<MTLBuffer>)ensure_device_buffer_for_write(s, (__bridge void*)device, nbytes);
}

inline void mark_device_data_dirty_mm(const SeriesBase& s, id<MTLCommandBuffer> cb) {
  mark_device_data_dirty(s, (__bridge void*)cb);
}

} // namespace pluss::codem::native::mps
#endif
