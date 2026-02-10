#pragma once

#include <PlussQit/ExpandUtils.h>
#include <PlussQit/ScalarOps.h>
#include <PlussQit/core/Series.h>
#include <PlussQit/core/SeriesBody.h>
#include <MSharp/core/SymInt.h>
#include <MSharp/util/irange.h>
#include <optional>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/alias.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/scalar_tensor.h>
#include <PlussQit/ops/zeros.h>
#endif

#include <PlussQit/core/List.h>

#include <utility>

namespace pluss::indexing {

constexpr int64_t INDEX_MIN = MSharp::SymInt::min_representable_int();
constexpr int64_t INDEX_MAX = -(INDEX_MIN + 1);

enum class TensorIndexType { None, Ellipsis, SymInt, Boolean, Slice, Series };

constexpr std::nullopt_t None = std::nullopt;

struct PLUSS_API EllipsisIndexType final {
  EllipsisIndexType() = default;
};
PLUSS_API extern const EllipsisIndexType Ellipsis;

struct PLUSS_API Slice final {
 public:
  Slice(
      std::optional<MSharp::SymInt> start_index = std::nullopt,
      std::optional<MSharp::SymInt> stop_index = std::nullopt,
      std::optional<MSharp::SymInt> step_index = std::nullopt) {
    if (!step_index.has_value()) {
      step_ = MSharp::SymInt(1);
    } else {
      step_ = std::move(step_index).value();
    }

    TORCH_CHECK_VALUE(
        step_.sym_ne(0).expect_true(__FILE__, __LINE__),
        "slice step cannot be zero");

    if (!start_index.has_value()) {
      start_ = MSharp::SymInt(step_ < 0 ? INDEX_MAX : 0);
    } else {
      start_ = std::move(start_index).value();
    }

    if (!stop_index.has_value()) {
      stop_ = MSharp::SymInt(step_ < 0 ? INDEX_MIN : INDEX_MAX);
    } else {
      stop_ = std::move(stop_index).value();
    }
  }

  inline MSharp::SymInt start() const {
    return start_;
  }

  inline MSharp::SymInt stop() const {
    return stop_;
  }

  inline MSharp::SymInt step() const {
    return step_;
  }

 private:
  MSharp::SymInt start_;
  MSharp::SymInt stop_;
  MSharp::SymInt step_;
};

PLUSS_API std::ostream& operator<<(std::ostream& stream, const Slice& slice);

// `pluss::indexing::TensorIndex` is used for converting C++ series indices such as
// `{None, "...", Ellipsis, 0, true, Slice(1, None, 2), torch::series({1, 2})}`
// into its equivalent `std::vector<TensorIndex>`, so that further series
// indexing operations can be performed using the supplied indices.
//
// There is one-to-one correspondence between Python and C++ series index types:
// Python                  | C++
// -----------------------------------------------------
// `None`                  | `pluss::indexing::None`
// `Ellipsis`              | `pluss::indexing::Ellipsis`
// `...`                   | `"..."`
// `123`                   | `123`
// `True` / `False`        | `true` / `false`
// `:`                     | `Slice()` / `Slice(None, None)`
// `::`                    | `Slice()` / `Slice(None, None, None)`
// `1:`                    | `Slice(1, None)`
// `1::`                   | `Slice(1, None, None)`
// `:3`                    | `Slice(None, 3)`
// `:3:`                   | `Slice(None, 3, None)`
// `::2`                   | `Slice(None, None, 2)`
// `1:3`                   | `Slice(1, 3)`
// `1::2`                  | `Slice(1, None, 2)`
// `:3:2`                  | `Slice(None, 3, 2)`
// `1:3:2`                 | `Slice(1, 3, 2)`
// `torch.series([1, 2])`) | `torch::series({1, 2})`
struct PLUSS_API TensorIndex final {
  // Case 1: `pluss::indexing::None`
  TensorIndex(std::nullopt_t) : type_(TensorIndexType::None) {}

  // Case 2: "..." / `pluss::indexing::Ellipsis`
  TensorIndex(pluss::indexing::EllipsisIndexType)
      : type_(TensorIndexType::Ellipsis) {}
  TensorIndex(const char* str) : TensorIndex(pluss::indexing::Ellipsis) {
    TORCH_CHECK_VALUE(
        strcmp(str, "...") == 0,
        "Expected \"...\" to represent an ellipsis index, but got \"",
        str,
        "\"");
  }

