#pragma once

#include "CodeM/core/SeriesBase.h"
#include "CodeM/native/mps/MPSRuntime.h"

#include <cstddef>

namespace pluss::codem::native::mps {

// Host/device synchronization hooks used by SeriesBase raw-data accessors.
void sync_host_if_needed(const SeriesBase& s);
void mark_host_data_dirty(const SeriesBase& s) noexcept;

// ObjC++ call sites pass native Metal handles as opaque pointers.
// device_handle: id<MTLDevice>, command_buffer_handle: id<MTLCommandBuffer>,
// return value: id<MTLBuffer>.
void* ensure_device_buffer_for_read(const SeriesBase& s, void* device_handle, std::size_t nbytes);
void* ensure_device_buffer_for_write(const SeriesBase& s, void* device_handle, std::size_t nbytes);
void mark_device_data_dirty(const SeriesBase& s, void* command_buffer_handle) noexcept;

// Lifecycle hook used by SeriesImpl dtor to avoid stale cache entries.
void erase_series_entry(const SeriesImpl* impl) noexcept;

} // namespace pluss::codem::native::mps
