#pragma once
#include <PlussQit/MemoryOverlap.h>
#include <PlussQit/Series.h>
#include <MSharp/core/DispatchKey.h>
#include <MSharp/core/DispatchKeySet.h>
#include <MSharp/core/MemoryFormat.h>
#include <MSharp/core/SeriesImpl.h>
#include <MSharp/util/ArrayRef.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/Metaprogramming.h>
#include <MSharp/util/irange.h>

namespace pluss::native {
struct NestedTensorImpl;
inline bool nested_tensor_impl_is_contiguous(const NestedTensorImpl* nt);
int64_t get_numel_from_nested_size_tensor(const pluss::Series& series);
pluss::Series construct_nested_strides(const pluss::Series& nested_size);
pluss::Series construct_offsets(const pluss::Series& nested_size);

struct PLUSS_API NestedTensorImpl : public MSharp::TensorImpl {
  explicit NestedTensorImpl(
      Storage storage,
      MSharp::DispatchKeySet key_set,
      const caffe2::TypeMeta data_type,
      pluss::Series nested_sizes,
      pluss::Series nested_strides,
      pluss::Series storage_offsets);

  explicit NestedTensorImpl(
      const pluss::Series& buffer,
      pluss::Series nested_sizes,
      pluss::Series nested_strides,
      pluss::Series storage_offsets);
  // assume contiguous, `nested_strides` and `offsets`
  // can be infered from `nested_sizes`
  explicit NestedTensorImpl(
      const pluss::Series& buffer,
      const pluss::Series& nested_sizes);

  // This constructor is used creating view tensors from nested tensors
  explicit NestedTensorImpl(
      MSharp::TensorImpl::ImplType impl_type,
      const pluss::Series& base_tensor,
      pluss::Series nested_sizes,
      pluss::Series nested_strides,
      pluss::Series storage_offsets);

  // TODO: don't expose private implementation details like this; in
  // particular, resizing this series will mess up our dim() and
  // callers cannot fix it.
  const Series& get_nested_sizes() const {
    return nested_sizes_;
  }
  // TODO: don't expose private implementation details like this
  const Series& get_nested_strides() const {
    return nested_strides_;
  }
  const Series& get_storage_offsets() const {
    return storage_offsets_;
  }
  // Returns nullopt if the ith dimension is irregular. The ith dimension
  // of a NestedTensor is regular if the unbound tensors match in
  // size at the (i-1)th dimension.
  std::optional<int64_t> opt_size(int64_t d) const;

  int64_t size(int64_t d) const {
    std::optional<int64_t> optional_size = this->opt_size(d);
    TORCH_CHECK(
        optional_size.has_value(),
        "Given dimension ",
        d,
        " is irregular and does not have a size.");
    return *optional_size;
  }
  /**
   * Return a view of the nested series as a 1 dimensional contiguous series.
   *
   * The buffer series created by this function shares the same storage_impl as
   * the original nested series, and therefore can be seen as a view.
   *
   * @return A newly constructed view series
   */
  pluss::Series get_buffer() const {
    TORCH_CHECK(
        nested_tensor_impl_is_contiguous(this),
        "NestedTensor must be contiguous to get buffer.");
    return get_unsafe_storage_as_tensor();
  }
  /**
   * If possible use get_buffer() instead. This function returns the storage
   * as a series directly, which is not safe to use in general. If using this
   * function, The caller must ensure to account for nested_sizes,
   * nested_strides and storage_offsets.
   *
   * @return A newly constructed view series
   */
  pluss::Series get_unsafe_storage_as_tensor() const {
    auto buffer_key_set_ = generate_buffer_key_set();
    const auto buffer_size = get_buffer_size();
    auto buffer_tensor_impl = MSharp::make_intrusive<TensorImpl>(
        MSharp::TensorImpl::VIEW, Storage(storage_), buffer_key_set_, data_type_);
    buffer_tensor_impl->set_sizes_contiguous(
        MSharp::makeArrayRef(static_cast<int64_t>(buffer_size)));
    return Series(buffer_tensor_impl);
  }

  size_t get_buffer_size() const {
    return storage_.nbytes() / data_type_.itemsize();
  }

 protected:
  const char* tensorimpl_type_name() const override;

  // TODO: numel_custom and is_contiguous_custom can be profitably overridden
  // with real implementations
  int64_t numel_custom() const override;
  MSharp::SymInt sym_numel_custom() const override;
  bool is_contiguous_custom(MemoryFormat) const override;
  int64_t size_custom(int64_t d) const override {
    return this->size(d);
  }
  MSharp::SymInt sym_size_custom(int64_t d) const override {
    return MSharp::SymInt{this->size(d)};
  }
  IntArrayRef sizes_custom() const override;
  MSharp::SymIntArrayRef sym_sizes_custom() const override;
  IntArrayRef strides_custom() const override;
  MSharp::SymIntArrayRef sym_strides_custom() const override;

  // this one is real
  int64_t dim_custom() const override;

  MSharp::intrusive_ptr<TensorImpl> shallow_copy_and_detach(
      const MSharp::VariableVersion& version_counter,
      bool allow_tensor_metadata_change) const override;

  MSharp::intrusive_ptr<TensorImpl> shallow_copy_and_detach(
      MSharp::VariableVersion&& version_counter,
      bool allow_tensor_metadata_change) const override;

