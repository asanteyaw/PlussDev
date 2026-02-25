#include "CodeM/native/mps/BufferCache.h"

namespace pluss::codem::native::mps {

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
