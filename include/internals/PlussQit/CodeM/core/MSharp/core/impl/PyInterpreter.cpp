#include <MSharp/core/SymIntArrayRef.h>
#include <MSharp/core/SeriesImpl.h>
#include <MSharp/core/impl/PyInterpreter.h>

namespace MSharp::impl {

struct NoopPyInterpreterVTable final : public PyInterpreterVTable {
  std::string name() const override {
    return "<unloaded interpreter>";
  }

  void incref(PyObject* pyobj) const override {} // do nothing

  void decref(PyObject* pyobj, bool has_pyobj_slot) const override {
  } // do nothing

#define PANIC(m)              \
  TORCH_INTERNAL_ASSERT(      \
      0,                      \
      "attempted to call " #m \
      " on a Series with nontrivial PyObject after corresponding interpreter died")

  MSharp::intrusive_ptr<TensorImpl> detach(const TensorImpl* self) const override {
    PANIC(detach);
  }

  void dispatch(const MSharp::OperatorHandle& op, torch::jit::Stack* stack)
      const override {
    PANIC(dispatch);
  }

  void reportErrorCallback(PyObject* callback, DispatchKey key) const override {
    PANIC(reportErrorCallback);
  }

  void python_op_registration_trampoline(
      const MSharp::OperatorHandle& op,
      MSharp::DispatchKey,
      MSharp::DispatchKeySet keyset,
      torch::jit::Stack* stack,
      bool with_keyset,
      bool with_op) const override {
    PANIC(python_op_registration_trampoline);
  }

  void throw_abstract_impl_not_imported_error(
      std::string opname,
      const char* pymodule,
      const char* context) const override {
    PANIC(throw_abstract_impl_not_imported_error);
  }

  void python_dispatcher(
      const MSharp::OperatorHandle& op,
      MSharp::DispatchKeySet,
      torch::jit::Stack* stack) const override {
    PANIC(python_dispatcher);
  }

  bool is_contiguous(const TensorImpl* self, pluss::MemoryFormat) const override {
    PANIC(is_contiguous);
  }
  bool is_strides_like(const TensorImpl* self, pluss::MemoryFormat)
      const override {
    PANIC(is_strides_like);
  }
  bool is_non_overlapping_and_dense(const TensorImpl* self) const override {
    PANIC(is_non_overlapping_and_dense);
  }
  MSharp::Device device(const TensorImpl* self) const override {
    PANIC(device);
  }
  int64_t dim(const TensorImpl* self) const override {
    PANIC(dim);
  }
  MSharp::IntArrayRef strides(const TensorImpl* self) const override {
    PANIC(strides);
  }
  MSharp::IntArrayRef sizes(const TensorImpl* self) const override {
    PANIC(sizes);
  }
  MSharp::SymIntArrayRef sym_sizes(const TensorImpl* self) const override {
    PANIC(sym_sizes);
  }
  MSharp::Layout layout(const TensorImpl* self) const override {
    PANIC(layout);
  }
  int64_t numel(const TensorImpl* self) const override {
    PANIC(numel);
  }
  MSharp::SymInt sym_numel(const TensorImpl* self) const override {
    PANIC(sym_numel);
  }
  MSharp::SymIntArrayRef sym_strides(const TensorImpl* self) const override {
    PANIC(sym_strides);
  }
  MSharp::SymInt sym_storage_offset(const TensorImpl* self) const override {
    PANIC(sym_storage_offset);
  }

  // Just swallow the event, don't do anything
  void trace_gpu_event_creation(MSharp::DeviceType device_type, uintptr_t event)
      const override {}
  void trace_gpu_event_deletion(MSharp::DeviceType device_type, uintptr_t event)
      const override {}
  void trace_gpu_event_record(
      MSharp::DeviceType device_type,
      uintptr_t event,
      uintptr_t stream) const override {}
  void trace_gpu_event_wait(
      MSharp::DeviceType device_type,
      uintptr_t event,
      uintptr_t stream) const override {}
  void trace_gpu_memory_allocation(MSharp::DeviceType device_type, uintptr_t ptr)
      const override {}
  void trace_gpu_memory_deallocation(MSharp::DeviceType device_type, uintptr_t ptr)
      const override {}
  void trace_gpu_stream_creation(MSharp::DeviceType device_type, uintptr_t stream)
      const override {}
  void trace_gpu_device_synchronization(
      MSharp::DeviceType device_type) const override {}
  void trace_gpu_stream_synchronization(
      MSharp::DeviceType device_type,
      uintptr_t stream) const override {}
  void trace_gpu_event_synchronization(
      MSharp::DeviceType device_type,
      uintptr_t event) const override {}

  void reset_backward_hooks(const TensorImpl* self) const override {
    PANIC(reset_backward_hooks);
  }
};

// Construct this in Global scope instead of within `disarm`
// where it will be only initialized first time `disarm` is called.
// This increases the likelihood `noop_vtable` lives longer than
// any object that refers to it.

// If `noop_vtable` goes out of scope first, other objects will have dangling
// reference to it.
static NoopPyInterpreterVTable noop_vtable;

void PyInterpreter::disarm() noexcept {
  vtable_ = &noop_vtable;
}

} // namespace MSharp::impl
