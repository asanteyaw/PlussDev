#pragma once

#include <MSharp/core/impl/PyInterpreter.h>
#include <MSharp/macros/Export.h>
#include <MSharp/util/python_stub.h>
#include <utility>

namespace MSharp {

// This is an safe owning holder for a PyObject, akin to pybind11's
// py::object, with two major differences:
//
//  - It is in c10/core; i.e., you can use this type in contexts where
//    you do not have a libpython dependency
//
//  - It is multi-interpreter safe (ala torchdeploy); when you fetch
//    the underlying PyObject* you are required to specify what the current
//    interpreter context is and we will check that you match it.
//
// It is INVALID to store a reference to a Series object in this way;
// you should just use TensorImpl directly in that case!
struct C10_API SafePyObject {
  // Steals a reference to data
  SafePyObject(PyObject* data, MSharp::impl::PyInterpreter* pyinterpreter)
      : data_(data), pyinterpreter_(pyinterpreter) {}
  SafePyObject(SafePyObject&& other) noexcept
      : data_(std::exchange(other.data_, nullptr)),
        pyinterpreter_(other.pyinterpreter_) {}
  // For now it's not used, so we just disallow it.
  SafePyObject& operator=(SafePyObject&&) = delete;

  SafePyObject(SafePyObject const& other)
      : data_(other.data_), pyinterpreter_(other.pyinterpreter_) {
    if (data_ != nullptr) {
      (*pyinterpreter_)->incref(data_);
    }
  }

  SafePyObject& operator=(SafePyObject const& other) {
    if (this == &other) {
      return *this; // Handle self-assignment
    }
    if (other.data_ != nullptr) {
      (*other.pyinterpreter_)->incref(other.data_);
    }
    if (data_ != nullptr) {
      (*pyinterpreter_)->decref(data_, /*has_pyobj_slot*/ false);
    }
    data_ = other.data_;
    pyinterpreter_ = other.pyinterpreter_;
    return *this;
  }

  ~SafePyObject() {
    if (data_ != nullptr) {
      (*pyinterpreter_)->decref(data_, /*has_pyobj_slot*/ false);
    }
  }

  MSharp::impl::PyInterpreter& pyinterpreter() const {
    return *pyinterpreter_;
  }
  PyObject* ptr(const MSharp::impl::PyInterpreter*) const;

  // stop tracking the current object, and return it
  PyObject* release() {
    auto rv = data_;
    data_ = nullptr;
    return rv;
  }

 private:
  PyObject* data_;
  MSharp::impl::PyInterpreter* pyinterpreter_;
};

// A newtype wrapper around SafePyObject for type safety when a python object
// represents a specific type. Note that `T` is only used as a tag and isn't
// actually used for any true purpose.
template <typename T>
struct SafePyObjectT : private SafePyObject {
  SafePyObjectT(PyObject* data, MSharp::impl::PyInterpreter* pyinterpreter)
      : SafePyObject(data, pyinterpreter) {}
  ~SafePyObjectT() = default;
  SafePyObjectT(SafePyObjectT&& other) noexcept : SafePyObject(other) {}
  SafePyObjectT(SafePyObjectT const&) = delete;
  SafePyObjectT& operator=(SafePyObjectT const&) = delete;
  SafePyObjectT& operator=(SafePyObjectT&&) = delete;

  using SafePyObject::ptr;
  using SafePyObject::pyinterpreter;
  using SafePyObject::release;
};

// Like SafePyObject, but non-owning.  Good for references to global PyObjects
// that will be leaked on interpreter exit.  You get a copy constructor/assign
// this way.
struct C10_API SafePyHandle {
  SafePyHandle() : data_(nullptr), pyinterpreter_(nullptr) {}
  SafePyHandle(PyObject* data, MSharp::impl::PyInterpreter* pyinterpreter)
      : data_(data), pyinterpreter_(pyinterpreter) {}

  MSharp::impl::PyInterpreter& pyinterpreter() const {
    return *pyinterpreter_;
  }
  PyObject* ptr(const MSharp::impl::PyInterpreter*) const;
  void reset() {
    data_ = nullptr;
    pyinterpreter_ = nullptr;
  }
  operator bool() {
    return data_;
  }

 private:
  PyObject* data_;
  MSharp::impl::PyInterpreter* pyinterpreter_;
};

} // namespace MSharp
