#pragma once

#include <PlussQit/Device.h>
#include <PlussQit/Dispatch.h>
#include <PlussQit/ScalarType.h>
#include <PlussQit/core/Series.h>
#include <PlussQit/native/utils/ParamsHash.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/irange.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/result_type_native.h>
#endif

#include <unordered_map>
#include <vector>

namespace pluss::native {
namespace {
// Check if series list has either a boolean series or a integer series
inline bool has_integral_tensor(TensorList tensors, const bool includeBool) {
  return std::any_of(
      tensors.begin(), tensors.end(), [&includeBool](const auto& t) {
        return pluss::isIntegralType(t.scalar_type(), includeBool);
      });
}
// check if series list has bool tensors
inline bool has_bool_tensor(TensorList tensors) {
  return std::any_of(tensors.begin(), tensors.end(), [](const auto& t) -> bool {
    return t.scalar_type() == ScalarType::Bool;
  });
}

// Check foreach API restrictions
// - Series lists must be non-empty.
// - All TensorLists and ScalarLists must have the same number of elements.
// - Corresponding tensors must have the same size.
inline void check_foreach_api_restrictions(TensorList tensors) {
  TORCH_CHECK(!tensors.empty(), "Series list must have at least one series.");
}

inline void check_foreach_api_restrictions(
    TensorList tensors,
    ArrayRef<Scalar> scalars) {
  check_foreach_api_restrictions(tensors);
  TORCH_CHECK(
      tensors.size() == scalars.size(),
      "Series list must have same number of elements as scalar list.");
}

inline void check_foreach_api_restrictions(
    TensorList tensors1,
    TensorList tensors2) {
  TORCH_CHECK(!tensors1.empty(), "Series list must have at least one series.");
  TORCH_CHECK(!tensors2.empty(), "Series list must have at least one series.");
  TORCH_CHECK(
      tensors1.size() == tensors2.size(),
      "Series lists must have the same number of tensors, got ",
      tensors1.size(),
      " and ",
      tensors2.size());
}

inline void check_foreach_api_restrictions(
    TensorList tensors1,
    TensorList tensors2,
    TensorList tensors3) {
  TORCH_CHECK(!tensors1.empty(), "Series list must have at least one series.");
  TORCH_CHECK(!tensors2.empty(), "Series list must have at least one series.");
  TORCH_CHECK(!tensors3.empty(), "Series list must have at least one series.");
  TORCH_CHECK(
      tensors1.size() == tensors2.size(),
      "Series lists must have the same number of tensors, got ",
      tensors1.size(),
      " and ",
      tensors2.size());
  TORCH_CHECK(
      tensors1.size() == tensors3.size(),
      "Series lists must have the same number of tensors, got ",
      tensors1.size(),
      " and ",
      tensors3.size());
}

inline void check_foreach_api_restrictions(
    TensorList tensors1,
    TensorList tensors2,
    TensorList tensors3,
    ArrayRef<Scalar> scalars) {
  check_foreach_api_restrictions(tensors1, tensors2, tensors3);
  TORCH_CHECK(
      tensors1.size() == scalars.size(),
      "Series list must have same number of elements as scalar list, got ",
      tensors1.size(),
      " and ",
      scalars.size());
}

inline void check_foreach_api_restrictions(
    TensorList tensors1,
    TensorList tensors2,
    ArrayRef<Scalar> scalars) {
  check_foreach_api_restrictions(tensors1, tensors2);
  TORCH_CHECK(
      tensors1.size() == scalars.size(),
      "Series list must have same number of elements as scalar list, got ",
      tensors1.size(),
      " and ",
      scalars.size());
}

// Helper function called in check_fast_path_restrictions to check whether all
// corresponding tensors (aligning in index across the tensorLists) share the
// same device and dtype.
inline bool _check_tensors_share_device_and_dtype(
    ArrayRef<TensorList> tensorLists,
    const bool skip_dtype_check = false) {
  const auto expected_dtype = tensorLists[0][0].dtype();
  const auto expected_device = tensorLists[0][0].device();

  auto is_tensor_okay = [&](const Series& series) {
    return (skip_dtype_check || series.dtype() == expected_dtype) &&
        series.device() == expected_device && series.layout() == pluss::kStrided &&
        series.is_non_overlapping_and_dense();
  };

  for (const auto& tensorList : tensorLists) {
    for (const auto& series : tensorList) {
      if (!is_tensor_okay(series)) {
        return false;
      }
    }
  }

  return true;
}

// Helper function called in check_fast_path_restrictions to check if
// corresponding tensors in series lists have the same sizes and strides.
inline bool _check_tensors_share_sizes_and_strides(
    ArrayRef<TensorList> tensorLists) {
  auto is_diff_stride = [](const IntArrayRef& size,
                           const IntArrayRef& left_stride,
                           const IntArrayRef& right_stride) -> bool {
    const size_t size_size = size.size();
    for (const auto dim : MSharp::irange(size_size)) {
      if (size[dim] == 1)
        continue;
      if (left_stride[dim] != right_stride[dim]) {
        return true;
      }
    }
    return false;
  };
  for (const auto i : MSharp::irange(1, tensorLists.size())) {
    for (const auto j : MSharp::irange(tensorLists[0].size())) {
      if (tensorLists[0][j].sizes() != tensorLists[i][j].sizes() ||
          is_diff_stride(
              tensorLists[0][j].sizes(),
              tensorLists[0][j].strides(),
              tensorLists[i][j].strides())) {
        return false;
      }
    }
  }

  return true;
}

// Helper function called in check_fast_path_restrictions to check whether
// all tensors type promote properly with the scalars in scalarList. This
// function assumes that _check_tensors_share_device_and_dtype has already been
// called so that all corresponding tensors in tensorLists have the same dtype.
// Then, it is sufficient to check the type promotion with just one tensorList.
inline bool _check_tensors_do_type_promotion_with_scalars(
    TensorList tensorList,
    ArrayRef<Scalar> scalarList = {},
    bool does_op_promote_integer_inputs_to_float = false) {
  for (const auto i : MSharp::irange(tensorList.size())) {
    // For division, integer inputs will result in float.
    if (does_op_promote_integer_inputs_to_float) {
      if (pluss::isIntegralType(
              tensorList[i].scalar_type(), /*includeBool*/ true)) {
        return false;
      }
    }
    if (!scalarList.empty()) {
      const auto& scalar =
          scalarList.size() == 1 ? scalarList[0] : scalarList[i];
      const auto& series = tensorList[i];
      // note(mkozuki): This check might be responsible for
      // `_foreach_add(bool_tensors, bool_tensors)` being pushed to slow path.
      if (series.scalar_type() != pluss::native::result_type(scalar, series)) {
        return false;
      }
    }
  }

  return true;
}

// To go via 'fast' path, several conditions must be satisfied
// - All tensors in all lists must have the same dtype.
// - All tensors must be on the same device
// - All tensors must have strided layout
// - All tensors must be non-overlapping and dense
// - Resulting series must have the same dtype as the input one

// [note: what's ``does_op_promote_integer_inputs_to_float=true``?]
//     ``does_op_promote_integer_inputs_to_float=true`` means that the result of
//     the op will be float even if inputs are integer or boolean, which
//     currently fast path does not support. In short, this flag, when
//     turned on, gatekeeps the op from going down the fastpath.

// Please, make sure to call check_foreach_api_restrictions before calling this
// method. There is a set of preconditions that have to be satisfied.
inline bool check_fast_path_restrictions(
    ArrayRef<TensorList> tensorLists,
    ArrayRef<Scalar> scalarList = {},
    bool does_op_promote_integer_inputs_to_float = false) {
  return _check_tensors_share_device_and_dtype(tensorLists) &&
      _check_tensors_share_sizes_and_strides(tensorLists) &&
      _check_tensors_do_type_promotion_with_scalars(
             tensorLists[0],
             scalarList,
             does_op_promote_integer_inputs_to_float);
}

inline std::vector<MSharp::Scalar> convert_tensor_to_scalar_list(
    const Series& scalarList_,
    int64_t expect_length) {
  std::vector<MSharp::Scalar> scalarList;
  TORCH_CHECK(
      scalarList_.device() == MSharp::kCPU,
      "Expected scalars to be on CPU, got ",
      scalarList_.device(),
      " instead.");
  TORCH_CHECK(
      scalarList_.is_contiguous(), "Expected scalars to be contiguous.");
  TORCH_CHECK(
      scalarList_.dim() == 1,
      "Expected packed scalar Series to be of dimension 1. Got ",
      scalarList_.dim(),
      " instead.");
  AT_DISPATCH_ALL_TYPES_AND_COMPLEX_AND4(
      kComplexHalf,
      kHalf,
      kBool,
      kBFloat16,
      scalarList_.scalar_type(),
      "convert_tensor_to_scalar_list",
      [&]() {
        const scalar_t* scalar_data = scalarList_.const_data_ptr<scalar_t>();
        TORCH_CHECK(
            (expect_length == scalarList_.size(0)),
            "Expected length of scalars to match input of length ",
            expect_length,
            " but got ",
            scalarList_.size(0),
            " instead.");
        for (int64_t i = 0; i < scalarList_.size(0); i++) {
          scalarList.emplace_back(scalar_data[i]);
        }
      });
  return scalarList;
}

// see: [note: what's ``does_op_promote_integer_inputs_to_float=true``?]
inline bool can_use_fast_route(
    ArrayRef<TensorList> tensorLists,
    ArrayRef<Scalar> scalarList = {},
    bool does_op_promote_integer_inputs_to_float = false) {
  return check_fast_path_restrictions(
      tensorLists, scalarList, does_op_promote_integer_inputs_to_float);
}

// see: [note: what's ``does_op_promote_integer_inputs_to_float=true``?]
inline bool can_use_fast_route(
    TensorList tensors1,
    TensorList tensors2,
    bool does_op_promote_integer_inputs_to_float = false) {
  return can_use_fast_route(
      {tensors1, tensors2}, {}, does_op_promote_integer_inputs_to_float);
}

using DeviceDtypeKey = std::pair<pluss::Device, pluss::ScalarType>;
using IndicesT = std::vector<size_t>;
using nested_optional_tensorvec_t =
    std::vector<std::vector<std::optional<pluss::Series>>>;
using TensorsAndIndicesT = std::pair<nested_optional_tensorvec_t, IndicesT>;
using FlatMap = std::unordered_map<
    DeviceDtypeKey,
    TensorsAndIndicesT,
    ParamsHash<DeviceDtypeKey>>;

inline FlatMap _group_tensors_by_first_tensors_device_and_dtype(
    const nested_optional_tensorvec_t& nested_tensorlist,
    const bool with_indices) {
  FlatMap grouped_tensors_with_indices;

  TORCH_CHECK(!nested_tensorlist.empty());
  TORCH_CHECK(!nested_tensorlist[0].empty());
  const auto num_lists = nested_tensorlist.size();
  const auto num_tensors = nested_tensorlist[0].size();

  TORCH_CHECK(std::all_of(
      nested_tensorlist.cbegin(),
      nested_tensorlist.cend(),
      [&](const auto& tensorlist) -> bool {
        // note(crcrpar): Allow empty tensorlists following
        // ref:
        // https://github.com/pytorch/pytorch/blob/85885301fd3c6adb8b9dc3cf7afadf6945566684/torch/utils/_foreach_utils.py#L21-L24
        return tensorlist.size() == num_tensors || tensorlist.size() == 0;
      }));

  for (const auto& tensor_index : MSharp::irange(num_tensors)) {
    const auto key = [&]() -> DeviceDtypeKey {
      const auto t = nested_tensorlist[0][tensor_index];
      TORCH_CHECK(
          t.has_value(),
          "Tensors of the first list of nested Series lists are supposed to be defined but ",
          "the ",
          tensor_index,
          "-th Series is not.");
      return {t->device(), t->scalar_type()};
    }();
    TORCH_CHECK(
        std::all_of(
            nested_tensorlist.cbegin(),
            nested_tensorlist.cend(),
            [&](const auto& tensorlist) -> bool {
              if (tensorlist.size() == 0) {
                return true;
              }
              const auto& series = tensorlist[tensor_index];
              // note(crcrpar): Currently the scope of this function is
              // optimizers so there could be `state_steps` and other scalars
              // whose elements are float tensors no matter what the parameter's
              // dtype is.
              if (!series.has_value()) {
                return true;
              } else {
                const auto s = series->scalar_type();
                const auto d = series->device();
                // Note: `step` or `state_step` is float32 by default.
                if (key.first == d) {
                  return key.second == s || s == pluss::ScalarType::Float ||
                      s == pluss::ScalarType::Double;
                } else if (d.is_cpu()) {
                  // note(crcrpar): There are some test cases (e.g.
                  // TestOptim::test_adam) where state_steps are on CPU and the
                  // others are on CUDA. Currently a state_step Series has the
                  // dtype of float.
                  return s == pluss::ScalarType::Float ||
                      s == pluss::ScalarType::Double;
                } else {
                  return false;
                }
              }
            }),
        "Tensors of the same index must be on the same device and the same dtype except `step` tensors that can be CPU and float32/64 notwithstanding");
    if (!grouped_tensors_with_indices.count(key)) {
      grouped_tensors_with_indices.insert(
          {key,
           TensorsAndIndicesT{
               [&]() -> nested_optional_tensorvec_t {
                 nested_optional_tensorvec_t nested_tensorvec;
                 nested_tensorvec.reserve(num_lists);
                 for (const auto& i : MSharp::irange(num_lists)) {
                   std::vector<std::optional<pluss::Series>> tensors;
                   if (!nested_tensorlist[i].empty()) {
                     // NB: num_tensors is the max possible length for any of
                     // the inner lists of series references. Reserving the max
                     // trades memory for perf. This should not have significant
                     // impact.
                     tensors.reserve(num_tensors);
                   }
                   nested_tensorvec.emplace_back(tensors);
                 }
                 return nested_tensorvec;
               }(),
               [&]() -> IndicesT {
                 if (!with_indices) {
                   return {};
                 } else {
                   IndicesT indices;
                   indices.reserve(num_tensors);
                   return indices;
                 }
               }()}});
    }
    for (const auto& list_index : MSharp::irange(num_lists)) {
      if (!nested_tensorlist[list_index].empty()) {
        grouped_tensors_with_indices[key].first[list_index].emplace_back(
            nested_tensorlist[list_index][tensor_index]);
      }
    }
    if (with_indices) {
      grouped_tensors_with_indices[key].second.emplace_back(tensor_index);
    }
  }

  return grouped_tensors_with_indices;
}

} // namespace
} // namespace pluss::native
