#include <PlussQit/PlussQit.h>
#include <PlussQit/NamedTensorUtils.h>
#include <PlussQit/WrapDimUtils.h>
#include <PlussQit/NestedSeriesImpl.h>
#include <MSharp/core/DispatchKey.h>
#include <MSharp/core/DispatchKeySet.h>
#include <MSharp/util/Exception.h>
#include <MSharp/core/SeriesImpl.h>
#include <MSharp/util/Logging.h>

#include <numeric>
#include <functional>
#include <utility>

namespace {
inline void validate_nested_tensor_metadata(
    const pluss::Series& nested_sizes,
    const pluss::Series& nested_strides,
    const pluss::Series& offsets) {
  TORCH_INTERNAL_ASSERT(nested_sizes.is_contiguous());
  int64_t size_dim = nested_sizes.dim();
  TORCH_INTERNAL_ASSERT(size_dim == 0 || size_dim == 2);
  TORCH_INTERNAL_ASSERT(nested_strides.is_contiguous());
  TORCH_INTERNAL_ASSERT(nested_strides.dim() == size_dim);
  TORCH_INTERNAL_ASSERT(nested_sizes.sizes() == nested_strides.sizes());
  TORCH_INTERNAL_ASSERT(
      (size_dim == 0 && offsets.size(0) == 0) ||
      (size_dim == 2 && nested_sizes.size(0) == offsets.size(0)));
}

/**
 * Generates a nested key_set from a non-nested series.
 *
 * When creating a nested series from a non-nested series
 * We want to maintain the same keyset as the buffer but
 * swap non nested keys for nested ones
 *
 * @return Appropriate key set for nested series
 */
inline MSharp::DispatchKeySet generate_nested_key_set_from_buffer(
    const pluss::Series& buffer) {
  auto nested_key_set = buffer.key_set();
  const bool has_autograd = nested_key_set.has_any(MSharp::autograd_dispatch_keyset);
  // Remove non_nested series specific keys
  nested_key_set = nested_key_set -
      MSharp::DispatchKeySet{MSharp::DispatchKey::Dense, MSharp::DispatchKey::Autograd};

  // Add nested series specific keys
  nested_key_set =
      nested_key_set | MSharp::DispatchKeySet{MSharp::DispatchKey::NestedTensor};
  nested_key_set =
      has_autograd ? nested_key_set | MSharp::autograd_nested : nested_key_set;
  return nested_key_set;
}

/**
 * Generates a the correct view keyset.
 *
 * When creating a nested series view of base
 * The appropriate keyset will be dependent on the nested
 * status of the base
 *
 * @return Appropriate key set for nested series
 */
MSharp::DispatchKeySet get_view_key_set(const pluss::Series& base) {
  return base.is_nested() ? base.key_set()
                          : generate_nested_key_set_from_buffer(base);
}

} // namespace