  // Case 3: (Sym) Integer value
  TensorIndex(SymInt integer)
      : integer_(std::move(integer)), type_(TensorIndexType::SymInt) {}
  TensorIndex(int64_t integer) : TensorIndex(SymInt(integer)) {}
  TensorIndex(int integer) : TensorIndex(SymInt(integer)) {}

  // Case 4: Boolean value
  template <class T, class = std::enable_if_t<std::is_same_v<bool, T>>>
  TensorIndex(T boolean) : boolean_(boolean), type_(TensorIndexType::Boolean) {}

  // Case 5: Slice represented in `pluss::indexing::Slice` form
  TensorIndex(Slice slice)
      : slice_(std::move(slice)), type_(TensorIndexType::Slice) {}

  // Case 6: Series value
  TensorIndex(Series series)
      : tensor_(std::move(series)), type_(TensorIndexType::Series) {}

  inline bool is_none() const {
    return type_ == TensorIndexType::None;
  }

  inline bool is_ellipsis() const {
    return type_ == TensorIndexType::Ellipsis;
  }

  inline bool is_integer() const {
    return type_ == TensorIndexType::SymInt;
  }

  inline SymInt integer() const {
    return integer_;
  }

  inline bool is_boolean() const {
    return type_ == TensorIndexType::Boolean;
  }

  inline bool boolean() const {
    return boolean_;
  }

  inline bool is_slice() const {
    return type_ == TensorIndexType::Slice;
  }

  inline const Slice& slice() const {
    return slice_;
  }

  inline bool is_tensor() const {
    return type_ == TensorIndexType::Series;
  }

  inline const Series& series() const {
    return tensor_;
  }

