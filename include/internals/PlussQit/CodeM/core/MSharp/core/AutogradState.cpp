#include <MSharp/core/AutogradState.h>

namespace MSharp {

namespace {
// By default, grad mode and multithreading are enabled, inference mode is
// disabled,
thread_local AutogradState autograd_state_tls = AutogradState(
    /* grad_mode */ true,
    /* inference_mode */ false,
    /* fw_grad_mode */ true,
    /* multithreading_enabled */ true);
} // namespace

AutogradState& AutogradState::get_tls_state() {
  return autograd_state_tls;
}

void AutogradState::set_tls_state(AutogradState state) {
  autograd_state_tls = state;
}

} // namespace MSharp
