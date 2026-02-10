#include <MSharp/core/impl/HermeticPyObjectTLS.h>

namespace MSharp::impl {

thread_local static std::atomic<bool> hermeticPyObjectState{false};

std::atomic<bool> HermeticPyObjectTLS::haveState_{false};

void HermeticPyObjectTLS::set_state(bool state) {
  hermeticPyObjectState = state;
}

bool HermeticPyObjectTLS::get_tls_state() {
  return hermeticPyObjectState;
}

void HermeticPyObjectTLS::init_state() {
  haveState_ = true;
}

} // namespace MSharp::impl
