#pragma once

#include <cstddef>

namespace pluss::codem::native::mps {

// Acquire a shared-storage Metal buffer from the pool (returns id<MTLBuffer> as void*).
void* acquire_pooled_buffer(void* device_handle, std::size_t nbytes, const char* tag);

// Return a Metal buffer to the reusable pool immediately.
void release_pooled_buffer(void* buffer_handle) noexcept;

// Return a Metal buffer to the pool after command buffer completion.
void defer_release_pooled_buffer(void* buffer_handle, void* command_buffer_handle) noexcept;

// Best-effort trim to target bytes (or env-configured watermark when target is 0).
void trim_pooled_buffers(std::size_t target_cached_bytes = 0) noexcept;

std::size_t pooled_cached_bytes() noexcept;
std::size_t pooled_inflight_bytes() noexcept;

} // namespace pluss::codem::native::mps
