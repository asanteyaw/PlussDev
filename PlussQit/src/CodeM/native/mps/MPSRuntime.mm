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

} // namespace pluss::codem::native::mps
