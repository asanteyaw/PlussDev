#include <MSharp/core/InferenceMode.h>

namespace MSharp {
// Invariant:
//   is_enabled() ==
//   !MSharp::impl::tls_is_dispatch_key_included(DispatchKey::ADInplaceOrView);
// InferenceMode::is_enabled() is in perf critical path (TensorImpl constructor)
// so it worths a separate TLS to skip the DispatchKeySet check.
bool InferenceMode::is_enabled() {
  return AutogradState::get_tls_state().get_inference_mode();
}
} // namespace MSharp
