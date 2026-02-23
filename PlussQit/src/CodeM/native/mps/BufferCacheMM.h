#pragma once

#include "CodeM/native/mps/BufferCache.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

namespace pluss::codem::native::mps {

inline id<MTLDevice> shared_device_mm() {
  return (__bridge id<MTLDevice>)shared_device_handle();
}

inline id<MTLCommandQueue> shared_queue_mm() {
  return (__bridge id<MTLCommandQueue>)shared_queue_handle();
}

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