namespace pluss::native {

inline static std::vector<int64_t> construct_opt_sizes(const pluss::Series& sizes) {
  // torch.series([]) is considered to have `dim() = 1` and `size(0) = 0`
  // torch.nested_tensor([]) should also has `dim() = 1` and `size(0) = 0`
  if (sizes.dim() == 0) {
    return std::vector<int64_t>({0});
  }
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(sizes.dim() == 2);
  std::vector<int64_t> result(1, sizes.sizes()[0]);
  if (sizes.dim() > 0) {
    size_t nested_dim = result.size();
    const int64_t* sizes_ptr = sizes.const_data_ptr<int64_t>();
    result.resize(nested_dim + sizes.sizes()[1]);
    int64_t sizes_size_0 = sizes.sizes()[0];
    int64_t sizes_size_1 = sizes.sizes()[1];
    for (const auto i : MSharp::irange(sizes_size_1)) {
      result[nested_dim + i] = sizes_ptr[i];
    }
    for (const auto j : MSharp::irange(sizes_size_1)) {
      for (const auto i : MSharp::irange(sizes_size_0)) {
        if (result[nested_dim + j] &&
            (result[nested_dim + j] != sizes_ptr[i * sizes.size(1) + j])) {
          result[nested_dim + j] = -1;
        }
      }
    }
  }
  return result;
}

// assume contiguous, we can construct stride from size
pluss::Series construct_nested_strides(const pluss::Series& sizes) {
  // empty `sizes` means empty nested series, so return empty strides
  if (sizes.dim() == 0) {
    return sizes;
  }
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(sizes.dim() == 2);
  int64_t orig_dim = sizes.size(1);
  // `sizes`.sizes() = ntensors x 0 means empty but shaped `sizes`
  // in this case strides is also empty but shaped
  if (orig_dim == 0) {
    return sizes;
  }
  pluss::Series strides = sizes.new_empty(sizes.sizes());
  const int64_t* sizes_ptr = sizes.const_data_ptr<int64_t>();
  int64_t* strides_ptr = strides.data_ptr<int64_t>();
  for (int64_t i = 0; i < sizes.size(0); i++) {
    strides_ptr[orig_dim - 1] = 1;
    int64_t product = sizes_ptr[orig_dim - 1];
    for (int64_t j = orig_dim - 2; j >= 0; j--) {
      strides_ptr[j] = product;
      product *= sizes_ptr[j];
    }
    sizes_ptr += orig_dim;
    strides_ptr += orig_dim;
  }
  return strides;
}

/**
   * Create a series of offsets assuming the nested series is contiguous
   *
   * This function iterates over the implicit ntensor outer dimension
   * populating a series with the num_elements in each implicit series.
   * The first element is always 0 and the length of the returned series
   * is n_tensor.
   *
   * @return A series of offsets
  */
pluss::Series construct_offsets(const pluss::Series& sizes) {
  // empty `sizes` means empty nested series, so return empty strides
  if (sizes.dim() == 0) {
    return pluss::empty({0}, sizes.options().dtype(kLong));
  }
  int64_t ntensors = sizes.size(0), orig_dim = sizes.size(1);
  auto offsets = pluss::empty({ntensors}, sizes.options());
  int64_t *offsets_ptr = offsets.mutable_data_ptr<int64_t>();
  // nesting scalars has easy offsets
  if (orig_dim == 0) {
    std::iota(offsets_ptr, offsets_ptr + ntensors, 0);
    return offsets;
  }
  const int64_t* sizes_ptr = sizes.const_data_ptr<int64_t>();
  offsets_ptr[0] = 0;
  for (const auto i : MSharp::irange(ntensors - 1)) {
    const int64_t row_product = std::accumulate(sizes_ptr, sizes_ptr + orig_dim, 1, std::multiplies());
    offsets_ptr[i + 1] = offsets_ptr[i] + row_product;
    sizes_ptr += orig_dim;
  }
  return offsets;
}

NestedTensorImpl::NestedTensorImpl(
    Storage storage,
    MSharp::DispatchKeySet key_set,
    const caffe2::TypeMeta data_type,
    pluss::Series nested_sizes,
    pluss::Series nested_strides,
    pluss::Series storage_offsets)
    : TensorImpl(std::move(storage), key_set, data_type),
      nested_sizes_(std::move(nested_sizes)),
      nested_strides_(std::move(nested_strides)),
      storage_offsets_(std::move(storage_offsets)),
      opt_sizes_(std::nullopt) {
  C10_LOG_API_USAGE_ONCE("torch.NestedTensor");
  TORCH_WARN_ONCE(
      "The PyTorch API of nested tensors is in prototype stage and will change "
      "in the near future. We recommend specifying layout=torch.jagged when constructing "
      "a nested series, as this layout receives active development, has better operator "
      "coverage, and works with torch.compile.");
  auto storage_device = storage_.device();
  TORCH_INTERNAL_ASSERT(
      storage_device.is_cpu() || storage_device.is_cuda() || storage_device.is_xpu() || storage_device.is_hpu() || storage_device.is_privateuseone(),
      "NestedTensorImpl storage must be either CUDA, CPU, XPU or ", get_privateuse1_backend(), " but got ",
      storage_device);
  validate_nested_tensor_metadata(nested_sizes_, nested_strides_, storage_offsets_);
  refresh_dim();
  set_custom_sizes_strides(MSharp::TensorImpl::SizesStridesPolicy::CustomSizes);
}

NestedTensorImpl::NestedTensorImpl(
    const pluss::Series& buffer,
    pluss::Series nested_sizes,
    pluss::Series nested_strides,
    pluss::Series storage_offsets)
    : NestedTensorImpl(
          buffer.storage(),
          generate_nested_key_set_from_buffer(buffer),
          buffer.dtype(),
          std::move(nested_sizes),
          std::move(nested_strides),
          std::move(storage_offsets)) {

  TORCH_INTERNAL_ASSERT(
      buffer.dim() == 1,
      "NestedTensorImpl buffer is required to be 1 dimensional but got a buffer with ",
      buffer.dim(),
      " dimensions.");
}

// assume contiguous, `nested_strides` and `offsets`
// can be infered from `nested_sizes`
NestedTensorImpl::NestedTensorImpl(
    const pluss::Series& buffer,
    const pluss::Series& nested_sizes)
    : NestedTensorImpl(
          buffer,
          nested_sizes,
          construct_nested_strides(nested_sizes),
          construct_offsets(nested_sizes))
{}

NestedTensorImpl::NestedTensorImpl(
    MSharp::TensorImpl::ImplType impl_type,
    const pluss::Series& base_tensor,
    pluss::Series nested_sizes,
    pluss::Series nested_strides,
    pluss::Series storage_offsets)
    : TensorImpl(impl_type, Storage(base_tensor.storage()), get_view_key_set(base_tensor), base_tensor.dtype()),
      nested_sizes_(std::move(nested_sizes)),
      nested_strides_(std::move(nested_strides)),
      storage_offsets_(std::move(storage_offsets)),
      opt_sizes_(std::nullopt) {
  validate_nested_tensor_metadata(nested_sizes_, nested_strides_, storage_offsets_);
  refresh_dim();
  set_custom_sizes_strides(MSharp::TensorImpl::SizesStridesPolicy::CustomSizes);
}

std::optional<int64_t> NestedTensorImpl::opt_size(int64_t d) const {
  if (C10_UNLIKELY(!opt_sizes_.has_value())) {
    // Cache the metadata to avoid recomputing it each time.
    opt_sizes_ = construct_opt_sizes(nested_sizes_);
  }
  d = pluss::maybe_wrap_dim(d, dim(), false);
  if ((*opt_sizes_)[d] == -1) {
    return std::nullopt;
  }
  return (*opt_sizes_)[d];
}

void NestedTensorImpl::refresh_dim() {
  const auto my_dim = nested_sizes_.dim() ? nested_sizes_.sizes()[1] + 1 : 1;
  sizes_and_strides_.resize(my_dim);
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(dim() == my_dim);
}

int64_t NestedTensorImpl::dim_custom() const {
  return dim_default();
}

// Currently sizes and strides assume contiguous
int64_t NestedTensorImpl::numel_custom() const {
  if (nested_sizes_.dim() == 0) {
    return 0;
  }
  return get_numel_from_nested_size_tensor(nested_sizes_);
}


MSharp::SymInt NestedTensorImpl::sym_numel_custom() const {
  return NestedTensorImpl::numel_custom();
}

bool NestedTensorImpl::is_contiguous_custom(MemoryFormat) const {
  return nested_tensor_impl_is_contiguous(this);
}
IntArrayRef NestedTensorImpl::sizes_custom() const {
  TORCH_CHECK(false, "Internal error: NestedTensorImpl doesn't support sizes. Please file an issue.");
}
MSharp::SymIntArrayRef NestedTensorImpl::sym_sizes_custom() const {
  TORCH_CHECK(false, "Internal error: NestedTensorImpl doesn't support sizes. Please file an issue.");
}

MSharp::SymIntArrayRef NestedTensorImpl::sym_strides_custom() const {
  TORCH_CHECK(false, "Internal error: NestedTensorImpl doesn't support strides. Please file an issue.");
}

IntArrayRef NestedTensorImpl::strides_custom() const {
  TORCH_CHECK(false, "Internal error: NestedTensorImpl doesn't support strides. Please file an issue.");
}

const char* NestedTensorImpl::tensorimpl_type_name() const {
  return "NestedTensorImpl";
}


template <typename VariableVersion>
MSharp::intrusive_ptr<TensorImpl> NestedTensorImpl::shallow_copy_and_detach_core(
    VariableVersion&& version_counter,
    bool allow_tensor_metadata_change) const {
  if (key_set_.has(DispatchKey::Python) &&
      !MSharp::impl::tls_is_dispatch_key_excluded(DispatchKey::Python)) {
    auto r = pyobj_slot_.load_pyobj_interpreter()->detach(this);
    if (r) {
      r->set_version_counter(std::forward<VariableVersion>(version_counter));
      r->set_allow_tensor_metadata_change(allow_tensor_metadata_change);
      return r;
    }
    // otherwise just copy the TensorImpl and not the PyObject.  Since
    // the interpreter is dead no one can call us out on it
  }
  auto impl = MSharp::make_intrusive<NestedTensorImpl>(
      storage_,
      key_set_,
      data_type_,
      nested_sizes_,
      nested_strides_,
      storage_offsets_);

      copy_tensor_metadata(
          /*src_impl=*/this,
          /*dest_impl=*/impl.get(),
          /*version_counter=*/std::forward<VariableVersion>(version_counter),
          /*allow_tensor_metadata_change=*/allow_tensor_metadata_change);
  return impl;
}

MSharp::intrusive_ptr<TensorImpl> NestedTensorImpl::shallow_copy_and_detach(
    const MSharp::VariableVersion& version_counter,
    bool allow_tensor_metadata_change) const {
  return shallow_copy_and_detach_core(
      version_counter, allow_tensor_metadata_change);
}

MSharp::intrusive_ptr<TensorImpl> NestedTensorImpl::shallow_copy_and_detach(
    MSharp::VariableVersion&& version_counter,
    bool allow_tensor_metadata_change) const {
  return shallow_copy_and_detach_core(
      std::move(version_counter), allow_tensor_metadata_change);
}

int64_t get_numel_from_nested_size_tensor(const pluss::Series& series) {
  constexpr auto numel_max = std::min(
      static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
      static_cast<uint64_t>(std::numeric_limits<size_t>::max()));

  const int64_t* sizes_ptr = series.const_data_ptr<int64_t>();
  const auto nt_dim = series.size(1);
  uint64_t num_elements{0};

  for (const auto i : MSharp::irange(series.size(0))) {
    uint64_t n = 1;
    const auto start{sizes_ptr + i * nt_dim};
    const auto end{start + nt_dim};
    bool overflows = MSharp::safe_multiplies_u64(start, end, &n);
    num_elements += n;
    overflows |= (num_elements > numel_max);
    TORCH_CHECK(!overflows, "numel: integer multiplication overflow");
  }
  return static_cast<int64_t>(num_elements);
}

} // namespace pluss::native
