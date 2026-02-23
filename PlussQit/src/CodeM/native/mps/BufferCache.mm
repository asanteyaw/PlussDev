#include "CodeM/native/mps/BufferCache.h"

#include "CodeM/core/Errors.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace pluss::codem::native::mps {
namespace {

struct RuntimeContext {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  SyncMode mode{SyncMode::CommitAdaptive};
  std::int64_t adaptive_wait_work_items{4096};
  std::string error{};
};

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

SyncMode parse_sync_mode_env_() noexcept {
  const char* raw = std::getenv("PLUSS_MPS_SYNC_MODE");
  if (raw == nullptr || *raw == '\0') return SyncMode::CommitAdaptive;
  if (ieq_(raw, "commit")) return SyncMode::Commit;
  if (ieq_(raw, "wait") || ieq_(raw, "commit_and_wait")) return SyncMode::CommitAndWait;
  if (ieq_(raw, "adaptive")) return SyncMode::CommitAdaptive;
  return SyncMode::CommitAdaptive;
}

std::int64_t parse_adaptive_wait_work_items_env_() noexcept {
  const char* raw = std::getenv("PLUSS_MPS_ADAPTIVE_WAIT_WORK_ITEMS");
  if (raw == nullptr || *raw == '\0') return 4096;
  char* end = nullptr;
  const long long v = std::strtoll(raw, &end, 10);
  if (end == raw || (end != nullptr && *end != '\0')) return 4096;
  if (v < 0) return 0;
  return static_cast<std::int64_t>(v);
}

RuntimeContext& runtime_ctx_() {
  static RuntimeContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      ctx.device = MTLCreateSystemDefaultDevice();
      if (ctx.device == nil) {
        ctx.error = "MPS runtime unavailable: no Metal device";
        return;
      }
      ctx.queue = [ctx.device newCommandQueue];
      if (ctx.queue == nil) {
        ctx.error = "MPS runtime unavailable: no Metal command queue";
        return;
      }
      ctx.mode = parse_sync_mode_env_();
      ctx.adaptive_wait_work_items = parse_adaptive_wait_work_items_env_();
      ctx.ready = true;
    }
  });
  return ctx;
}

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

void wait_pending_(CacheEntry& e) {
  if (e.pending != nil) {
    [e.pending waitUntilCompleted];
    e.pending = nil;
  }
}

} // namespace

bool runtime_available() noexcept {
  return runtime_ctx_().ready;
}

const char* runtime_error_cstr() noexcept {
  return runtime_ctx_().error.c_str();
}

void* shared_device_handle() noexcept {
  return runtime_ctx_().ready ? (__bridge void*)runtime_ctx_().device : nullptr;
}

void* shared_queue_handle() noexcept {
  return runtime_ctx_().ready ? (__bridge void*)runtime_ctx_().queue : nullptr;
}

SyncMode sync_mode() noexcept {
  return runtime_ctx_().mode;
}

void submit_command_buffer(void* command_buffer_handle, std::int64_t work_items_hint) noexcept {
  if (command_buffer_handle == nullptr) {
    return;
  }
  id<MTLCommandBuffer> cb = (__bridge id<MTLCommandBuffer>)command_buffer_handle;
  if (cb == nil) {
    return;
  }
  [cb commit];

  const auto& ctx = runtime_ctx_();
  bool should_wait = false;
  switch (ctx.mode) {
    case SyncMode::Commit:
      should_wait = false;
      break;
    case SyncMode::CommitAndWait:
      should_wait = true;
      break;
    case SyncMode::CommitAdaptive:
      should_wait = (ctx.adaptive_wait_work_items > 0 && work_items_hint > 0 &&
                     work_items_hint <= ctx.adaptive_wait_work_items);
      break;
    default:
      should_wait = false;
      break;
  }
  if (should_wait) {
    [cb waitUntilCompleted];
  }
}

void wait_command_buffer(void* command_buffer_handle) noexcept {
  if (command_buffer_handle == nullptr) {
    return;
  }
  id<MTLCommandBuffer> cb = (__bridge id<MTLCommandBuffer>)command_buffer_handle;
  if (cb == nil) {
    return;
  }
  [cb waitUntilCompleted];
}

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
    e.buffer = [device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
    if (e.buffer == nil) {
      throw_error("MPS cache read: failed to allocate MTLBuffer");
    }
    e.nbytes = bytes;
    e.device_valid = false;
    e.device_dirty = false;
  }

  if (e.host_dirty || !e.device_valid) {
    const auto* host = host_ptr_const_(s);
    if (host == nullptr) {
      throw_error("MPS cache read: null host storage");
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
    e.buffer = [device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
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
  cache_().erase(it);
}

} // namespace pluss::codem::native::mps
