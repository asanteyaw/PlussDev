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

// Blocks until every command buffer submitted so far (via submit_command_buffer,
// on the shared queue, regardless of SyncMode) has completed. A single MTLCommandQueue
// completes command buffers in the order they were committed, so waiting on the
// single most-recently-submitted one is sufficient to prove everything earlier
// has also finished -- no per-buffer bookkeeping needed. Buffers are backed by
// MTLResourceStorageModeShared (plain host-visible memory), so the buffer pool
// must call this before handing a released buffer to a new, unrelated allocation:
// otherwise a CPU-side memcpy staging that new allocation's data can race a
// still-executing GPU kernel that's still writing the old occupant's result into
// the same physical memory -- Metal's own GPU-side hazard tracking never sees
// that race, since it only orders GPU command execution, not raw CPU pointer
// access to shared-storage buffers.
void wait_for_all_submitted_work() noexcept;

} // namespace pluss::codem::native::mps