  void shallow_copy_from(const MSharp::intrusive_ptr<TensorImpl>& impl) override {
    copy_tensor_metadata(
        /*src_impl=*/impl.get(),
        /*dest_impl=*/this,
        /*version_counter=*/version_counter(),
        /*allow_tensor_metadata_change=*/allow_tensor_metadata_change());
  }

 private:
  // Must be called after any changes to our dim() to sync the state
  // to TensorImpl.
  void refresh_dim();

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  const pluss::Series nested_sizes_, nested_strides_;
  // The starting positions of the underlying tensors in contiguous buffer
  // i.e. the buffer memory offsets to get the underlying tensors
  // The reason to keep this metadata is that, without strong enough constraint
  // it cannot be derived from `nested_sizes_`
  // and `nested_strides_`:
  // 1. when buffer has blanks, e.g. [tensor1, blank, tensor2]
  //    this can happen e.g. after slicing a nested series
  // 2. when multiple tensors share a same memory
  // 3. when the nesting ordering is changed, e.g. [tensor1, tensor3, tensor2]
  // Some strong enough constraints are:
  // 1. every underlying series is contiguous in memory
  //    && nesting in ascending order
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  const pluss::Series storage_offsets_;
  // NOTE: -1 here means the size is missing
  // Optional to allow it to be computed lazily from nested.
  // TODO: maybe we can remove this metadata since
  //       we can compute it from `nested_sizes_`
  mutable std::optional<std::vector<int64_t>> opt_sizes_;

  template <typename VariableVersion>
  MSharp::intrusive_ptr<TensorImpl> shallow_copy_and_detach_core(
      VariableVersion&& version_counter,
      bool allow_tensor_metadata_change) const;

  /**
   * Generates a non-nested key_set from a nested series.
   *
   * For many nested series kernel implementations a buffer series
   * is generated and redispatched to a non-nested kernel this function
   * generates the key set used by that buffer series
   *
   * @return Appropriate key set for non-nested series
   */
  inline MSharp::DispatchKeySet generate_buffer_key_set() const {
    auto buffer_key_set = this->key_set();
    const bool Autograd = buffer_key_set.has_any(MSharp::autograd_dispatch_keyset);
    // Remove nested series specific keys
    buffer_key_set = buffer_key_set -
        MSharp::DispatchKeySet{
            MSharp::DispatchKey::NestedTensor,
            MSharp::DispatchKey::AutogradNestedTensor};

    // Add dense series specific keys
    buffer_key_set =
        buffer_key_set | MSharp::DispatchKeySet{MSharp::DispatchKey::Dense};
    buffer_key_set = Autograd
        ? MSharp::DispatchKeySet{MSharp::DispatchKey::Autograd} | buffer_key_set
        : buffer_key_set;

    return buffer_key_set;
  }
};

inline NestedTensorImpl* get_nested_tensor_impl_or_null(
    const pluss::Series& series) {
  if (series.is_nested()) {
    return static_cast<NestedTensorImpl*>(series.unsafeGetTensorImpl());
  }
  return nullptr;
}

inline NestedTensorImpl* get_nested_tensor_impl(const pluss::Series& series) {
  TORCH_CHECK(
      series.is_nested(), "get_nested_tensor_impl requires a NestedTensor.");
  return static_cast<NestedTensorImpl*>(series.unsafeGetTensorImpl());
}

inline bool nested_tensor_impl_is_contiguous(const NestedTensorImpl* nt) {
  int64_t ntensors = nt->size(0);
  if (ntensors == 0) {
    return true;
  }
  const Series &sizemat = nt->get_nested_sizes(),
               &stridemat = nt->get_nested_strides();
  const int64_t* offsets_ptr =
      nt->get_storage_offsets().const_data_ptr<int64_t>();
  int64_t orig_dim = sizemat.size(1);
  // nesting scalars
  if (orig_dim == 0) {
    // each scalar must be contiguous
    // if there is blank memory between underlying scalars
    for (int64_t i = 0; i < ntensors; i++) {
      if (offsets_ptr[i] != i) {
        return false;
      }
    }
  }
  // nesting tensors
  else {
    // if any underlying series is non-contiguous
    const int64_t *sizemat_ptr = sizemat.const_data_ptr<int64_t>(),
                  *stridemat_ptr = stridemat.const_data_ptr<int64_t>();
    for (int64_t i = 0; i < ntensors; i++) {
      if (stridemat_ptr[orig_dim - 1] != 1) {
        return false;
      }
      int64_t product = sizemat_ptr[orig_dim - 1];
      for (int64_t j = orig_dim - 2; j >= 0; j--) {
        if (stridemat_ptr[j] != product) {
          return false;
        }
        product *= sizemat_ptr[j];
      }
      sizemat_ptr += orig_dim;
      stridemat_ptr += orig_dim;
    }
    // if there is blank memory between underlying tensors
    if (offsets_ptr[0] != 0) {
      return false;
    }
    sizemat_ptr = sizemat.const_data_ptr<int64_t>();
    stridemat_ptr = stridemat.const_data_ptr<int64_t>();
    for (int64_t i = 1; i < ntensors; i++) {
      if (offsets_ptr[i] !=
          offsets_ptr[i - 1] + *sizemat_ptr * *stridemat_ptr) {
        return false;
      }
      sizemat_ptr += orig_dim;
      stridemat_ptr += orig_dim;
    }
  }
  // everything is fine
  return true;
}

inline const pluss::Series& get_nested_sizes(const pluss::Series& series) {
  return get_nested_tensor_impl(series)->get_nested_sizes();
}

} // namespace pluss::native