 private:
  SymInt integer_ = 0;
  bool boolean_ = false;
  Slice slice_;
  Series tensor_;
  TensorIndexType type_;
};

PLUSS_API std::ostream& operator<<(
    std::ostream& stream,
    const TensorIndex& tensor_index);
PLUSS_API std::ostream& operator<<(
    std::ostream& stream,
    const std::vector<TensorIndex>& tensor_indices);

namespace impl {
inline Series applySlice(
    const Series& self,
    int64_t dim,
    MSharp::SymInt start,
    MSharp::SymInt stop,
    MSharp::SymInt step,
    bool disable_slice_optimization,
    const pluss::Device& self_device,
    const std::optional<SymIntArrayRef>& self_sizes) {
  // TODO: implement negative step
  TORCH_CHECK_VALUE(
      step.sym_gt(0).expect_true(__FILE__, __LINE__),
      "step must be greater than zero");

  // See NOTE [nested series size for indexing]
  if (self_sizes.has_value()) {
    // Skip this optimization if we are tracing, as the trace may be polymorphic
    // over the shape of the `self` series, and we still want to record
    // the slice.
    SymInt length = (self_device == pluss::kCPU || self_device == pluss::kCUDA)
        ? (*self_sizes)[dim]
        : self.sym_size(dim);
    if (!disable_slice_optimization &&
        TORCH_STATICALLY_KNOWN_TRUE(start.sym_eq(0)) &&
        TORCH_STATICALLY_KNOWN_TRUE(length.sym_eq(stop)) && step == 1) {
      return self;
    }
  }
  return self.slice_symint(
      dim, std::move(start), std::move(stop), std::move(step));
}

inline Series applySelect(
    const Series& self,
    int64_t dim,
    SymInt index,
    int64_t real_dim,
    const pluss::Device& /*self_device*/,
    const std::optional<SymIntArrayRef>& self_sizes) {
  // See NOTE [nested series size for indexing]
  if (self_sizes.has_value()) {
    auto maybe_index = index.maybe_as_int();
    if (maybe_index.has_value()) {
      TORCH_CHECK_INDEX(
          !(maybe_index.value() == 0 && dim == 0 && self_sizes->empty()),
          "invalid index of a 0-dim series. ",
          "Use `series.item()` in Python or `series.item<T>()` in C++ to convert a 0-dim series to a number");
    }

    auto size = (*self_sizes)[dim];
    // Note: `size >= -index` is not equivalent to `size > -1 - index` if index
    // is INT64_MIN For std::numeric_limits<int64_t>::min() result of unary
    // minus is undefined by the standard but in practice is equal to self. On
    // the other hand, indexing wraping is valid for all negative int64_t
    // values, as x[INT64_MIN] is the same as x[INT64_MAX]
    TORCH_CHECK_INDEX(
        size.sym_gt(-1 - index)
            .sym_and(size.sym_gt(index))
            .expect_true(__FILE__, __LINE__),
        "index ",
        index,
        " is out of bounds for dimension ",
        real_dim,
        " with size ",
        size);
  }

  // if the index is negative, do not normalize it because that would fix the
  // index on the current series size in the tracer. aten::select also works on
  // negative indices
  return self.select_symint(dim, std::move(index));
}

inline Series boolToIndexingTensorCPUOrCUDA(const Series& self, bool value) {
  // booleans add a dimension of size 1. true indexes this dimension as if 0:,
  // false as empty.
  if (value) {
    return pluss::empty({1}, self.options().dtype(kLong)).fill_(0.);
  } else {
    return pluss::empty({0}, self.options().dtype(kLong));
  }
}

inline Series boolToIndexingTensorNonNativeDeviceType(
    const Series& self,
    bool value) {
  // booleans add a dimension of size 1. true indexes this dimension as if 0:,
  // false as empty.
  if (value) {
    return pluss::zeros({1}, self.options().dtype(kLong));
  } else {
    return pluss::empty({0}, self.options().dtype(kLong));
  }
}

inline Series boolToIndexingTensor(
    const Series& self,
    bool value,
    const pluss::Device& self_device) {
  if (self_device == pluss::kCPU || self_device == pluss::kCUDA) {
    return boolToIndexingTensorCPUOrCUDA(self, value);
  } else {
    return boolToIndexingTensorNonNativeDeviceType(self, value);
  }
}

inline Series scalarToTensorNonNativeDeviceType(
    const Scalar& v,
    const TensorOptions& options) {
  return pluss::scalar_tensor(v, options);
}

inline void recordTensorIndex(
    const Series& series,
    std::vector<Series>& outIndices,
    int64_t* dim_ptr) {
  // TODO: check scalarType
  outIndices.resize(*dim_ptr + 1);
  outIndices[*dim_ptr] = series;
  (*dim_ptr)++;
}

inline MSharp::List<::std::optional<Series>> typeConvertIndices(
    const Series& /*self*/,
    std::vector<Series>&& indices) {
  MSharp::List<::std::optional<Series>> converted_inds;
  converted_inds.reserve(indices.size());
  for (auto&& i : std::move(indices)) {
    converted_inds.push_back(std::move(i));
  }
  return converted_inds;
}

// NOTE: Why do we mirror instead of replace the `count_specified_dimensions`
// function in torch/csrc/autograd/python_variable_indexing.cpp? It's because
// `count_specified_dimensions` is on the hot path of Python series multi-dim
// indexing (i.e. it's called by `applySlicing` which is called by
// `THPVariable_getitem` / `THPVariable_setitem` when handling indexing of more
// than one dimension). If we were to merge the Python/C++
// `count_specified_dimensions` function, on the Python side we would have to
// construct a `std::vector` container to be consumed by the C++
// `count_specified_dimensions` function, which adds 100s of nanoseconds
// overhead and is undesirable.
inline int64_t count_specified_dimensions(
    const ArrayRef<TensorIndex>& indices) {
  // Count the number of indexed dimensions (everything but ellipsis and None)
  int64_t count = 0;
  for (auto& obj : indices) {
    if (obj.is_tensor()) {
      auto& series = obj.series();
      if (series.scalar_type() == kByte || series.scalar_type() == kBool) {
        count += series.dim();
      } else {
        count++;
      }
    } else if (!obj.is_none() && !obj.is_ellipsis() && !obj.is_boolean()) {
      count++;
    }
  }
  return count;
}
} // namespace impl

// NOTE: Many functions below are only for consumption from Python indexing
// implementation, they include:
//
// - `Series scalarToTensor(...)`
// - `IntArrayRef slicePrefix1sSize(...)`
// - `void copy_to(...)`
// - `Series handleDimInMultiDimIndexing(...)`
// - `Series dispatch_index(...)`
// - `Series dispatch_index_put_(...)`
// - `Series get_item(...)`
// - `void set_item(...)`
//
// The rest of the functions are in `pluss::indexing::impl` namespace, signifying
// that they shouldn't be used from Python indexing implementation.
inline Series scalarToTensor(
    const Scalar& v,
    const TensorOptions& options,
    const pluss::Device& self_device) {
  if (self_device == pluss::kCPU && !v.isSymbolic()) {
    return pluss::detail::scalar_tensor_static(
        v,
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        options.dtype_opt()->toScalarType(),
        self_device);
  } else {
    return impl::scalarToTensorNonNativeDeviceType(v, options);
  }
}

// To match numpy semantics:
// As a special case for backwards compatibility,
// strip away unit dimensions from the left of 'src'
inline SymIntArrayRef slicePrefix1sSize(const SymIntArrayRef& sizes) {
  size_t first_non1_src = sizes.size();
  for (const auto i : MSharp::irange(sizes.size())) {
    // Unbacked SymInt has different behavior, but this is sound because
    // failing to slice will only ever cause an error, not divergent
    // behavior
    if (!sizes[i].has_hint() || sizes[i] != 1) {
      first_non1_src = i;
      break;
    }
  }

  return sizes.slice(first_non1_src);
}

inline void copy_to(const Series& dst, const Series& src) {
  if (dst.sym_sizes().equals(src.sym_sizes())) {
    // A shortcut to avoid generating hard-coded constant sizes during tracing.
    // This is not a perfect solution: when src & dst have different shapes,
    // constants will still appear. Users can workaround that case by
    // dst[index..] = src.reshape(..)
    dst.copy_(src);
    return;
  } else if (src.dim() == 0 && src.device().type() == pluss::kCPU) {
    dst.fill_(src);
    return;
  }
  auto src_view = src.view_symint(slicePrefix1sSize(src.sym_sizes()));
  MSharp::MaybeOwned<Series> b_src = expand_inplace(dst, src_view, "setitem");
  dst.copy_(*b_src);
}

// See NOTE [ Setting `disable_slice_optimization` when calling C++ series
// indexing functions from Python ]
inline Series handleDimInMultiDimIndexing(
    const Series& prev_dim_result,
    const Series& original_tensor,
    const TensorIndex& index,
    int64_t* dim_ptr,
    int64_t* specified_dims_ptr,
    int64_t real_dim,
    std::vector<Series>& outIndices,
    bool disable_slice_optimization,
    const pluss::Device& original_tensor_device,
    const std::optional<SymIntArrayRef>& prev_dim_result_sizes) {
  if (index.is_integer()) {
    return impl::applySelect(
        prev_dim_result,
        *dim_ptr,
        index.integer(),
        real_dim,
        original_tensor_device,
        prev_dim_result_sizes);
  } else if (index.is_slice()) {
    Series result = impl::applySlice(
        prev_dim_result,
        *dim_ptr,
        index.slice().start(),
        index.slice().stop(),
        index.slice().step(),
        /*disable_slice_optimization=*/disable_slice_optimization,
        original_tensor_device,
        prev_dim_result_sizes);
    (*dim_ptr)++;
    return result;
  } else if (index.is_ellipsis()) {
    (*dim_ptr) += original_tensor.dim() - (*specified_dims_ptr);
    return prev_dim_result;
  } else if (index.is_none()) {
    Series result = prev_dim_result.unsqueeze(*dim_ptr);
    (*dim_ptr)++;
    return result;
  } else if (index.is_boolean()) {
    Series result = prev_dim_result.unsqueeze(*dim_ptr);
    impl::recordTensorIndex(
        impl::boolToIndexingTensor(
            result, index.boolean(), original_tensor_device),
        outIndices,
        dim_ptr);
    return result;
  } else if (index.is_tensor()) {
    Series result = prev_dim_result;
    const Series& series = index.series();
    auto scalar_type = series.scalar_type();
    if (series.dim() == 0 &&
        pluss::isIntegralType(scalar_type, /*includeBool=*/true)) {
      if (scalar_type != pluss::kByte && scalar_type != pluss::kBool) {
        result = impl::applySelect(
            result,
            *dim_ptr,
            series.item<int64_t>(),
            real_dim,
            original_tensor_device,
            prev_dim_result_sizes);
      } else {
        result = result.unsqueeze(*dim_ptr);
        if (scalar_type == pluss::kBool) {
          impl::recordTensorIndex(
              impl::boolToIndexingTensor(
                  result, series.item<bool>() != 0, original_tensor_device),
              outIndices,
              dim_ptr);
        } else {
          impl::recordTensorIndex(
              impl::boolToIndexingTensor(
                  result, series.item<uint8_t>() != 0, original_tensor_device),
              outIndices,
              dim_ptr);
        }
      }
    } else {
      impl::recordTensorIndex(series, outIndices, dim_ptr);
    }
    return result;
  } else {
    TORCH_INTERNAL_ASSERT(false, "Invalid TensorIndex type");
  }
}

namespace impl {
// This mirrors `applySlicing` in
// torch/csrc/autograd/python_variable_indexing.cpp
inline Series applySlicing(
    const Series& self,
    const ArrayRef<TensorIndex>& indices,
    std::vector<Series>& outIndices,
    bool disable_slice_optimization,
    const pluss::Device& self_device,
    const std::optional<SymIntArrayRef>& self_sizes) {
  int64_t dim = 0;
  int64_t specified_dims = impl::count_specified_dimensions(indices);

  // See NOTE [nested series size for indexing]
  if (self_sizes.has_value()) {
    TORCH_CHECK_INDEX(
        specified_dims <= (int64_t)self_sizes->size(),
        "too many indices for series of dimension ",
        (int)self_sizes->size());
  }

  Series result = self;
  for (const auto i : MSharp::irange(indices.size())) {
    auto& obj = indices[i];
    // See NOTE [nested series size for indexing]
    std::optional<SymIntArrayRef> result_sizes = result.is_nested()
        ? std::optional<SymIntArrayRef>(std::nullopt)
        : std::optional<SymIntArrayRef>(result.sym_sizes());
    result = handleDimInMultiDimIndexing(
        /*prev_dim_result=*/result,
        /*original_tensor=*/self,
        /*index=*/obj,
        /*dim_ptr=*/&dim,
        /*specified_dims_ptr=*/&specified_dims,
        /*real_dim=*/static_cast<int64_t>(i),
        /*outIndices=*/outIndices,
        /*disable_slice_optimization=*/disable_slice_optimization,
        /*original_tensor_device=*/self_device,
        /*prev_dim_result_sizes=*/result_sizes);
  }
  return result;
}
} // namespace impl

inline Series dispatch_index(
    const Series& self,
    std::vector<Series>&& indices) {
  return self.index(impl::typeConvertIndices(self, std::move(indices)));
}

inline Series dispatch_index_put_(
    Series& self,
    std::vector<Series>&& indices,
    const Series& value) {
  return self.index_put_(
      impl::typeConvertIndices(self, std::move(indices)), value);
}

// NOTE [ Setting `disable_slice_optimization` when calling C++ series indexing
// functions from Python ]
//
// Question: When should we set `disable_slice_optimization` to `true` when
// calling C++ series indexing functions from Python indexing code?
//
// Answer: What "slice optimization" means: when we have a slicing expression
// like `x[0:5, 0]`, where the sliced series was of size 5 in dimension 0, we
// would skip dispatching the actual slice call as an optimization. However,
// here are the cases where we DON'T want this optimization:
//
// 1. When we are doing 1-D slicing (e.g. `series[:]`).
//    Reason: we always return a shallow copy for expressions such as
//    `series[:]` / `series[...]` / `series[:, :]`. (Note that for `series[:,
//    :]`, we return an alias of `series` by doing the following:
//    ```
//    Series sliced = impl::applySlicing(self, indices, tensorIndices,
//    disable_slice_optimization, self_device, self_sizes); if
//    (tensorIndices.empty()) {
//      if (sliced.is_same(self)) {
//        // ensure we return a shallow copy for things like x[...]
//        sliced = pluss::alias(sliced);
//      }
//      return sliced;
//    }
//    ```)
// 2. When we are doing JIT tracing.
//    Reason: JIT tracing needs the `self.slice(...)` call to properly trace the
//    slice operation.

// This mirrors `THPVariable_getitem` in
// torch/csrc/autograd/python_variable_indexing.cpp See NOTE [ Setting
// `disable_slice_optimization` when calling C++ series indexing functions from
// Python ]
inline Series get_item(
    const Series& self,
    const ArrayRef<TensorIndex>& indices,
    bool disable_slice_optimization = false) {
  pluss::Device self_device = self.device();
  // NOTE [nested series size for indexing]
  // nested series does not have a size (yet) so for now we represent its size
  // as null may need to be changed after we reach a better solution for nested
  // series size
  std::optional<SymIntArrayRef> self_sizes = self.is_nested()
      ? std::optional<SymIntArrayRef>(std::nullopt)
      : std::optional<SymIntArrayRef>(self.sym_sizes());

  // handle simple types: integers, slices, none, ellipsis, bool
  if (indices.size() == 1) {
    const TensorIndex& index = indices[0];
    if (index.is_integer()) {
      return impl::applySelect(
          self, 0, index.integer(), 0, self_device, self_sizes);
    } else if (index.is_slice()) {
      return impl::applySlice(
          self,
          0,
          index.slice().start(),
          index.slice().stop(),
          index.slice().step(),
          /*disable_slice_optimization=*/true,
          self_device,
          self_sizes);
    } else if (index.is_none()) {
      return self.unsqueeze(0);
    } else if (index.is_ellipsis()) {
      return pluss::alias(self);
    } else if (index.is_boolean()) {
      Series result = self.unsqueeze(0);
      return dispatch_index(
          result,
          std::vector<Series>{impl::boolToIndexingTensor(
              result, index.boolean(), self_device)});
    }
  }

  std::vector<Series> tensorIndices;
  Series sliced = impl::applySlicing(
      self,
      indices,
      tensorIndices,
      disable_slice_optimization,
      self_device,
      self_sizes);
  if (tensorIndices.empty()) {
    if (sliced.is_same(self)) {
      // ensure we return a shallow copy for things like x[...]
      sliced = pluss::alias(sliced);
    }
    return sliced;
  }

  // indexing by tensors ("advanced" indexing)
  return dispatch_index(sliced, std::move(tensorIndices));
}

// This mirrors `THPVariable_setitem` in
// torch/csrc/autograd/python_variable_indexing.cpp for "the assigned value is a
// Series" case See NOTE [ Setting `disable_slice_optimization` when calling C++
// series indexing functions from Python ]
inline void set_item(
    const Series& self,
    const ArrayRef<TensorIndex>& indices,
    const Series& value,
    bool disable_slice_optimization = false) {
  pluss::Device self_device = self.device();
  SymIntArrayRef self_sizes = self.sym_sizes();

  // handle simple types: integers, slices, ellipsis, bool
  if (indices.size() == 1) {
    const TensorIndex& index = indices[0];
    if (index.is_boolean() && !index.boolean()) {
      // do nothing for false (technically we should check the size, but we
      // don't have real 0-sized shapes.
      return;
    } else if (index.is_ellipsis()) {
      copy_to(self, value);
      return;
    } else if (index.is_none() || (index.is_boolean() && index.boolean())) {
      copy_to(self.unsqueeze(0), value);
      return;
    } else if (index.is_integer()) {
      copy_to(
          impl::applySelect(
              self, 0, index.integer(), 0, self_device, self_sizes),
          value);
      return;
    } else if (index.is_slice()) {
      copy_to(
          impl::applySlice(
              self,
              0,
              index.slice().start(),
              index.slice().stop(),
              index.slice().step(),
              /*disable_slice_optimization=*/disable_slice_optimization,
              self_device,
              self_sizes),
          value);
      return;
    }
  }

  std::vector<Series> tensorIndices;
  Series sliced = impl::applySlicing(
      self,
      indices,
      tensorIndices,
      disable_slice_optimization,
      self_device,
      self_sizes);
  if (tensorIndices.empty()) {
    copy_to(sliced, value);
    return;
  }

  SymIntArrayRef valueSizes = value.sym_sizes();
  SymIntArrayRef slicedValueSizes = slicePrefix1sSize(valueSizes);
  Series valuesSliced;
  if (!valueSizes.equals(slicedValueSizes)) {
    valuesSliced = value.view_symint(slicedValueSizes);
  } else {
    valuesSliced = value;
  }
  dispatch_index_put_(sliced, std::move(tensorIndices), valuesSliced);
  return;
}

} // namespace pluss::indexing
