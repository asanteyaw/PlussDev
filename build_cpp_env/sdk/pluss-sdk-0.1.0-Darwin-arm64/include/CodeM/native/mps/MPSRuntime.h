#pragma once

#include <cstdint>

namespace pluss::codem::native::mps {

// Runtime / availability
bool runtime_available() noexcept;
const char* runtime_error_cstr() noexcept;
void* shared_device_handle() noexcept; // id<MTLDevice>
void* shared_queue_handle() noexcept;  // id<MTLCommandQueue>

// Command-buffer policy.
enum class SyncMode : std::uint8_t {
  Commit,
  CommitAndWait,
  CommitAdaptive,
};

SyncMode sync_mode() noexcept;
void submit_command_buffer(void* command_buffer_handle, std::int64_t work_items_hint) noexcept;
void wait_command_buffer(void* command_buffer_handle) noexcept;

} // namespace pluss::codem::native::mps
