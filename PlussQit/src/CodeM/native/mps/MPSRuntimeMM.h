#pragma once

#include "CodeM/native/mps/MPSRuntime.h"

#if defined(__OBJC__)
#import <Metal/Metal.h>

namespace pluss::codem::native::mps {

inline id<MTLDevice> shared_device_mm() {
  return (__bridge id<MTLDevice>)shared_device_handle();
}

inline id<MTLCommandQueue> shared_queue_mm() {
  return (__bridge id<MTLCommandQueue>)shared_queue_handle();
}

inline id<MTLCommandBuffer> create_command_buffer_mm(id<MTLCommandQueue> queue) noexcept {
  return (queue == nil) ? nil : [queue commandBuffer];
}

inline id<MTLComputeCommandEncoder> create_compute_encoder_mm(id<MTLCommandBuffer> cb) noexcept {
  return (cb == nil) ? nil : [cb computeCommandEncoder];
}

inline void submit_command_buffer_mm(id<MTLCommandBuffer> cb, std::int64_t work_items_hint) {
  submit_command_buffer((__bridge void*)cb, work_items_hint);
}

inline void wait_command_buffer_mm(id<MTLCommandBuffer> cb) {
  wait_command_buffer((__bridge void*)cb);
}

} // namespace pluss::codem::native::mps
#endif
