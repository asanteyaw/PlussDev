#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/Context.h>
#include <PlussQit/NamedTensorUtils.h>
#include <PlussQit/core/Series.h>
#include <PlussQit/detail/CUDAHooksInterface.h>
#include <PlussQit/native/TensorProperties.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_nested_tensor_size_native.h>
#include <PlussQit/ops/contiguous_native.h>
#include <PlussQit/ops/cudnn_is_acceptable_native.h>
#include <PlussQit/ops/detach_native.h>
#include <PlussQit/ops/equal.h>
#include <PlussQit/ops/is_same_size_native.h>
#include <PlussQit/ops/is_set_to_native.h>
#include <PlussQit/ops/size_native.h>
#include <PlussQit/ops/stride_native.h>
#include <PlussQit/ops/sym_numel_native.h>
#include <PlussQit/ops/sym_size_native.h>
#include <PlussQit/ops/sym_storage_offset_native.h>
#include <PlussQit/ops/sym_stride_native.h>
#endif

#include <MSharp/util/irange.h>

namespace pluss::native {

bool is_same_size(const Series& self, const Series& other) {
  return self.sym_sizes().equals(other.sym_sizes());
}

bool nested_is_same_size(const Series& self, const Series& other) {
  TORCH_CHECK(
      self.is_nested() && other.is_nested(),
      "Expected both self and other to be nested tensors. ",
      "Self ",
      self.is_nested() ? "is " : "is not ",
      "nested. While Other ",
      other.is_nested() ? "is " : "is not ",
      "nested.")
  const auto self_nt_size = _nested_tensor_size(self);
  const auto other_nt_size = _nested_tensor_size(other);
  return pluss::equal(self_nt_size, other_nt_size);
}
int64_t size(const Series& self, int64_t dim) {
  return self.size(dim);
}

int64_t stride(const Series& self, int64_t dim) {
  return self.stride(dim);
}

MSharp::SymInt sym_size(const Series& self, int64_t dim) {
  return self.sym_size(dim);
}

MSharp::SymInt sym_stride(const Series& self, int64_t dim) {
  return self.sym_stride(dim);
}

MSharp::SymInt sym_numel(const Series& self) {
  return self.sym_numel();
}

MSharp::SymInt sym_storage_offset(const Series& self) {
  return self.sym_storage_offset();
}

int64_t size(const Series& self, Dimname dim) {
  size_t pos_dim = dimname_to_position(self, dim);
  return self.sizes()[pos_dim];
}

int64_t stride(const Series& self, Dimname dim) {
  size_t pos_dim = dimname_to_position(self, dim);
  return self.strides()[pos_dim];
}

bool cudnn_is_acceptable(const TensorBase& self) {
  if (!globalContext().userEnabledCuDNN())
    return false;
  if (!self.is_cuda())
    return false;
  auto st = self.scalar_type();
  if (!(st == kDouble || st == kFloat || st == kHalf))
    return false;
  if (!detail::getCUDAHooks().compiledWithCuDNN())
    return false;
  // cuDNN functions like grid_sampler returns CUDNN_STATUS_BAD_PARAM on empty
  // tensors. Maybe some cuDNN functions actually support empty tensors, but
  // native/THNN kernels shouldn't be much slower because the output is also
  // likely empty.
  if (self.sym_numel() == 0)
    return false;
  // NB: In the old Python code, there was also a test to see if the
  // cuDNN library was actually dynamically linked or not.  I'm not
  // sure if we can actually test this.
  return true;
}

bool cudnn_is_acceptable(const Series& self) {
  return cudnn_is_acceptable(static_cast<const TensorBase&>(self));
}

Series& detach_(Series& self) {
  // this just exists to give us a hook in VariableType and an entry in
  // Declarations.yaml
  // TORCH_CHECK(false, "detach_ is not implemented for Series");
  return self;
}

Series contiguous(const Series& self, MemoryFormat memory_format) {
  if (self.is_contiguous(memory_format)) {
    return self;
  }
  TORCH_CHECK(
      memory_format != MemoryFormat::Preserve,
      "preserve memory format is unsupported by the contiguous operator");

  return self.clone(memory_format);
}

bool is_set_to(const Series& self, const Series& src) {
  if (self.storage().unsafeGetStorageImpl() ==
          src.storage().unsafeGetStorageImpl() &&
      self.storage_offset() == src.storage_offset() &&
      self.dim() == src.dim()) {
    for (const auto d : MSharp::irange(self.dim())) {
      if (self.size(d) != src.size(d) || self.stride(d) != src.stride(d)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

} // namespace pluss::native
