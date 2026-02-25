#include "CodeM/native/mps/MPSRuntime.h"

namespace pluss::codem::native::mps {

bool runtime_available() noexcept {
  return false;
}

const char* runtime_error_cstr() noexcept {
  return "MPS backend unavailable on this platform/build";
}

void* shared_device_handle() noexcept {
  return nullptr;
}

void* shared_queue_handle() noexcept {
  return nullptr;
}

SyncMode sync_mode() noexcept {
  return SyncMode::CommitAdaptive;
}

void submit_command_buffer(void*, std::int64_t) noexcept {}

void wait_command_buffer(void*) noexcept {}

} // namespace pluss::codem::native::mps
