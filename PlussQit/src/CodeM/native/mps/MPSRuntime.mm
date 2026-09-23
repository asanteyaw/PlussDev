#include "CodeM/native/mps/MPSRuntime.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>

namespace pluss::codem::native::mps {
namespace {

struct RuntimeContext {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  // CommitAdaptive's async dispatch (skip waitUntilCompleted whenever a
  // command buffer's work_items exceeds adaptive_wait_work_items) used to
  // race against the buffer pool: a Series destroyed right after a later op
  // read it as an input only waited for ITS OWN producing command buffer
  // before its MPS buffer went back to the pool -- not for any LATER
  // command buffer that read it -- so the pool could hand that buffer to a
  // new, unrelated allocation while the earlier op's kernel was still
  // reading it on the GPU. Buffers are MTLResourceStorageModeShared (plain
  // host-visible memory), so that new allocation's CPU-side memcpy into the
  // "free" buffer raced the still-executing GPU kernel writing the same
  // physical memory -- invisible to Metal's own hazard tracking, since that
  // only orders GPU command execution, never raw CPU pointer access.
  // Confirmed live and reproducibly: a broadcast add computed a+a*b instead
  // of a+b for operands above the 4096-element threshold, and a longer
  // chained NGARCH-style recursion diverged from its CPU result entirely
  // once per-timestep tensors crossed the same threshold.
  //
  // Fixed at the root in BufferCache.mm's release_entry_buffer_(): before
  // any buffer goes back into the pool for reuse, wait_for_all_submitted_
  // work() (MPSRuntime.mm) drains every command buffer submitted so far on
  // the shared queue. A single MTLCommandQueue completes command buffers in
  // commit order, so waiting on the single most-recently-submitted one
  // proves everything earlier -- including any op that read this buffer as
  // an input -- has also finished; no per-buffer read bookkeeping needed.
  // Verified against both reproductions above with PLUSS_MPS_SYNC_MODE=
  // adaptive forced (i.e. with this default's safety net entirely absent)
  // and the full regression suite, zero failures. CommitAdaptive is safe to
  // default to again.
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
  // Default is CommitAdaptive -- see RuntimeContext::mode's comment.
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

std::mutex& latest_submitted_mu_() {
  static std::mutex mu;
  return mu;
}

// Strong reference under ARC: assigning here keeps the command buffer alive
// as long as it's the latest one, independent of whether the caller that
// submitted it still holds a reference. Guarded entirely by
// latest_submitted_mu_(); accessed only through set_latest_submitted_/
// get_latest_submitted_ below.
static id<MTLCommandBuffer> g_latest_submitted = nil;

void set_latest_submitted_(id<MTLCommandBuffer> cb) {
  g_latest_submitted = cb;
}

id<MTLCommandBuffer> get_latest_submitted_() {
  return g_latest_submitted;
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

  {
    std::scoped_lock lock(latest_submitted_mu_());
    set_latest_submitted_(cb);
  }

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

void wait_for_all_submitted_work() noexcept {
  id<MTLCommandBuffer> cb = nil;
  {
    std::scoped_lock lock(latest_submitted_mu_());
    cb = get_latest_submitted_();
  }
  if (cb != nil) {
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

} // namespace pluss::codem::native::mps
