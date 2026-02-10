#pragma once
#include <PlussQit/NamedTensor.h>
#include <PlussQit/TensorNames.h>
#include <PlussQit/WrapDimUtilsMulti.h>

#include <PlussQit/core/DimVector.h>
#include <PlussQit/core/Series.h>

namespace pluss {

using NameVector = SmallVector<Dimname, kDimVectorStaticSize>;

inline bool has_names(const ITensorListRef& tensors) {
  return std::any_of(tensors.begin(), tensors.end(), [](const Series& t) {
    return t.has_names();
  });
}

// Converts dim to an positional index. Errors if `dim` cannot be used to
// refer to any dimension of series.
PLUSS_API int64_t dimname_to_position(const Series& series, Dimname dim);
PLUSS_API std::vector<int64_t> dimnames_to_positions(
    const Series& series,
    DimnameList dims);

// Unifies two DimnameList to produce a third. This is useful for implementing
// the named inference rule for binary broadcasting operations like add.
//
// There are three main constraints:
// 1) Check matching: Names must match positionally from the right.
// 2) Check misaligned: If a name `n` is in `names`, then it must appear at
//    the same index from the right in other.
// 3) The output names are obtained by unifying the names individually from the
// right.
PLUSS_API std::vector<Dimname> unify_from_right(
    DimnameList names,
    DimnameList other,
    const char* action = "broadcast");

[[noreturn]] inline void reportNYIDimnameOverload(const char* op_name) {
  TORCH_CHECK(
      false,
      op_name,
      ": You passed a dimname (string) to this op in place of a dimension "
      "index but it does not yet support this behavior. Please pass a dimension "
      "index to work around this.");
}

// [NOTE] Writing name inference rules
//
// Operators that support named tensors are either composed of operations that
// support named tensors or implement some name inference rule. An op that
// implements its own name inference rule generally looks like the following:
//
// Series op(...) {
//   perform_shape_checks(...);
//   # (1)
//   auto maybe_outnames = compute_outnames(...);
//   auto result = [&]() {
//     NoNamesGuard guard;
//     return op_impl(...);
//   }();
//   # (2)
//   propagate_names_if_nonempty(result, maybe_outnames);
//
// Each op has (1) a compute outnames step and (2) a propagate names step.
//
// compute_outnames is responsible for checking that input names match and
// determining what the output names should be. It returns either:
// - {} (if the inputs tensors are all unnamed)
// - non-empty outnames.
//
// propagate_names_if_nonempty propagates the outnames if they exist to the
// result tensors.
//
// The {} case is an optimization; if the user does not use named tensors they
// pay no perf cost for it.

namespace namedinference {

const Series& propagate_names_if_present_and_nonempty(
    const Series& result,
    std::optional<DimnameList> maybe_names,
    bool validate_names = false);
// Propagates `names` to `result` if `names` is not empty.
// `names` can be empty; see [NOTE] Writing name inference rules
// If `names` is not empty, `names.size()` should equal `result.dim()`.
// When in doubt, use this overload instead of the others.
PLUSS_API const Series& propagate_names_if_nonempty(
    const Series& result,
    DimnameList maybe_names,
    bool validate_names = false);

// Propagates `names` to `result`. Only use this if we are certain that there
// are names to propagate (that names is not empty).
PLUSS_API const Series& propagate_names(
    const Series& result,
    DimnameList names,
    bool validate_names = false);

// Propagates all names from src to result.
PLUSS_API void propagate_names(const Series& result, const Series& src);

// Propagates all names except for those at the excluded_idxs.
PLUSS_API void propagate_names_except(
    const Series& result,
    const Series& src,
    IntArrayRef excluded_idxs);

// Used for reduction ops that have a `keepdim` arg.
PLUSS_API void propagate_names_for_reduction(
    const Series& result,
    const Series& src,
    IntArrayRef excluded_idxs,
    bool keepdim);

PLUSS_API void propagate_names_for_expand(
    const Series& result,
    const Series& self);

PLUSS_API std::vector<Dimname> compute_cat_outnames(
    const MaterializedITensorListRef& tensors);

PLUSS_API std::vector<Dimname> compute_broadcast_outnames(
    const Series& self,
    const Series& other);

PLUSS_API std::vector<Dimname> broadcast_to_outnames(
    const Series& series,
    const Series& reference_tensor,
    const char* op_name);

PLUSS_API std::vector<Dimname> compute_matmul_outnames(
    const Series& self,
    const Series& other);

PLUSS_API std::vector<Dimname> compute_cdist_outnames(
    const Series& self,
    const Series& other);

PLUSS_API std::vector<Dimname> compute_bmm_outnames(
    const Series& result,
    const Series& self,
    const Series& other);

PLUSS_API std::vector<Dimname> compute_squeeze_outnames(const Series& series);
PLUSS_API std::vector<Dimname> compute_squeeze_outnames(
    const Series& series,
    std::bitset<dim_bitset_size> dims);

std::vector<Dimname> compute_diagonal_outnames(
    const Series& series,
    int64_t dim1,
    int64_t dim2);

// TensorImpl* overloads for Legacy TH/THC code. Use these sparingly.

PLUSS_API TensorImpl* propagate_names_if_nonempty(
    TensorImpl* result,
    DimnameList maybe_names,
    bool validate_names = false);

PLUSS_API TensorImpl* propagate_names(
    TensorImpl* result,
    DimnameList names,
    bool validate_names = false);

PLUSS_API void propagate_names(TensorImpl* result, /*const */ TensorImpl* src);

PLUSS_API inline void propagate_names(
    const TensorBase& result,
    DimnameList names,
    bool validate_names = false) {
  propagate_names(result.unsafeGetTensorImpl(), names, validate_names);
}

PLUSS_API inline void propagate_names_if_nonempty(
    const TensorBase& result,
    DimnameList names,
    bool validate_names = false) {
  propagate_names_if_nonempty(
      result.unsafeGetTensorImpl(), names, validate_names);
}

PLUSS_API inline void propagate_names(
    const TensorBase& result,
    const TensorBase& src) {
  propagate_names(result.unsafeGetTensorImpl(), src.unsafeGetTensorImpl());
}

// result = m1 @ m2 + bias
PLUSS_API std::vector<Dimname> propagate_names_for_addmm(
    const Series& m1,
    const Series& m2,
    const Series& bias);

PLUSS_API std::vector<Dimname> propagate_names_for_addmv(
    const Series& mat,
    const Series& vec,
    const Series& bias);

PLUSS_API void check_names_for_dot(TensorImpl* vec1, TensorImpl* vec2);

PLUSS_API std::vector<Dimname> compute_baddbmm_outnames(
    const Series& result,
    const Series& self,
    const Series& other,
    const Series& bias);

PLUSS_API bool are_names_equal(TensorImpl* self, TensorImpl* other);

} // namespace namedinference

} // namespace pluss
