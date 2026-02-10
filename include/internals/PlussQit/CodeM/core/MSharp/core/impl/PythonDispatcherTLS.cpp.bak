#include <MSharp/core/DispatchKey.h>
#include <MSharp/core/impl/LocalDispatchKeySet.h>
#include <MSharp/core/impl/PythonDispatcherTLS.h>

namespace MSharp::impl {

thread_local static PyInterpreter* pythonDispatcherState;

void PythonDispatcherTLS::set_state(PyInterpreter* state) {
  if (state) {
    MSharp::impl::tls_set_dispatch_key_included(
        DispatchKey::PythonDispatcher, true);
  } else {
    PythonDispatcherTLS::reset_state();
  }
  pythonDispatcherState = state;
}

PyInterpreter* PythonDispatcherTLS::get_state() {
  return pythonDispatcherState;
}

void PythonDispatcherTLS::reset_state() {
  pythonDispatcherState = nullptr;
  MSharp::impl::tls_set_dispatch_key_included(
      DispatchKey::PythonDispatcher, false);
}

} // namespace MSharp::impl
