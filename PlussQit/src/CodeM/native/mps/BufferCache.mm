#include "CodeM/native/mps/BufferCache.h"

#include "CodeM/core/Errors.h"
#include "CodeM/native/mps/MPSAllocatorMM.h"
#include "CodeM/native/mps/MPSRuntime.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pluss::codem::native::mps {
namespace {

struct CacheEntry {
  id<MTLBuffer> buffer{nil};
  std::size_t nbytes{0};
  bool host_dirty{true};
  bool device_dirty{false};
  bool device_valid{false};
  id<MTLCommandBuffer> pending{nil};
};

std::mutex& cache_mu_() {
  static std::mutex mu;
  return mu;
}

std::unordered_map<const SeriesImpl*, CacheEntry>& cache_() {
  static std::unordered_map<const SeriesImpl*, CacheEntry> map;
  return map;
}
// A superset of the entries whose device copy is newer than their host copy: filled when an entry is marked dirty,
// pruned lazily when the scan finds it clean. Lets the view write-back look only at these instead of the whole cache.
std::unordered_set<const SeriesImpl*>& maybe_dirty_() {
  static std::unordered_set<const SeriesImpl*> set;
  return set;
}

const SeriesImpl* impl_of_(const SeriesBase& s) noexcept {
  return s.unsafe_impl();
}

std::byte* host_ptr_mut_(const SeriesBase& s) noexcept {
  const auto* impl = s.unsafe_impl();
  return (impl == nullptr) ? nullptr : const_cast<std::byte*>(impl->raw_data());
}

const std::byte* host_ptr_const_(const SeriesBase& s) noexcept {
  const auto* impl = s.unsafe_impl();
  return (impl == nullptr) ? nullptr : impl->raw_data();
}

void wait_pending_(CacheEntry& e);
// A view (slice, narrow, reshape of a slice, ...) is its own Series handle with its own cache entry but shares host storage
// with its base. If the base was last written on the device its host bytes are stale, so a view uploading from host would
// read garbage. Before any host -> device upload, write back every other device-dirty entry whose host range overlaps it.
// Called with cache_mu_() held.
void sync_overlapping_dirty_entries_(const CacheEntry* self, const std::byte* host, std::size_t nbytes) {
  auto& dirty = maybe_dirty_();
  for (auto it = dirty.begin(); it != dirty.end();) {
    const SeriesImpl* impl = *it;
    auto found = cache_().find(impl);
    if (found == cache_().end() || !found->second.device_dirty) {
      it = dirty.erase(it);
      continue;
    }
    ++it;
    CacheEntry& other = found->second;
    if (&other == self || other.buffer == nil || other.nbytes == 0) {
      continue;
    }
    const std::byte* other_host = impl->raw_data();
    if (other_host == nullptr) {
      continue;
    }
    const bool overlaps = other_host < host + nbytes && host < other_host + other.nbytes;
    if (!overlaps) {
      continue;
    }
    wait_pending_(other);
    std::memcpy(const_cast<std::byte*>(other_host), [other.buffer contents], other.nbytes);
    other.device_dirty = false;
    other.host_dirty = false;
    other.device_valid = true;
  }
}

void wait_pending_(CacheEntry& e) {
  if (e.pending != nil) {
    [e.pending waitUntilCompleted];
    e.pending = nil;
  }
}

void release_entry_buffer_(CacheEntry& e) {
  if (e.buffer != nil) {
    // e.pending only covers this entry's own last WRITE -- it says nothing
    // about later ops that read this same buffer as an input afterward
    // (ensure_device_buffer_for_read hands out the buffer without recording
    // who reads it). Those reads are still real in-flight GPU work, and
    // this buffer is about to go back into the shared pool for a totally
    // unrelated allocation, which will memcpy new data into it from the
    // CPU. Draining the whole queue here -- cheap, since a single MTLCommand
    // Queue completes buffers in commit order, so waiting on the latest one
    // proves everything earlier (including those reads) has also finished --
    // is what actually closes that race, not just waiting on e.pending.
    wait_for_all_submitted_work();
    release_pooled_buffer_mm(e.buffer);
    e.buffer = nil;
    e.nbytes = 0;
    e.device_valid = false;
    e.device_dirty = false;
  }
}

} // namespace

