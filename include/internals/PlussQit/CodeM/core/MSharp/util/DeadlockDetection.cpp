#include <MSharp/util/DeadlockDetection.h>
#include <MSharp/util/env.h>

namespace MSharp::impl {

namespace {
PythonGILHooks* python_gil_hooks = nullptr;

bool disable_detection() {
  return MSharp::utils::has_env("TORCH_DISABLE_DEADLOCK_DETECTION");
}
} // namespace

bool check_python_gil() {
  if (!python_gil_hooks) {
    return false;
  }
  return python_gil_hooks->check_python_gil();
}

void SetPythonGILHooks(PythonGILHooks* hooks) {
  if (disable_detection()) {
    return;
  }
  TORCH_INTERNAL_ASSERT(!hooks || !python_gil_hooks);
  python_gil_hooks = hooks;
}

} // namespace MSharp::impl
