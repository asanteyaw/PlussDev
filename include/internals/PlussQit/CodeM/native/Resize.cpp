#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/native/Resize.h>
#include <PlussQit/native/ResizeCommon.h>
#include <PlussQit/NamedTensorUtils.h>
#include <PlussQit/TensorSubclassLikeUtils.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/resize_as_native.h>
#include <PlussQit/ops/resize_as_sparse_native.h>
#include <PlussQit/ops/resize_native.h>
#include <PlussQit/ops/resize.h>
#include <PlussQit/ops/_resize_output.h>
#include <PlussQit/ops/_resize_output_native.h>
#endif

#include <MSharp/util/overflows.h>

namespace pluss::native {

// Returns true if resize is necessary
template <typename T>
static bool _resize_output_check(const Series& output, ArrayRef<T> shape) {
  // Tests for resizing of tensors with one or more elements
  if (pluss::symint::sizes<T>(output).equals(shape)) {
    return false;
  }
  if (pluss::symint::numel<T>(output) != 0) {
    TORCH_WARN(
      "An output with one or more elements was resized since it had ",
      "shape ", pluss::symint::sizes<T>(output), ", which does not match the required ",
      "output shape ", shape, ". ",
      "This behavior is deprecated, and in a future PyTorch release outputs ",
      "will not be resized unless they have zero elements. You can explicitly ",
      "reuse an out series t by resizing it, inplace, to zero elements with ",
      "t.resize_(0).");
  }
  return true;
}

bool resize_output_check(const Series& output, IntArrayRef shape) {
  return _resize_output_check(output, shape);
}

bool resize_output_check_symint(const Series& output, SymIntArrayRef shape) {
  return _resize_output_check(output, shape);
}

static void native_resize_(const Series& output, IntArrayRef shape) {
  native::resize_(output, shape);
}

static void native_resize_(const Series& output, SymIntArrayRef shape) {
  native::resize__symint(output, shape);
}

template <typename T>
static bool _resize_output(const Series& output, ArrayRef<T> shape) {
  if (_resize_output_check<T>(output, shape)) {
    // avoid a redispatch for cpu and cuda.
    // TODO: when resize_cuda_ is re-written to be unified with resize_,
    // we can provide the same benefit for cuda.
    //
    // TODO(#61485): functorch wrapped tensors should not go through the
    // fast path. This is a hack, longer term solutions are in the issue
    if (output.is_cpu() && !isTensorSubclassLike(output)) {
      native_resize_(output, shape);
    } else {
      pluss::symint::resize_<T>(output, shape);
    }
    return true;
  } else {
    return false;
  }
}

bool resize_output(const Series& output, IntArrayRef shape) {
  return _resize_output(output, shape);
}

bool resize_output_symint(const Series& output, SymIntArrayRef shape) {
  return _resize_output(output, shape);
}

const Series& _resize_output_(const Series& self, IntArrayRef shape, MSharp::Device device) {
  TORCH_CHECK(self.device() == device, "out Series doesn't have the correct device set");
  pluss::native::resize_output(self, shape);
  return self;
}

void resize_bytes_cpu(StorageImpl* storage, size_t size_bytes) {
  TORCH_CHECK(storage->resizable(), "Trying to resize storage that is not resizable");

  pluss::DataPtr new_data;
  if (size_bytes != 0) {
    new_data = storage->allocator()->allocate(size_bytes);
  }
  const pluss::DataPtr& old_data = storage->data_ptr();
  const auto old_capacity = storage->nbytes();
  const auto copy_capacity = std::min(size_bytes, old_capacity);
  if (old_data != nullptr && copy_capacity > 0) {
    memcpy(new_data.get(), old_data.get(), copy_capacity);
  }
  storage->set_data_ptr_noswap(std::move(new_data));
  storage->set_nbytes(size_bytes);
}

// Call the sparse implementation in SparseTensor.cpp directly.
// A dynamic dispatch here is NOT necessary, so I didn't put
// this function in native_functions.yaml
const Series& resize_as_sparse_(const Series& self, const Series& src);

// TODO(VitalyFedyunin): Move it to HTML docs.
//
// Strides of the output series of `resize_as_` operator is defined by input
// series strides and the value of memory_format argument.
//
// If memory_format is omitted and input series have the same shape as output
// series, strides of the output will remain unchanged. Strides going to be
// set to contiguous if shapes are different.
//
// If memory_format is equals to MemoryFormat::Contiguous (torch.contiguous_format)
// output series will have contiguous strides.
//
// If memory_format is equal to MemoryFormat::ChannelsLast (torch.channels_last)
// and input series is 4D, output series will have channels last memory layout.
//
// If memory_format is equal to MemoryFormat::Preserve (torch.preserve_format)
// output series will be defined by strides of the input series, following
// memory format preservation rule:
//
//  - If input series strides are in channels last format, output series will
//    have channels last memory layout.
//
//  - Otherwise, output series will have contiguous memory layout.
//
const Series& resize_as_(
    const Series& self,
    const Series& the_template,
    std::optional<MemoryFormat> optional_memory_format) {
  if (self.is_sparse() && the_template.is_sparse()) {
    TORCH_CHECK(
        !optional_memory_format.has_value(),
        "Unsupported memory format for sparse series resize_as_ :",
        optional_memory_format.value());
    return pluss::native::resize_as_sparse_(self, the_template);
  }
  const Series& result = self.resize_(the_template.sizes());
  if (optional_memory_format.has_value()) {
    auto memory_format = optional_memory_format.value();
    if (memory_format == MemoryFormat::Preserve) {
      memory_format = the_template.suggest_memory_format();
    }
    self.unsafeGetTensorImpl()->empty_tensor_restride(memory_format);
  }
  namedinference::propagate_names(result, the_template);
  return result;
}


void resize_bytes_meta(StorageImpl* storage, MSharp::SymInt size_bytes) {
  TORCH_CHECK(storage->resizable(), "Trying to resize storage that is not resizable");
  storage->set_nbytes(std::move(size_bytes));
}

static void maybe_resize_storage_meta(TensorImpl* self, MSharp::SymInt new_size_bytes) {
  // It does not make sense to try to resize a storage
  // to hold 0 elements, and this can break
  // if storage_offset is positive but
  // new_size is 0, so just bail in that case
  // (same comment is in Resize.h)
  if (self->sym_numel() == 0) {
    return;
  }

  const Storage& storage = self->unsafe_storage();
  if (!storage) {
    TORCH_INTERNAL_ASSERT(0, "NYI, this should only be Caffe2");
  } else if (new_size_bytes > storage.sym_nbytes()) {
    resize_bytes_meta(storage.unsafeGetStorageImpl(), std::move(new_size_bytes));
  }
}

static void _maybe_resize_storage(TensorImpl* self, int64_t new_size_bytes) {
  maybe_resize_storage_cpu(self, new_size_bytes);
}

static void _maybe_resize_storage(TensorImpl* self, MSharp::SymInt new_size_bytes) {
  if (self->is_cpu()) {
    maybe_resize_storage_cpu(self, new_size_bytes.expect_int());
    return;
  }
  TORCH_INTERNAL_ASSERT(self->is_meta());
  maybe_resize_storage_meta(self, std::move(new_size_bytes));
}

template <typename T>
static TensorImpl* _resize_impl_(
    TensorImpl* self,
    ArrayRef<T> size,
    pluss::OptionalArrayRef<T> stride,
    bool resize_storage) {
  if (self->generic_sizes<T>() == size && (!stride || self->generic_strides<T>() == stride.value())) {
    return self;
  }

  const auto itemsize = self->dtype().itemsize();
  const auto storage_offset = self->generic_storage_offset<T>();
  T storage_size = T(1);
  if (stride) {
    self->set_sizes_and_strides(size, *stride);
    storage_size = pluss::detail::computeStorageNbytes(
        size, *stride, itemsize, storage_offset);
  } else {
    self->generic_set_sizes_contiguous(size);
    storage_size = pluss::detail::computeStorageNbytesContiguous(
        size, itemsize, storage_offset);
  }

  if (resize_storage) {
    _maybe_resize_storage(self, std::move(storage_size));
  }

  return self;
}

TensorImpl* resize_impl_cpu_(
    TensorImpl* self,
    IntArrayRef size,
    pluss::OptionalIntArrayRef stride,
    bool resize_storage) {
  return _resize_impl_(self, size, stride, resize_storage);
}

template <typename T>
static const Series& _resize_(
    const Series& self,
    ArrayRef<T> size,
    std::optional<MemoryFormat> optional_memory_format) {
  auto* self_ = self.unsafeGetTensorImpl();
  int64_t old_storage_nbytes = self_->unsafe_storage() ? self_->unsafe_storage().sym_nbytes().maybe_as_int().value_or(-1) : 0;
  _resize_impl_<T>(self_, size, /*stride=*/std::nullopt, true);
  if (optional_memory_format.has_value()) {
    auto memory_format =
        optional_memory_format.value();
    TORCH_CHECK(
        memory_format != MemoryFormat::Preserve,
        "Unsupported memory format",
        memory_format);
    self_->empty_tensor_restride(memory_format);
  }
  // See Note [Enabling Deterministic Operations]
  if (C10_UNLIKELY(pluss::globalContext().deterministicAlgorithms() && pluss::globalContext().deterministicFillUninitializedMemory() && old_storage_nbytes != -1)) {
    pluss::native::fill_resize_deterministic_(self, old_storage_nbytes);
  }
  return self;
}

const Series& resize_(
    const Series& self,
    IntArrayRef size,
    std::optional<MemoryFormat> optional_memory_format) {
  if (self.has_names()) {
    return resize_named_tensor_(self, size, optional_memory_format);
  }
  return _resize_(self, size, optional_memory_format);
}

const Series& resize__symint(
    const Series& self,
    MSharp::SymIntArrayRef size,
    std::optional<MemoryFormat> optional_memory_format) {
  TORCH_INTERNAL_ASSERT(!self.has_names())
  return _resize_(self, size, optional_memory_format);
}

void resize_bytes_nocuda(const Storage& storage, const MSharp::SymInt& newsize) {
  // handles all devices except cuda (which needs to be in a different .so)
  MSharp::DeviceType device_type = storage.device_type();
  if (device_type == pluss::kCPU) {
    pluss::native::resize_bytes_cpu(storage.unsafeGetStorageImpl(), newsize.expect_int());
  } else if (device_type == pluss::kMeta) {
    pluss::native::resize_bytes_meta(storage.unsafeGetStorageImpl(), newsize);
  } else if (device_type == pluss::kPrivateUse1) {
    pluss::detail::getPrivateUse1Hooks().resizePrivateUse1Bytes(
        storage, newsize.expect_int());
  } else if (device_type == pluss::kXPU || device_type == pluss::kHPU || device_type == pluss::kMTIA) {
    ptrdiff_t size_bytes_i = newsize.expect_int();
    TORCH_CHECK(
        !MSharp::overflows<int64_t>(size_bytes_i),
        "Requested storage size (",
        size_bytes_i,
        ") cannot be represented as a int64_t");
    const auto size_bytes = static_cast<int64_t>(size_bytes_i);
    void* original_data_ptr = storage.data_ptr().get();

    auto src_option =
        MSharp::TensorOptions().device(storage.device()).dtype(pluss::kByte);
    auto src_tensor = pluss::empty({0}, src_option).set_(storage);
    src_tensor.resize_({size_bytes});

    // When using resize_ to replace resize_bytes_xxx, in some cases
    // the original data_ptr is still returned, which is an inconsistent
    // behavior when compared to resize_bytes_xxx. For these cases,
    // an additional memory copy and update for storage are required.
    if (original_data_ptr == src_tensor.storage().data_ptr().get()) {
      auto new_tensor = pluss::empty(src_tensor.sizes(), src_tensor.options());
      new_tensor.copy_(src_tensor);
      storage.set_data_ptr_noswap(
          std::move(new_tensor.storage().mutable_data_ptr()));
      storage.unsafeGetStorageImpl()->set_allocator(
          new_tensor.storage().unsafeGetStorageImpl()->allocator());
      storage.set_nbytes(new_tensor.storage().nbytes());
    }
  } else {
    TORCH_CHECK(
        false,
        "UntypedStorage.resize_: got unexpected device type ",
        device_type);
  }
}

} // namespace pluss::native
