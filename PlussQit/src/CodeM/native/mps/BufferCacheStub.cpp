#include "CodeM/native/mps/BufferCache.h"

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

void sync_host_if_needed(const SeriesBase&) {}

void mark_host_data_dirty(const SeriesBase&) noexcept {}

void* ensure_device_buffer_for_read(const SeriesBase&, void*, std::size_t) {
  return nullptr;
}

void* ensure_device_buffer_for_write(const SeriesBase&, void*, std::size_t) {
  return nullptr;
}

void mark_device_data_dirty(const SeriesBase&, void*) noexcept {}

void erase_series_entry(const SeriesImpl*) noexcept {}

} // namespace pluss::codem::native::mps
