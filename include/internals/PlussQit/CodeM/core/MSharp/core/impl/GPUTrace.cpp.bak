#include <MSharp/core/impl/GPUTrace.h>

namespace MSharp::impl {

std::atomic<const PyInterpreter*> GPUTrace::gpuTraceState{nullptr};

bool GPUTrace::haveState{false};

void GPUTrace::set_trace(const PyInterpreter* trace) {
  static bool once_flag [[maybe_unused]] = [&]() {
    gpuTraceState.store(trace, std::memory_order_release);
    haveState = true;
    return true;
  }();
}

} // namespace MSharp::impl
