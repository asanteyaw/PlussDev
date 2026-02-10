#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <MSharp/util/SmallBuffer.h>
#include <MSharp/core/impl/COW.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_has_same_storage_numel_native.h>
#include <PlussQit/ops/_make_dual_native.h>
#include <PlussQit/ops/_new_zeros_with_same_feature_meta_native.h>
#include <PlussQit/ops/_unpack_dual_native.h>
#include <PlussQit/ops/_lazy_clone_native.h>
#include <PlussQit/ops/alias.h>
#include <PlussQit/ops/zeros.h>
#endif

namespace pluss::native {

// We expect this code to only be reached in inference mode and when all inputs are inference tensors
Series _make_dual(const Series& primal, const Series& tangent, int64_t level) {
  TORCH_INTERNAL_ASSERT(
      InferenceMode::is_enabled() && primal.is_inference() && tangent.is_inference(),
      "Expected this function to only be reached in inference mode and when all the "
      "inputs are inference tensors. You should NOT call this function directly as "
      "native::_make_dual. Please use the dispatcher, i.e., pluss::_make_dual. Please "
      "file an issue if you come across this error otherwise.");
  return pluss::alias(primal);
}

/// This function can be used to unpack a given dual Series to get its primal and tangent. The returned primal
/// is a view of the dual and the tangent is returned as is.
/// This function is backward differentiable.
std::tuple<pluss::Series, pluss::Series> _unpack_dual(const pluss::Series& series, int64_t level) {
  return std::tuple<pluss::Series, pluss::Series>(series._fw_primal(level), series._fw_grad(level));
}

// NB: This function can be called directly from _set_fw_grad or
//     if self is batched, from this function's batching rule.
//     See NOTE: [_new_zeros_with_same_feature_meta] for more information.
Series _new_zeros_with_same_feature_meta(
    const pluss::Series& self,
    const pluss::Series& other,
    int64_t self_num_batch_dims) {
  auto other_sizes = other.sym_sizes();
  auto other_strides = other.sym_strides();
  auto other_storage_offset = other.storage_offset();
  auto other_storage_numel = other.storage().sym_nbytes() / other.itemsize();

  if (self_num_batch_dims == 0) {
    auto new_tensor = pluss::zeros_symint({other_storage_numel}, other.options());
    return new_tensor.as_strided_symint(other_sizes, other_strides, other_storage_offset);
  }

  auto self_sizes = self.sym_sizes();

  // NB: We don't check that the sizes of self is the same as that of other
  //     because this function is also used in the inplace over view case
  //     In the inplace over view case we cannot rely on self and other being
  //     the same size. So we will use the size of other, and simply tack on
  //     the batch dims from self. For example: If self.sizes: [B, 2, 3],
  //     and other.size: [6], we return [B, 6].
  //     Also see the test test_inplace_on_view_not_same_layout, for when we reach
  //     this case.
  constexpr int64_t kSmallBufferSizeHint = 8;

  auto out_sizes = MSharp::SmallVector<MSharp::SymInt, kSmallBufferSizeHint>(other.dim() + self_num_batch_dims);
  std::copy(self_sizes.begin(), self_sizes.begin() + self_num_batch_dims, out_sizes.begin());
  std::copy(other_sizes.begin(), other_sizes.end(), out_sizes.begin() + self_num_batch_dims);

  // We use the strides of other, and tack on the strides computed with
  // the batch dims of self, so that the slices are arranged contiguously
  auto out_strides = MSharp::SmallVector<MSharp::SymInt, kSmallBufferSizeHint>(other.dim() + self_num_batch_dims);
  auto prod = other_storage_numel;

  for (int64_t i = self_num_batch_dims - 1; i >= 0; --i) {
    out_strides[i] = prod;
    prod *= self_sizes[i];
  }
  std::copy(other_strides.begin(), other_strides.end(), out_strides.begin() + self_num_batch_dims);

  auto storage_numel = prod;

  // Inherit the TensorOptions of the primal
  auto new_tensor = pluss::zeros_symint({storage_numel}, other.options());
  return new_tensor.as_strided_symint(out_sizes, out_strides, other_storage_offset);
}

bool _has_same_storage_numel(const pluss::Series& base, const pluss::Series& other) {
  return base.storage().sym_nbytes() / base.itemsize() == other.storage().sym_nbytes() / other.itemsize();
}

Series _lazy_clone(Series const& self) {
  MSharp::StorageImpl* self_storage = self.storage().unsafeGetStorageImpl();
  MSharp::intrusive_ptr<MSharp::StorageImpl> storage =
    MSharp::impl::cow::lazy_clone_storage(*self_storage);
  TORCH_CHECK(storage != nullptr);
  auto series = MSharp::make_intrusive<MSharp::TensorImpl>(
      MSharp::Storage(std::move(storage)),
      self.key_set(),
      self.dtype());
  series->set_sizes_and_strides(self.sym_sizes(),
                                self.sym_strides(),
                                self.sym_storage_offset());
  return Series(std::move(series));
}

} // namespace pluss::native