void sync_host_if_needed(const SeriesBase& s) {
  if (!s.defined() || s.platform() != Platform::MPS) {
    return;
  }
  std::scoped_lock lock(cache_mu_());
  auto it = cache_().find(impl_of_(s));
  if (it == cache_().end()) {
    return;
  }
  auto& e = it->second;
  wait_pending_(e);
  if (!e.device_dirty || e.buffer == nil || e.nbytes == 0) {
    return;
  }
  auto* host = host_ptr_mut_(s);
  if (host == nullptr) {
    throw_error("MPS cache sync: null host storage");
  }
  std::memcpy(host, [e.buffer contents], e.nbytes);
  e.device_dirty = false;
  e.host_dirty = false;
  e.device_valid = true;
}

void mark_host_data_dirty(const SeriesBase& s) noexcept {
  if (!s.defined() || s.platform() != Platform::MPS) {
    return;
  }
  std::scoped_lock lock(cache_mu_());
  auto it = cache_().find(impl_of_(s));
  if (it == cache_().end()) {
    return;
  }
  auto& e = it->second;
  wait_pending_(e);
  e.host_dirty = true;
  e.device_dirty = false;
}

void* ensure_device_buffer_for_read(const SeriesBase& s, void* device_handle, std::size_t nbytes) {
  if (!s.defined() || s.platform() != Platform::MPS) {
    throw_error("MPS cache read: expected defined MPS Series");
  }
  if (!runtime_available()) {
    throw_error(std::string("MPS cache read: ") + runtime_error_cstr());
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_handle;
  if (device == nil) {
    throw_error("MPS cache read: null MTLDevice");
  }

  const auto bytes = std::max<std::size_t>(nbytes, 1);
  std::scoped_lock lock(cache_mu_());
  auto& e = cache_()[impl_of_(s)];

  if (e.buffer == nil || e.nbytes != bytes) {
    wait_pending_(e);
    release_entry_buffer_(e);
    e.buffer = acquire_pooled_buffer_mm(device, bytes, "series_read");
    if (e.buffer == nil) {
      throw_error("MPS cache read: failed to allocate MTLBuffer");
    }
    e.nbytes = bytes;
  }

  if (e.host_dirty || !e.device_valid) {
    const auto* host = host_ptr_const_(s);
    if (host == nullptr) {
      throw_error("MPS cache read: null host storage");
    }
    // Only a handle whose storage another handle also holds (a view or alias) can be stale because of a device write made
    // through that other handle; a sole owner's host bytes are only ever written through its own cache entry.
    if (s.unsafe_impl()->storage().handle().use_count() > 2) {
      sync_overlapping_dirty_entries_(&e, host, bytes);
    }
    std::memcpy([e.buffer contents], host, bytes);
    e.host_dirty = false;
    e.device_dirty = false;
    e.device_valid = true;
  }

  return (__bridge void*)e.buffer;
}

void* ensure_device_buffer_for_write(const SeriesBase& s, void* device_handle, std::size_t nbytes) {
  if (!s.defined() || s.platform() != Platform::MPS) {
    throw_error("MPS cache write: expected defined MPS Series");
  }
  if (!runtime_available()) {
    throw_error(std::string("MPS cache write: ") + runtime_error_cstr());
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_handle;
  if (device == nil) {
    throw_error("MPS cache write: null MTLDevice");
  }

  const auto bytes = std::max<std::size_t>(nbytes, 1);
  std::scoped_lock lock(cache_mu_());
  auto& e = cache_()[impl_of_(s)];

  if (e.buffer == nil || e.nbytes != bytes) {
    wait_pending_(e);
    release_entry_buffer_(e);
    e.buffer = acquire_pooled_buffer_mm(device, bytes, "series_write");
    if (e.buffer == nil) {
      throw_error("MPS cache write: failed to allocate MTLBuffer");
    }
    e.nbytes = bytes;
  }

  e.device_valid = true;
  return (__bridge void*)e.buffer;
}

void mark_device_data_dirty(const SeriesBase& s, void* command_buffer_handle) noexcept {
  if (!s.defined() || s.platform() != Platform::MPS) {
    return;
  }
  std::scoped_lock lock(cache_mu_());
  auto& e = cache_()[impl_of_(s)];
  e.pending = (__bridge id<MTLCommandBuffer>)command_buffer_handle;
  e.host_dirty = false;
  e.device_dirty = true;
  e.device_valid = true;
  maybe_dirty_().insert(impl_of_(s));
}

void erase_series_entry(const SeriesImpl* impl) noexcept {
  if (impl == nullptr) {
    return;
  }
  std::scoped_lock lock(cache_mu_());
  auto it = cache_().find(impl);
  if (it == cache_().end()) {
    return;
  }
  wait_pending_(it->second);
  release_entry_buffer_(it->second);
  maybe_dirty_().erase(impl);
  cache_().erase(it);
}

} // namespace pluss::codem::native::mps
