#include "CodeM/native/mps/MPSAllocator.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

namespace pluss::codem::native::mps {
namespace {

struct InFlightBuffer {
  void* buffer_ref{nullptr};         // retained id<MTLBuffer>
  void* command_buffer_ref{nullptr}; // retained id<MTLCommandBuffer>
  std::size_t nbytes{0};
};

struct PoolState {
  std::map<std::size_t, std::vector<void*>> free_bins{}; // retained id<MTLBuffer>
  std::vector<InFlightBuffer> inflight{};
  std::size_t cached_bytes{0};
  std::size_t inflight_bytes{0};
  std::size_t max_cached_bytes{256ull * 1024ull * 1024ull};
  bool initialized{false};
};

std::mutex& pool_mu_() {
  static std::mutex mu;
  return mu;
}

PoolState& pool_() {
  static PoolState pool;
  return pool;
}

std::size_t parse_size_env_(const char* name, std::size_t fallback) noexcept {
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const unsigned long long v = std::strtoull(raw, &end, 10);
  if (end == raw || (end != nullptr && *end != '\0')) {
    return fallback;
  }
  return static_cast<std::size_t>(v);
}

std::size_t buffer_size_from_ref_(void* buffer_ref) noexcept {
  if (buffer_ref == nullptr) {
    return 0;
  }
  id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)buffer_ref;
  return static_cast<std::size_t>(buffer.length);
}

void ensure_init_(PoolState& p) {
  if (p.initialized) {
    return;
  }
  p.max_cached_bytes = parse_size_env_("PLUSS_MPS_ALLOC_MAX_CACHED_BYTES", p.max_cached_bytes);
  p.initialized = true;
}

void release_ref_(void* ref) noexcept {
  if (ref != nullptr) {
    (void)CFBridgingRelease(ref);
  }
}

void recycle_completed_inflight_(PoolState& p) {
  std::size_t dst = 0;
  for (std::size_t i = 0; i < p.inflight.size(); ++i) {
    auto& in = p.inflight[i];
    id<MTLCommandBuffer> cb = (__bridge id<MTLCommandBuffer>)in.command_buffer_ref;
    const bool done = (cb == nil) ||
                      (cb.status == MTLCommandBufferStatusCompleted) ||
                      (cb.status == MTLCommandBufferStatusError);
    if (done) {
      p.inflight_bytes = (p.inflight_bytes >= in.nbytes) ? (p.inflight_bytes - in.nbytes) : 0;
      if (in.buffer_ref != nullptr && in.nbytes > 0) {
        p.free_bins[in.nbytes].push_back(in.buffer_ref);
        p.cached_bytes += in.nbytes;
        in.buffer_ref = nullptr;
      }
      release_ref_(in.command_buffer_ref);
      in.command_buffer_ref = nullptr;
      continue;
    }
    if (dst != i) {
      p.inflight[dst] = in;
    }
    ++dst;
  }
  p.inflight.resize(dst);
}

void trim_locked_(PoolState& p, std::size_t target_cached_bytes) {
  if (target_cached_bytes == 0) {
    target_cached_bytes = p.max_cached_bytes;
  }
  if (p.cached_bytes <= target_cached_bytes) {
    return;
  }

  for (auto it = p.free_bins.rbegin(); it != p.free_bins.rend() && p.cached_bytes > target_cached_bytes; ++it) {
    auto& vec = it->second;
    const std::size_t nbytes = it->first;
    while (!vec.empty() && p.cached_bytes > target_cached_bytes) {
      void* ref = vec.back();
      vec.pop_back();
      release_ref_(ref);
      p.cached_bytes = (p.cached_bytes >= nbytes) ? (p.cached_bytes - nbytes) : 0;
    }
  }

  for (auto it = p.free_bins.begin(); it != p.free_bins.end();) {
    if (it->second.empty()) {
      it = p.free_bins.erase(it);
    } else {
      ++it;
    }
  }
}

id<MTLBuffer> pop_reusable_buffer_(PoolState& p, std::size_t nbytes) {
  auto it = p.free_bins.lower_bound(nbytes);
  if (it == p.free_bins.end()) {
    return nil;
  }

  id<MTLBuffer> buffer = nil;
  auto& vec = it->second;
  if (!vec.empty()) {
    void* ref = vec.back();
    vec.pop_back();
    buffer = (__bridge_transfer id<MTLBuffer>)ref;
    p.cached_bytes = (p.cached_bytes >= it->first) ? (p.cached_bytes - it->first) : 0;
  }

  if (vec.empty()) {
    p.free_bins.erase(it);
  }
  return buffer;
}

} // namespace

void* acquire_pooled_buffer(void* device_handle, std::size_t nbytes, const char* /*tag*/) {
  id<MTLDevice> device = (__bridge id<MTLDevice>)device_handle;
  if (device == nil) {
    return nullptr;
  }

  const std::size_t bytes = (nbytes == 0) ? 1 : nbytes;
  std::scoped_lock lock(pool_mu_());
  auto& p = pool_();
  ensure_init_(p);
  recycle_completed_inflight_(p);

  id<MTLBuffer> buffer = pop_reusable_buffer_(p, bytes);
  if (buffer == nil) {
    buffer = [device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
  }
  return (__bridge_retained void*)buffer;
}

void release_pooled_buffer(void* buffer_handle) noexcept {
  id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)buffer_handle;
  if (buffer == nil) {
    return;
  }

  std::scoped_lock lock(pool_mu_());
  auto& p = pool_();
  ensure_init_(p);
  recycle_completed_inflight_(p);

  void* ref = (__bridge_retained void*)buffer;
  const std::size_t nbytes = buffer_size_from_ref_(ref);
  if (nbytes == 0) {
    release_ref_(ref);
    return;
  }
  p.free_bins[nbytes].push_back(ref);
  p.cached_bytes += nbytes;
  trim_locked_(p, 0);
}

void defer_release_pooled_buffer(void* buffer_handle, void* command_buffer_handle) noexcept {
  id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)buffer_handle;
  id<MTLCommandBuffer> cb = (__bridge id<MTLCommandBuffer>)command_buffer_handle;
  if (buffer == nil) {
    return;
  }
  if (cb == nil) {
    release_pooled_buffer(buffer_handle);
    return;
  }

  std::scoped_lock lock(pool_mu_());
  auto& p = pool_();
  ensure_init_(p);
  recycle_completed_inflight_(p);

  void* buffer_ref = (__bridge_retained void*)buffer;
  const std::size_t nbytes = buffer_size_from_ref_(buffer_ref);
  if (nbytes == 0) {
    release_ref_(buffer_ref);
    return;
  }

  void* cb_ref = (__bridge_retained void*)cb;
  p.inflight.push_back(InFlightBuffer{buffer_ref, cb_ref, nbytes});
  p.inflight_bytes += nbytes;
}

void trim_pooled_buffers(std::size_t target_cached_bytes) noexcept {
  std::scoped_lock lock(pool_mu_());
  auto& p = pool_();
  ensure_init_(p);
  recycle_completed_inflight_(p);
  trim_locked_(p, target_cached_bytes);
}

std::size_t pooled_cached_bytes() noexcept {
  std::scoped_lock lock(pool_mu_());
  auto& p = pool_();
  ensure_init_(p);
  recycle_completed_inflight_(p);
  return p.cached_bytes;
}

std::size_t pooled_inflight_bytes() noexcept {
  std::scoped_lock lock(pool_mu_());
  auto& p = pool_();
  ensure_init_(p);
  recycle_completed_inflight_(p);
  return p.inflight_bytes;
}

} // namespace pluss::codem::native::mps
