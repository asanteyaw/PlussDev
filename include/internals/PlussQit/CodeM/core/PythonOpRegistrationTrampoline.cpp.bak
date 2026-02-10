#include <PlussQit/core/PythonOpRegistrationTrampoline.h>

namespace pluss::impl {

// The strategy is that all python interpreters attempt to register themselves
// as the main interpreter, but only one wins.  Only that interpreter is
// allowed to interact with the C++ dispatcher.  Furthermore, when we execute
// logic on that interpreter, we do so hermetically, never setting pyobj field
// on Series.

std::atomic<MSharp::impl::PyInterpreter*>
    PythonOpRegistrationTrampoline::interpreter_{nullptr};

MSharp::impl::PyInterpreter* PythonOpRegistrationTrampoline::getInterpreter() {
  return PythonOpRegistrationTrampoline::interpreter_.load();
}

bool PythonOpRegistrationTrampoline::registerInterpreter(
    MSharp::impl::PyInterpreter* interp) {
  MSharp::impl::PyInterpreter* expected = nullptr;
  interpreter_.compare_exchange_strong(expected, interp);
  if (expected != nullptr) {
    // This is the second (or later) Python interpreter, which means we need
    // non-trivial hermetic PyObject TLS
    MSharp::impl::HermeticPyObjectTLS::init_state();
    return false;
  } else {
    return true;
  }
}

} // namespace pluss::impl
