#include <MSharp/core/SafePyObject.h>

namespace MSharp {

PyObject* SafePyObject::ptr(const MSharp::impl::PyInterpreter* interpreter) const {
  TORCH_INTERNAL_ASSERT(interpreter == pyinterpreter_);
  return data_;
}

PyObject* SafePyHandle::ptr(const MSharp::impl::PyInterpreter* interpreter) const {
  TORCH_INTERNAL_ASSERT(interpreter == pyinterpreter_);
  return data_;
}

} // namespace MSharp
