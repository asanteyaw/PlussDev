#include "CodeM/native/mps/MPSAllocator.h"

namespace pluss::codem::native::mps {

void* acquire_pooled_buffer(void* /*device_handle*/, std::size_t /*nbytes*/, const char* /*tag*/) {
  return nullptr;
}

void release_pooled_buffer(void* /*buffer_handle*/) noexcept {}

void defer_release_pooled_buffer(void* /*buffer_handle*/, void* /*command_buffer_handle*/) noexcept {}

void trim_pooled_buffers(std::size_t /*target_cached_bytes*/) noexcept {}

std::size_t pooled_cached_bytes() noexcept {
  return 0;
}

std::size_t pooled_inflight_bytes() noexcept {
  return 0;
}

} // namespace pluss::codem::native::mps
