// OpsInternal_all_impl.cpp
// Generated implementations for pluss::codem::ops_internal from OpsInternal.h.
//
// NOTE: This file is meant to be pasted into your project. You may want to split
// the "parameter types" section into a shared header (e.g. CodeM/core/ArrayRef.h)
// so both Series.h and OpsInternal.h can include it.

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "CodeM/core/Series.h"



// ----- Optional: centralized native CPU backend hooks -----
// If you already have native::cpu kernels, include their headers instead and remove this block.
// The idea is: OpsInternal stays thin, type-safe, and does validation + dispatch.

namespace pluss::codem::ops_internal {

Series __and__(const Series& self, const Scalar & other) {
  return bitwise_and(self, other);
}

Series __and__(const Series& self, const Series & other) {
  return bitwise_and(self, other);
}

bool __dispatch__is_zerotensor(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch__is_zerotensor", self);
}

Series __dispatch_conj(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("__dispatch_conj", self);
}

Series __dispatch_contiguous(const Series& self, MemoryFormat memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("__dispatch_contiguous", self, memory_format);
}

bool __dispatch_is_complex(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch_is_complex", self);
}

bool __dispatch_is_conj(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch_is_conj", self);
}

bool __dispatch_is_floating_point(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch_is_floating_point", self);
}

bool __dispatch_is_inference(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch_is_inference", self);
}

bool __dispatch_is_neg(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch_is_neg", self);
}

bool __dispatch_is_signed(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("__dispatch_is_signed", self);
}

Series & __iand__(Series& self, const Scalar & other) {
  return bitwise_and_(self, other);
}

Series & __iand__(Series& self, const Series & other) {
  return bitwise_and_(self, other);
}

Series & __ilshift__(Series& self, const Scalar & other) {
  return bitwise_left_shift_(self, other);
}

Series & __ilshift__(Series& self, const Series & other) {
  return bitwise_left_shift_(self, other);
}

Series & __ior__(Series& self, const Scalar & other) {
  return bitwise_or_(self, other);
}

Series & __ior__(Series& self, const Series & other) {
  return bitwise_or_(self, other);
}

Series & __irshift__(Series& self, const Scalar & other) {
  return bitwise_right_shift_(self, other);
}

Series & __irshift__(Series& self, const Series & other) {
  return bitwise_right_shift_(self, other);
}

Series & __ixor__(Series& self, const Scalar & other) {
  return bitwise_xor_(self, other);
}

Series & __ixor__(Series& self, const Series & other) {
  return bitwise_xor_(self, other);
}

Series __lshift__(const Series& self, const Scalar & other) {
  return bitwise_left_shift(self, other);
}

Series __lshift__(const Series& self, const Series & other) {
  return bitwise_left_shift(self, other);
}

Series __or__(const Series& self, const Scalar & other) {
  return bitwise_or(self, other);
}

Series __or__(const Series& self, const Series & other) {
  return bitwise_or(self, other);
}

Series __rshift__(const Series& self, const Scalar & other) {
  return bitwise_right_shift(self, other);
}

Series __rshift__(const Series& self, const Series & other) {
  return bitwise_right_shift(self, other);
}

Series __xor__(const Series& self, const Scalar & other) {
  return bitwise_xor(self, other);
}

Series __xor__(const Series& self, const Series & other) {
  return bitwise_xor(self, other);
}

Series _addmm_activation(const Series& self, const Series & mat1, const Series & mat2, const Scalar & beta, const Scalar & alpha, bool use_gelu) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_addmm_activation", self, mat1, mat2, beta, alpha, use_gelu);
}

Series _autocast_to_full_precision(const Series& self, bool cuda_enabled, bool cpu_enabled) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_autocast_to_full_precision", self, cuda_enabled, cpu_enabled);
}

Series _autocast_to_reduced_precision(const Series& self, bool cuda_enabled, bool cpu_enabled, ScalarType cuda_dtype, ScalarType cpu_dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_autocast_to_reduced_precision", self, cuda_enabled, cpu_enabled, cuda_dtype, cpu_dtype);
}

Series & _coalesced_(Series& self, bool coalesced) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("_coalesced_", self, coalesced);
}

Series _conj(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_conj", self);
}

Series _conj_physical(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_conj_physical", self);
}

int64_t _dimI(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("_dimI", self);
}

int64_t _dimV(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("_dimV", self);
}

Series _fw_primal(const Series& self, int64_t level) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_fw_primal", self, level);
}

Series _indices(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_indices", self);
}

Series _is_all_true(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_is_all_true", self);
}

Series _is_any_true(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_is_any_true", self);
}

Series _lazy_clone(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_lazy_clone", self);
}

Series _neg_view(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_neg_view", self);
}

Series _nested_tensor_size(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_nested_tensor_size", self);
}

Series _nested_tensor_storage_offsets(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_nested_tensor_storage_offsets", self);
}

Series _nested_tensor_strides(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_nested_tensor_strides", self);
}

int64_t _nnz(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("_nnz", self);
}

Series _reshape_alias(const Series& self, IntArrayRef size, IntArrayRef stride) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_reshape_alias", self, size, stride);
}

Series _reshape_alias_symint(const Series& self, SymIntArrayRef size, SymIntArrayRef stride) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_reshape_alias_symint", self, size, stride);
}

Series _sparse_mask_projection(const Series& self, const Series & mask, bool accumulate_matches) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_sparse_mask_projection", self, mask, accumulate_matches);
}

Series _to_dense(const Series& self, optional<ScalarType> dtype, optional<bool> masked_grad) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_dense", self, dtype, masked_grad);
}

Series _to_sparse(const Series& self, optional<Layout> layout, OptionalIntArrayRef blocksize, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_sparse", self, layout, blocksize, dense_dim);
}

Series _to_sparse(const Series& self, int64_t sparse_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_sparse", self, sparse_dim);
}

Series _to_sparse_bsc(const Series& self, IntArrayRef blocksize, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_sparse_bsc", self, blocksize, dense_dim);
}

Series _to_sparse_bsr(const Series& self, IntArrayRef blocksize, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_sparse_bsr", self, blocksize, dense_dim);
}

Series _to_sparse_csc(const Series& self, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_sparse_csc", self, dense_dim);
}

Series _to_sparse_csr(const Series& self, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_to_sparse_csr", self, dense_dim);
}

Series _values(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("_values", self);
}

Series abs(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("abs", self);
}

Series& abs_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("abs_", self);
}

Series absolute(const Series& self) {
  return abs(self);
}

Series & absolute_(Series& self) {
  return abs_(self);
}

Series acos(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("acos", self);
}

Series& acos_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("acos_", self);
}

Series acosh(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("acosh", self);
}

Series& acosh_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("acosh_", self);
}

Series add(const Series& self, const Scalar & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("add", self, other, alpha);
}

Series add(const Series& self, const Series & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("add", self, other, alpha);
}

Series add(const Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("add", self, rhs);
}

Series add(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("add", self, rhs);
}

Series & add_(Series& self, const Scalar & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("add_", self, other, alpha);
}

Series & add_(Series& self, const Series & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("add_", self, other, alpha);
}

Series& add_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("add_", self, rhs);
}

Series& add_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("add_", self, rhs);
}

Series addbmm(const Series& self, const Series & batch1, const Series & batch2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addbmm", self, batch1, batch2, beta, alpha);
}

Series & addbmm_(Series& self, const Series & batch1, const Series & batch2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("addbmm_", self, batch1, batch2, beta, alpha);
}

Series addcdiv(const Series& self, const Series & tensor1, const Series & tensor2, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addcdiv", self, tensor1, tensor2, value);
}

Series addcdiv(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addcdiv", self, dim, keepdim);
}

Series & addcdiv_(Series& self, const Series & tensor1, const Series & tensor2, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("addcdiv_", self, tensor1, tensor2, value);
}

Series addcmul(const Series& self, const Series & tensor1, const Series & tensor2, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addcmul", self, tensor1, tensor2, value);
}

Series addcmul(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addcmul", self, dim, keepdim);
}

Series & addcmul_(Series& self, const Series & tensor1, const Series & tensor2, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("addcmul_", self, tensor1, tensor2, value);
}

Series addmm(const Series& self, const Series & mat1, const Series & mat2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addmm", self, mat1, mat2, beta, alpha);
}

Series addmm(const Series& self, const Series& mat1, const Series& mat2, double beta, double alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addmm", self, mat1, mat2, beta, alpha);
}

Series & addmm_(Series& self, const Series & mat1, const Series & mat2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("addmm_", self, mat1, mat2, beta, alpha);
}

Series addmv(const Series& self, const Series & mat, const Series & vec, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addmv", self, mat, vec, beta, alpha);
}

Series & addmv_(Series& self, const Series & mat, const Series & vec, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("addmv_", self, mat, vec, beta, alpha);
}

Series addr(const Series& self, const Series & vec1, const Series & vec2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("addr", self, vec1, vec2, beta, alpha);
}

Series & addr_(Series& self, const Series & vec1, const Series & vec2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("addr_", self, vec1, vec2, beta, alpha);
}

Series adjoint(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("adjoint", self);
}

Series alias(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("alias", self);
}

Series align_as(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("align_as", self, other);
}

Series align_to(const Series& self, DimnameList names) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("align_to", self, names);
}

Series align_to(const Series& self, DimnameList order, int64_t ellipsis_idx) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("align_to", self, order, ellipsis_idx);
}

Series all(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("all", self);
}

Series all(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("all", self, dim, keepdim);
}

Series all(const Series& self, OptionalIntArrayRef dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("all", self, dim, keepdim);
}

Series all(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("all", self, dim, keepdim);
}

Series all(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("all", self, dims, keepdim);
}

Series all_all(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("all_all", self);
}

Series& all_all_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("all_all_", self);
}

bool allclose(const Series& self, const Series & other, double rtol, double atol, bool equal_nan) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("allclose", self, other, rtol, atol, equal_nan);
}

Series amax(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amax", self);
}

Series amax(const Series& self, IntArrayRef dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amax", self, dim, keepdim);
}

Series amax(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amax", self, dim, keepdim);
}

Series amax(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amax", self, dims, keepdim);
}

Series& amax_(Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("amax_", self, dim, keepdim);
}

Series amin(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amin", self);
}

Series amin(const Series& self, IntArrayRef dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amin", self, dim, keepdim);
}

Series amin(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amin", self, dim, keepdim);
}

Series amin(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("amin", self, dims, keepdim);
}

Series& amin_(Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("amin_", self, dim, keepdim);
}

::std::tuple<Series,Series> aminmax(const Series& self, optional<int64_t> dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("aminmax", self, dim, keepdim);
}

Series angle(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("angle", self);
}

Series any(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("any", self);
}

Series any(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("any", self, dim, keepdim);
}

Series any(const Series& self, OptionalIntArrayRef dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("any", self, dim, keepdim);
}

Series any(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("any", self, dim, keepdim);
}

Series any(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("any", self, dims, keepdim);
}

Series any_all(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("any_all", self);
}

Series& any_all_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("any_all_", self);
}

Series arccos(const Series& self) {
  return acos(self);
}

Series & arccos_(Series& self) {
  return acos_(self);
}

Series arccosh(const Series& self) {
  return acosh(self);
}

Series & arccosh_(Series& self) {
  return acosh_(self);
}

Series arcsin(const Series& self) {
  return asin(self);
}

Series & arcsin_(Series& self) {
  return asin_(self);
}

Series arcsinh(const Series& self) {
  return asinh(self);
}

Series & arcsinh_(Series& self) {
  return asinh_(self);
}

Series arctan(const Series& self) {
  return atan(self);
}

Series arctan2(const Series& self, const Series & other) {
  return atan2(self, other);
}

Series & arctan2_(Series& self, const Series & other) {
  return atan2_(self, other);
}

Series & arctan_(Series& self) {
  return atan_(self);
}

Series arctanh(const Series& self) {
  return atanh(self);
}

Series & arctanh_(Series& self) {
  return atanh_(self);
}

Series argmax(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmax", self);
}

Series argmax(const Series& self, optional<int64_t> dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmax", self, dim, keepdim);
}

Series argmax(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmax", self, dim, keepdim);
}

Series argmax(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmax", self, dims, keepdim);
}

Series& argmax_(Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("argmax_", self, dim, keepdim);
}

Series argmin(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmin", self);
}

Series argmin(const Series& self, optional<int64_t> dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmin", self, dim, keepdim);
}

Series argmin(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmin", self, dim, keepdim);
}

Series argmin(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argmin", self, dims, keepdim);
}

Series& argmin_(Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("argmin_", self, dim, keepdim);
}

Series argsort(const Series& self, Dimname dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argsort", self, dim, descending);
}

Series argsort(const Series& self, bool stable, int64_t dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argsort", self, stable, dim, descending);
}

Series argsort(const Series& self, int64_t dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argsort", self, dim, descending);
}

Series argwhere(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("argwhere", self);
}

Series as_strided(const Series& self, IntArrayRef size, IntArrayRef stride, optional<int64_t> storage_offset) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("as_strided", self, size, stride, storage_offset);
}

const Series & as_strided_(Series& self, IntArrayRef size, IntArrayRef stride, optional<int64_t> storage_offset) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("as_strided_", self, size, stride, storage_offset);
}

const Series & as_strided__symint(Series& self, SymIntArrayRef size, SymIntArrayRef stride, optional<SymInt> storage_offset) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("as_strided__symint", self, size, stride, storage_offset);
}

Series as_strided_scatter(const Series& self, const Series & src, IntArrayRef size, IntArrayRef stride, optional<int64_t> storage_offset) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("as_strided_scatter", self, src, size, stride, storage_offset);
}

Series as_strided_scatter_symint(const Series& self, const Series & src, SymIntArrayRef size, SymIntArrayRef stride, optional<SymInt> storage_offset) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("as_strided_scatter_symint", self, src, size, stride, storage_offset);
}

Series as_strided_symint(const Series& self, SymIntArrayRef size, SymIntArrayRef stride, optional<SymInt> storage_offset) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("as_strided_symint", self, size, stride, storage_offset);
}

Series asin(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("asin", self);
}

Series& asin_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("asin_", self);
}

Series asinh(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("asinh", self);
}

Series& asinh_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("asinh_", self);
}

Series atan(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("atan", self);
}

Series atan2(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("atan2", self, other);
}

Series & atan2_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("atan2_", self, other);
}

Series& atan_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("atan_", self);
}

Series atanh(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("atanh", self);
}

Series& atanh_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("atanh_", self);
}

Series baddbmm(const Series& self, const Series & batch1, const Series & batch2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("baddbmm", self, batch1, batch2, beta, alpha);
}

Series & baddbmm_(Series& self, const Series & batch1, const Series & batch2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("baddbmm_", self, batch1, batch2, beta, alpha);
}

Series bernoulli(const Series& self, double p, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bernoulli", self, p, generator);
}

Series bernoulli(const Series& self, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bernoulli", self, generator);
}

Series & bernoulli_(Series& self, const Series & p, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bernoulli_", self, p, generator);
}

Series & bernoulli_(Series& self, double p, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bernoulli_", self, p, generator);
}

Series bincount(const Series& self, const optional<Series> & weights, int64_t minlength) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bincount", self, weights, minlength);
}

Series bitwise_and(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_and", self, other);
}

Series bitwise_and(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_and", self, other);
}

Series & bitwise_and_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_and_", self, other);
}

Series & bitwise_and_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_and_", self, other);
}

Series bitwise_left_shift(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_left_shift", self, other);
}

Series bitwise_left_shift(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_left_shift", self, other);
}

Series bitwise_left_shift(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_left_shift", self, dim, keepdim);
}

Series & bitwise_left_shift_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_left_shift_", self, other);
}

Series & bitwise_left_shift_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_left_shift_", self, other);
}

Series bitwise_not(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_not", self);
}

Series & bitwise_not_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_not_", self);
}

Series bitwise_or(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_or", self, other);
}

Series bitwise_or(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_or", self, other);
}

Series & bitwise_or_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_or_", self, other);
}

Series & bitwise_or_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_or_", self, other);
}

Series bitwise_right_shift(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_right_shift", self, other);
}

Series bitwise_right_shift(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_right_shift", self, other);
}

Series bitwise_right_shift(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_right_shift", self, dim, keepdim);
}

Series & bitwise_right_shift_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_right_shift_", self, other);
}

Series & bitwise_right_shift_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_right_shift_", self, other);
}

Series bitwise_xor(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_xor", self, other);
}

Series bitwise_xor(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bitwise_xor", self, other);
}

Series & bitwise_xor_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_xor_", self, other);
}

Series & bitwise_xor_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("bitwise_xor_", self, other);
}

Series bmm(const Series& self, const Series & mat2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bmm", self, mat2);
}

Series bmm(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("bmm", self, dim, keepdim);
}

Series broadcast_to(const Series& self, IntArrayRef size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("broadcast_to", self, size);
}

Series broadcast_to_symint(const Series& self, SymIntArrayRef size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("broadcast_to_symint", self, size);
}

Series & cauchy_(Series& self, double median, double sigma, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("cauchy_", self, median, sigma, generator);
}

Series ccol_indices(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ccol_indices", self);
}

Series ceil(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ceil", self);
}

Series& ceil_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("ceil_", self);
}

Series chalf(const Series& self, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("chalf", self, memory_format);
}

Series cholesky(const Series& self, bool upper) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cholesky", self, upper);
}

Series cholesky_inverse(const Series& self, bool upper) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cholesky_inverse", self, upper);
}

Series cholesky_solve(const Series& self, const Series & input2, bool upper) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cholesky_solve", self, input2, upper);
}

::std::vector<Series> chunk(const Series& self, int64_t chunks, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("chunk", self, chunks, dim);
}

Series clamp(const Series& self, const optional<Scalar> & min, const optional<Scalar> & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp", self, min, max);
}

Series clamp(const Series& self, const optional<Series> & min, const optional<Series> & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp", self, min, max);
}

Series clamp(const Series& self, double lo, double hi) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp", self, lo, hi);
}

Series & clamp_(Series& self, const optional<Scalar> & min, const optional<Scalar> & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("clamp_", self, min, max);
}

Series & clamp_(Series& self, const optional<Series> & min, const optional<Series> & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("clamp_", self, min, max);
}

Series& clamp_(Series& self, double lo, double hi) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("clamp_", self, lo, hi);
}

Series clamp_max(const Series& self, const Scalar & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp_max", self, max);
}

Series clamp_max(const Series& self, const Series & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp_max", self, max);
}

Series clamp_max(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp_max", self, dim, keepdim);
}

Series & clamp_max_(Series& self, const Scalar & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("clamp_max_", self, max);
}

Series & clamp_max_(Series& self, const Series & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("clamp_max_", self, max);
}

Series clamp_min(const Series& self, const Scalar & min) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp_min", self, min);
}

Series clamp_min(const Series& self, const Series & min) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp_min", self, min);
}

Series clamp_min(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clamp_min", self, dim, keepdim);
}

Series & clamp_min_(Series& self, const Scalar & min) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("clamp_min_", self, min);
}

Series & clamp_min_(Series& self, const Series & min) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("clamp_min_", self, min);
}

Series clip(const Series& self, const optional<Scalar> & min, const optional<Scalar> & max) {
  return clamp(self, min, max);
}

Series clip(const Series& self, const optional<Series> & min, const optional<Series> & max) {
  return clamp(self, min, max);
}

Series clip(const Series& self, double lo, double hi) {
  return clamp(self, lo, hi);
}

Series & clip_(Series& self, const optional<Scalar> & min, const optional<Scalar> & max) {
  return clamp_(self, min, max);
}

Series & clip_(Series& self, const optional<Series> & min, const optional<Series> & max) {
  return clamp_(self, min, max);
}

Series& clip_(Series& self, double lo, double hi) {
  return clamp_(self, lo, hi);
}

Series clone(const Series& self, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("clone", self, memory_format);
}

Series coalesce(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("coalesce", self);
}

Series col_indices(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("col_indices", self);
}

Series conj_physical(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("conj_physical", self);
}

Series & conj_physical_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("conj_physical_", self);
}

Series conv1d(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("conv1d", self, dim, keepdim);
}

Series conv2d(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("conv2d", self, dim, keepdim);
}

Series conv3d(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("conv3d", self, dim, keepdim);
}

Series & copy_(Series& self, const Series & src, bool non_blocking) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("copy_", self, src, non_blocking);
}

Series copysign(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("copysign", self, other);
}

Series copysign(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("copysign", self, other);
}

Series & copysign_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("copysign_", self, other);
}

Series & copysign_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("copysign_", self, other);
}

Series corrcoef(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("corrcoef", self);
}

Series cos(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cos", self);
}

Series& cos_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("cos_", self);
}

Series cosh(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cosh", self);
}

Series& cosh_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("cosh_", self);
}

Series count_nonzero(const Series& self, IntArrayRef dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("count_nonzero", self, dim);
}

Series count_nonzero(const Series& self, optional<int64_t> dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("count_nonzero", self, dim);
}

Series cov(const Series& self, int64_t correction, const optional<Series> & fweights, const optional<Series> & aweights) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cov", self, correction, fweights, aweights);
}

Series cross(const Series& self, const Series & other, optional<int64_t> dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cross", self, other, dim);
}

Series crow_indices(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("crow_indices", self);
}

::std::tuple<Series,Series> cummax(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("cummax", self, dim);
}

::std::tuple<Series,Series> cummax(const Series& self, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("cummax", self, dim);
}

::std::tuple<Series,Series> cummin(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("cummin", self, dim);
}

::std::tuple<Series,Series> cummin(const Series& self, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("cummin", self, dim);
}

Series cumprod(const Series& self, Dimname dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cumprod", self, dim, dtype);
}

Series cumprod(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cumprod", self, dim, keepdim);
}

Series cumprod(const Series& self, int64_t dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cumprod", self, dim, dtype);
}

Series & cumprod_(Series& self, Dimname dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("cumprod_", self, dim, dtype);
}

Series & cumprod_(Series& self, int64_t dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("cumprod_", self, dim, dtype);
}

Series cumsum(const Series& self, Dimname dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cumsum", self, dim, dtype);
}

Series cumsum(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cumsum", self, dim, keepdim);
}

Series cumsum(const Series& self, int64_t dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("cumsum", self, dim, dtype);
}

Series & cumsum_(Series& self, Dimname dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("cumsum_", self, dim, dtype);
}

Series & cumsum_(Series& self, int64_t dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("cumsum_", self, dim, dtype);
}

Series deg2rad(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("deg2rad", self);
}

Series & deg2rad_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("deg2rad_", self);
}

int64_t dense_dim(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("dense_dim", self);
}

Series dequantize(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("dequantize", self);
}

Series det(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("det", self);
}

Series detach(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("detach", self);
}

Series & detach_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("detach_", self);
}

Series diag(const Series& self, int64_t diagonal) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diag", self, diagonal);
}

Series diag_embed(const Series& self, int64_t offset, int64_t dim1, int64_t dim2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diag_embed", self, offset, dim1, dim2);
}

Series diagflat(const Series& self, int64_t offset) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diagflat", self, offset);
}

Series diagonal(const Series& self, Dimname outdim, Dimname dim1, Dimname dim2, int64_t offset) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diagonal", self, outdim, dim1, dim2, offset);
}

Series diagonal(const Series& self, int64_t offset, int64_t dim1, int64_t dim2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diagonal", self, offset, dim1, dim2);
}

Series diagonal_scatter(const Series& self, const Series & src, int64_t offset, int64_t dim1, int64_t dim2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diagonal_scatter", self, src, offset, dim1, dim2);
}

Series diff(const Series& self, int64_t n, int64_t dim, const optional<Series> & prepend, const optional<Series> & append) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("diff", self, n, dim, prepend, append);
}

Series digamma(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("digamma", self);
}

Series & digamma_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("digamma_", self);
}

Series dist(const Series& self, const Series & other, const Scalar & p) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("dist", self, other, p);
}

Series div(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("div", self, other);
}

Series div(const Series& self, const Scalar & other, optional<string_view> rounding_mode) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("div", self, other, rounding_mode);
}

Series div(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("div", self, other);
}

Series div(const Series& self, const Series & other, optional<string_view> rounding_mode) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("div", self, other, rounding_mode);
}

Series div(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("div", self, rhs);
}

Series & div_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("div_", self, other);
}

Series & div_(Series& self, const Scalar & other, optional<string_view> rounding_mode) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("div_", self, other, rounding_mode);
}

Series & div_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("div_", self, other);
}

Series & div_(Series& self, const Series & other, optional<string_view> rounding_mode) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("div_", self, other, rounding_mode);
}

Series& div_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("div_", self, rhs);
}

Series& div_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("div_", self, rhs);
}

Series divide(const Series& self, const Scalar & other) {
  return div(self, other);
}

Series divide(const Series& self, const Scalar & other, optional<string_view> rounding_mode) {
  return div(self, other, rounding_mode);
}

Series divide(const Series& self, const Series & other) {
  return div(self, other);
}

Series divide(const Series& self, const Series & other, optional<string_view> rounding_mode) {
  return div(self, other, rounding_mode);
}

Series & divide_(Series& self, const Scalar & other) {
  return div_(self, other);
}

Series & divide_(Series& self, const Scalar & other, optional<string_view> rounding_mode) {
  return div_(self, other, rounding_mode);
}

Series & divide_(Series& self, const Series & other) {
  return div_(self, other);
}

Series & divide_(Series& self, const Series & other, optional<string_view> rounding_mode) {
  return div_(self, other, rounding_mode);
}

Series dot(const Series& self, const Series & tensor) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("dot", self, tensor);
}

Series dot(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("dot", self, dim, keepdim);
}

Series dropout(const Series& self, double p, bool train) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("dropout", self, p, train);
}

::std::vector<Series> dsplit(const Series& self, IntArrayRef indices) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("dsplit", self, indices);
}

::std::vector<Series> dsplit(const Series& self, int64_t sections) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("dsplit", self, sections);
}

Series einsum(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("einsum", self, dim, keepdim);
}

Series eq(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("eq", self, other);
}

Series eq(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("eq", self, other);
}

Series eq(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("eq", self, b);
}

Series & eq_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("eq_", self, other);
}

Series & eq_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("eq_", self, other);
}

Series& eq_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("eq_", self, b);
}

bool equal(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("equal", self, other);
}

Series erf(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("erf", self);
}

Series& erf_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("erf_", self);
}

Series erfc(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("erfc", self);
}

Series& erfc_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("erfc_", self);
}

Series erfinv(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("erfinv", self);
}

Series & erfinv_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("erfinv_", self);
}

Series exp(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("exp", self);
}

Series exp2(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("exp2", self);
}

Series & exp2_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("exp2_", self);
}

Series& exp_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("exp_", self);
}

Series expand(const Series& self, IntArrayRef size, bool implicit) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("expand", self, size, implicit);
}

Series expand_as(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("expand_as", self, other);
}

Series expand_symint(const Series& self, SymIntArrayRef size, bool implicit) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("expand_symint", self, size, implicit);
}

Series expm1(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("expm1", self);
}

Series& expm1_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("expm1_", self);
}

Series & exponential_(Series& self, double lambd, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("exponential_", self, lambd, generator);
}

Series & fill_(Series& self, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("fill_", self, value);
}

Series & fill_(Series& self, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("fill_", self, value);
}

Series & fill_diagonal_(Series& self, const Scalar & fill_value, bool wrap) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("fill_diagonal_", self, fill_value, wrap);
}

Series fix(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("fix", self);
}

Series & fix_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("fix_", self);
}

Series flatten(const Series& self, Dimname start_dim, Dimname end_dim, Dimname out_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flatten", self, start_dim, end_dim, out_dim);
}

Series flatten(const Series& self, DimnameList dims, Dimname out_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flatten", self, dims, out_dim);
}

Series flatten(const Series& self, std::int64_t start_dim, std::int64_t end_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flatten", self, start_dim, end_dim);
}

Series flatten(const Series& self, int64_t start_dim, int64_t end_dim, Dimname out_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flatten", self, start_dim, end_dim, out_dim);
}

Series flip(const Series& self, IntArrayRef dims) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flip", self, dims);
}

Series flip(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flip", self, dim, keepdim);
}

Series fliplr(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("fliplr", self);
}

Series flipud(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("flipud", self);
}

Series float_power(const Series& self, const Scalar & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("float_power", self, exponent);
}

Series float_power(const Series& self, const Series & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("float_power", self, exponent);
}

Series & float_power_(Series& self, const Scalar & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("float_power_", self, exponent);
}

Series & float_power_(Series& self, const Series & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("float_power_", self, exponent);
}

Series floor(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("floor", self);
}

Series& floor_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("floor_", self);
}

Series floor_divide(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("floor_divide", self, other);
}

Series floor_divide(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("floor_divide", self, other);
}

Series & floor_divide_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("floor_divide_", self, other);
}

Series & floor_divide_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("floor_divide_", self, other);
}

Series fmax(const Series& self, const Series & other) {
  return maximum(self, other);
}

Series fmin(const Series& self, const Series & other) {
  return minimum(self, other);
}

Series fmod(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("fmod", self, other);
}

Series fmod(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("fmod", self, other);
}

Series & fmod_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("fmod_", self, other);
}

Series & fmod_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("fmod_", self, other);
}

Series frac(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("frac", self);
}

Series & frac_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("frac_", self);
}

::std::tuple<Series,Series> frexp(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("frexp", self);
}

Series frobenius_norm(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("frobenius_norm", self, dim, keepdim);
}

Series gather(const Series& self, Dimname dim, const Series & index, bool sparse_grad) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gather", self, dim, index, sparse_grad);
}

Series gather(const Series& self, int64_t dim, const Series & index, bool sparse_grad) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gather", self, dim, index, sparse_grad);
}

Series gather(const Series& self, std::int64_t dim, const Series& index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gather", self, dim, index);
}

Series gather(const Series& self, std::int64_t dim, double index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gather", self, dim, index);
}

Series& gather_(Series& self, std::int64_t dim, const Series& index) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("gather_", self, dim, index);
}

Series& gather_(Series& self, std::int64_t dim, double index) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("gather_", self, dim, index);
}

Series gather_nd(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gather_nd", self, dim, keepdim);
}

Series gcd(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gcd", self, other);
}

Series & gcd_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("gcd_", self, other);
}

Series ge(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ge", self, other);
}

Series ge(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ge", self, other);
}

Series ge(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ge", self, b);
}

Series & ge_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("ge_", self, other);
}

Series & ge_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("ge_", self, other);
}

Series& ge_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("ge_", self, b);
}

Series & geometric_(Series& self, double p, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("geometric_", self, p, generator);
}

::std::tuple<Series,Series> geqrf(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("geqrf", self);
}

Series ger(const Series& self, const Series & vec2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ger", self, vec2);
}

Series greater(const Series& self, const Scalar & other) {
  return gt(self, other);
}

Series greater(const Series& self, const Series & other) {
  return gt(self, other);
}

Series & greater_(Series& self, const Scalar & other) {
  return gt_(self, other);
}

Series & greater_(Series& self, const Series & other) {
  return gt_(self, other);
}

Series greater_equal(const Series& self, const Scalar & other) {
  return ge(self, other);
}

Series greater_equal(const Series& self, const Series & other) {
  return ge(self, other);
}

Series & greater_equal_(Series& self, const Scalar & other) {
  return ge_(self, other);
}

Series & greater_equal_(Series& self, const Series & other) {
  return ge_(self, other);
}

Series gt(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gt", self, other);
}

Series gt(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gt", self, other);
}

Series gt(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("gt", self, b);
}

Series & gt_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("gt_", self, other);
}

Series & gt_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("gt_", self, other);
}

Series& gt_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("gt_", self, b);
}

Series hardshrink(const Series& self, const Scalar & lambd) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("hardshrink", self, lambd);
}

Series hardshrink_backward(const Series& self, const Series & grad_out, const Scalar & lambd) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("hardshrink_backward", self, grad_out, lambd);
}

Series heaviside(const Series& self, const Series & values) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("heaviside", self, values);
}

Series & heaviside_(Series& self, const Series & values) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("heaviside_", self, values);
}

Series histc(const Series& self, int64_t bins, const Scalar & min, const Scalar & max) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("histc", self, bins, min, max);
}

::std::tuple<Series,Series> histogram(const Series& self, const Series & bins, const optional<Series> & weight, bool density) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("histogram", self, bins, weight, density);
}

::std::tuple<Series,Series> histogram(const Series& self, int64_t bins, optional<ArrayRef<double>> range, const optional<Series> & weight, bool density) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("histogram", self, bins, range, weight, density);
}

::std::vector<Series> hsplit(const Series& self, IntArrayRef indices) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("hsplit", self, indices);
}

::std::vector<Series> hsplit(const Series& self, int64_t sections) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("hsplit", self, sections);
}

Series hypot(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("hypot", self, other);
}

Series & hypot_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("hypot_", self, other);
}

Series i0(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("i0", self);
}

Series & i0_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("i0_", self);
}

Series igamma(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("igamma", self, other);
}

Series & igamma_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("igamma_", self, other);
}

Series igammac(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("igammac", self, other);
}

Series & igammac_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("igammac_", self, other);
}

Series index(const Series& self, const std::vector<std::optional<Series>>& indices) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index", self, indices);
}

Series index(const Series& self, std::span<const Series> indices) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index", self, indices);
}

Series& index_(Series& self, std::span<const Series> indices) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("index_", self, indices);
}

Series index_add(const Series& self, Dimname dim, const Series & index, const Series & source, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_add", self, dim, index, source, alpha);
}

Series index_add(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_add", self, dim, keepdim);
}

Series index_add(const Series& self, int64_t dim, const Series & index, const Series & source, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_add", self, dim, index, source, alpha);
}

Series & index_add_(Series& self, int64_t dim, const Series & index, const Series & source, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_add_", self, dim, index, source, alpha);
}

Series index_copy(const Series& self, Dimname dim, const Series & index, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_copy", self, dim, index, source);
}

Series index_copy(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_copy", self, dim, keepdim);
}

Series index_copy(const Series& self, int64_t dim, const Series & index, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_copy", self, dim, index, source);
}

Series & index_copy_(Series& self, Dimname dim, const Series & index, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_copy_", self, dim, index, source);
}

Series & index_copy_(Series& self, int64_t dim, const Series & index, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_copy_", self, dim, index, source);
}

Series index_fill(const Series& self, Dimname dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_fill", self, dim, index, value);
}

Series index_fill(const Series& self, Dimname dim, const Series & index, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_fill", self, dim, index, value);
}

Series index_fill(const Series& self, int64_t dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_fill", self, dim, index, value);
}

Series index_fill(const Series& self, int64_t dim, const Series & index, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_fill", self, dim, index, value);
}

Series & index_fill_(Series& self, Dimname dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_fill_", self, dim, index, value);
}

Series & index_fill_(Series& self, Dimname dim, const Series & index, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_fill_", self, dim, index, value);
}

Series & index_fill_(Series& self, int64_t dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_fill_", self, dim, index, value);
}

Series & index_fill_(Series& self, int64_t dim, const Series & index, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_fill_", self, dim, index, value);
}

Series index_put(const Series& self, const std::vector<optional<Series>>& indices, const Series & values, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_put", self, indices, values, accumulate);
}

Series index_put(const Series& self, std::span<const Series> indices, const Series& values, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_put", self, indices, values, accumulate);
}

Series index_put(const Series& self, std::span<const Series> indices, double values, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_put", self, indices, values, accumulate);
}

Series & index_put_(Series& self, const std::vector<optional<Series>>& indices, const Series & values, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_put_", self, indices, values, accumulate);
}

Series& index_put_(Series& self, std::span<const Series> indices, const Series& values, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("index_put_", self, indices, values, accumulate);
}

Series& index_put_(Series& self, std::span<const Series> indices, double values, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("index_put_", self, indices, values, accumulate);
}

Series index_reduce(const Series& self, int64_t dim, const Series & index, const Series & source, string_view reduce, bool include_self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_reduce", self, dim, index, source, reduce, include_self);
}

Series & index_reduce_(Series& self, int64_t dim, const Series & index, const Series & source, string_view reduce, bool include_self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("index_reduce_", self, dim, index, source, reduce, include_self);
}

Series index_select(const Series& self, Dimname dim, const Series & index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_select", self, dim, index);
}

Series index_select(const Series& self, int64_t dim, const Series & index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_select", self, dim, index);
}

Series index_select(const Series& self, std::int64_t dim, double index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("index_select", self, dim, index);
}

Series& index_select_(Series& self, std::int64_t dim, const Series& index) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("index_select_", self, dim, index);
}

Series& index_select_(Series& self, std::int64_t dim, double index) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("index_select_", self, dim, index);
}

Series indices(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("indices", self);
}

Series inner(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("inner", self, other);
}

Series int_repr(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("int_repr", self);
}

Series inverse(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("inverse", self);
}

bool is_coalesced(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("is_coalesced", self);
}

bool is_distributed(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("is_distributed", self);
}

bool is_nonzero(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("is_nonzero", self);
}

bool is_pinned(const Series& self, optional<Device> device) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("is_pinned", self, device);
}

bool is_same_size(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("is_same_size", self, other);
}

bool is_set_to(const Series& self, const Series & tensor) {
  return ::pluss::codem::native::cpu::unimplemented<bool>("is_set_to", self, tensor);
}

Series isclose(const Series& self, const Series & other, double rtol, double atol, bool equal_nan) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isclose", self, other, rtol, atol, equal_nan);
}

Series isfinite(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isfinite", self);
}

Series isinf(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isinf", self);
}

Series isnan(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isnan", self);
}

Series isneginf(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isneginf", self);
}

Series isposinf(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isposinf", self);
}

Series isreal(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("isreal", self);
}

Series istft(const Series& self, int64_t n_fft, optional<int64_t> hop_length, optional<int64_t> win_length, const optional<Series> & window, bool center, bool normalized, optional<bool> onesided, optional<int64_t> length, bool return_complex) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("istft", self, n_fft, hop_length, win_length, window, center, normalized, onesided, length, return_complex);
}

Scalar item(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Scalar>("item", self);
}

Series kron(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("kron", self, other);
}

::std::tuple<Series,Series> kthvalue(const Series& self, int64_t k, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("kthvalue", self, k, dim, keepdim);
}

Series kthvalue(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("kthvalue", self, dim, keepdim);
}

::std::tuple<Series,Series> kthvalue(const Series& self, int64_t k, int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("kthvalue", self, k, dim, keepdim);
}

Series lcm(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lcm", self, other);
}

Series & lcm_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("lcm_", self, other);
}

Series ldexp(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ldexp", self, other);
}

Series & ldexp_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("ldexp_", self, other);
}

Series le(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("le", self, other);
}

Series le(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("le", self, other);
}

Series le(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("le", self, b);
}

Series & le_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("le_", self, other);
}

Series & le_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("le_", self, other);
}

Series& le_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("le_", self, b);
}

Series lerp(const Series& self, const Series & end, const Scalar & weight) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lerp", self, end, weight);
}

Series lerp(const Series& self, const Series & end, const Series & weight) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lerp", self, end, weight);
}

Series lerp(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lerp", self, dim, keepdim);
}

Series & lerp_(Series& self, const Series & end, const Scalar & weight) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("lerp_", self, end, weight);
}

Series & lerp_(Series& self, const Series & end, const Series & weight) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("lerp_", self, end, weight);
}

Series less(const Series& self, const Scalar & other) {
  return lt(self, other);
}

Series less(const Series& self, const Series & other) {
  return lt(self, other);
}

Series & less_(Series& self, const Scalar & other) {
  return lt_(self, other);
}

Series & less_(Series& self, const Series & other) {
  return lt_(self, other);
}

Series less_equal(const Series& self, const Scalar & other) {
  return le(self, other);
}

Series less_equal(const Series& self, const Series & other) {
  return le(self, other);
}

Series & less_equal_(Series& self, const Scalar & other) {
  return le_(self, other);
}

Series & less_equal_(Series& self, const Series & other) {
  return le_(self, other);
}

Series lgamma(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lgamma", self);
}

Series& lgamma_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("lgamma_", self);
}

Series linalg_inv(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("linalg_inv", self, dim, keepdim);
}

Series linalg_qr(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("linalg_qr", self, dim, keepdim);
}

Series linalg_svd(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("linalg_svd", self, dim, keepdim);
}

Series log(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log", self);
}

Series log10(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log10", self);
}

Series & log10_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("log10_", self);
}

Series log1p(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log1p", self);
}

Series& log1p_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("log1p_", self);
}

Series log2(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log2", self);
}

Series & log2_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("log2_", self);
}

Series& log_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("log_", self);
}

Series & log_normal_(Series& self, double mean, double std, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("log_normal_", self, mean, std, generator);
}

Series log_softmax(const Series& self, Dimname dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log_softmax", self, dim, dtype);
}

Series log_softmax(const Series& self, std::int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log_softmax", self, dim);
}

Series log_softmax(const Series& self, int64_t dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("log_softmax", self, dim, dtype);
}

Series logaddexp(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logaddexp", self, other);
}

Series logaddexp2(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logaddexp2", self, other);
}

Series logcumsumexp(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logcumsumexp", self, dim);
}

Series logcumsumexp(const Series& self, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logcumsumexp", self, dim);
}

Series logdet(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logdet", self);
}

Series logical_and(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logical_and", self, other);
}

Series & logical_and_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("logical_and_", self, other);
}

Series logical_not(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logical_not", self);
}

Series& logical_not_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("logical_not_", self);
}

Series logical_or(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logical_or", self, other);
}

Series & logical_or_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("logical_or_", self, other);
}

Series logical_xor(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logical_xor", self, other);
}

Series & logical_xor_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("logical_xor_", self, other);
}

Series logit(const Series& self, optional<double> eps) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logit", self, eps);
}

Series & logit_(Series& self, optional<double> eps) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("logit_", self, eps);
}

Series logsumexp(const Series& self, DimnameList dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logsumexp", self, dim, keepdim);
}

Series logsumexp(const Series& self, IntArrayRef dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logsumexp", self, dim, keepdim);
}

Series logsumexp(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("logsumexp", self, dim, keepdim);
}

Series lt(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lt", self, other);
}

Series lt(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lt", self, other);
}

Series lt(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lt", self, b);
}

Series & lt_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("lt_", self, other);
}

Series & lt_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("lt_", self, other);
}

Series& lt_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("lt_", self, b);
}

Series lu_solve(const Series& self, const Series & LU_data, const Series & LU_pivots) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("lu_solve", self, LU_data, LU_pivots);
}

Series mH(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mH", self);
}

Series mT(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mT", self);
}

Series masked_fill(const Series& self, const Series & mask, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_fill", self, mask, value);
}

Series masked_fill(const Series& self, const Series & mask, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_fill", self, mask, value);
}

Series masked_fill(const Series& self, const Series& mask, double value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_fill", self, mask, value);
}

Series masked_fill(const Series& self, double mask, double value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_fill", self, mask, value);
}

Series & masked_fill_(Series& self, const Series & mask, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("masked_fill_", self, mask, value);
}

Series & masked_fill_(Series& self, const Series & mask, const Series & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("masked_fill_", self, mask, value);
}

Series& masked_fill_(Series& self, const Series& mask, double value) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("masked_fill_", self, mask, value);
}

Series& masked_fill_(Series& self, double mask, double value) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("masked_fill_", self, mask, value);
}

Series masked_scatter(const Series& self, const Series & mask, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_scatter", self, mask, source);
}

Series & masked_scatter_(Series& self, const Series & mask, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("masked_scatter_", self, mask, source);
}

Series masked_select(const Series& self, const Series & mask) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_select", self, mask);
}

Series masked_select(const Series& self, double mask) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("masked_select", self, mask);
}

Series& masked_select_(Series& self, const Series& mask) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("masked_select_", self, mask);
}

Series& masked_select_(Series& self, double mask) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("masked_select_", self, mask);
}

Series matmul(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("matmul", self, other);
}

Series matmul(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("matmul", self, b);
}

Series& matmul_(Series& self, const Series& b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("matmul_", self, b);
}

Series& matmul_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("matmul_", self, b);
}

Series matrix_H(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("matrix_H", self);
}

Series matrix_exp(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("matrix_exp", self);
}

Series matrix_power(const Series& self, int64_t n) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("matrix_power", self, n);
}

Series max(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("max", self);
}

::std::tuple<Series,Series> max(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("max", self, dim, keepdim);
}

Series max(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("max", self, other);
}

::std::tuple<Series,Series> max(const Series& self, int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("max", self, dim, keepdim);
}

Series max_pool1d(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("max_pool1d", self, dim, keepdim);
}

Series max_pool2d(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("max_pool2d", self, dim, keepdim);
}

Series maximum(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("maximum", self, other);
}

Series maximum(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("maximum", self, rhs);
}

Series& maximum_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("maximum_", self, rhs);
}

Series& maximum_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("maximum_", self, rhs);
}

Series mean(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mean", self);
}

Series mean(const Series& self, DimnameList dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mean", self, dim, keepdim, dtype);
}

Series mean(const Series& self, OptionalIntArrayRef dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mean", self, dim, keepdim, dtype);
}

Series mean(const Series& self, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mean", self, dtype);
}

Series mean(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mean", self, dim, keepdim);
}

Series mean(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mean", self, dims, keepdim);
}

Series& mean_(Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("mean_", self, dim, keepdim);
}

Series median(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("median", self);
}

::std::tuple<Series,Series> median(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("median", self, dim, keepdim);
}

Series median(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("median", self, dim, keepdim);
}

Series min(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("min", self);
}

::std::tuple<Series,Series> min(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("min", self, dim, keepdim);
}

Series min(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("min", self, other);
}

::std::tuple<Series,Series> min(const Series& self, int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("min", self, dim, keepdim);
}

Series minimum(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("minimum", self, other);
}

Series minimum(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("minimum", self, rhs);
}

Series& minimum_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("minimum_", self, rhs);
}

Series& minimum_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("minimum_", self, rhs);
}

Series mm(const Series& self, const Series & mat2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mm", self, mat2);
}

Series mm(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mm", self, dim, keepdim);
}

::std::tuple<Series,Series> mode(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("mode", self, dim, keepdim);
}

::std::tuple<Series,Series> mode(const Series& self, int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("mode", self, dim, keepdim);
}

Series moveaxis(const Series& self, IntArrayRef source, IntArrayRef destination) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("moveaxis", self, source, destination);
}

Series moveaxis(const Series& self, int64_t source, int64_t destination) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("moveaxis", self, source, destination);
}

Series movedim(const Series& self, IntArrayRef source, IntArrayRef destination) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("movedim", self, source, destination);
}

Series movedim(const Series& self, int64_t source, int64_t destination) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("movedim", self, source, destination);
}

Series msort(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("msort", self);
}

Series mul(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mul", self, other);
}

Series mul(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mul", self, other);
}

Series mul(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mul", self, rhs);
}

Series & mul_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("mul_", self, other);
}

Series & mul_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("mul_", self, other);
}

Series& mul_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("mul_", self, rhs);
}

Series& mul_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("mul_", self, rhs);
}

Series multinomial(const Series& self, int64_t num_samples, bool replacement, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("multinomial", self, num_samples, replacement, generator);
}

Series multiply(const Series& self, const Scalar & other) {
  return mul(self, other);
}

Series multiply(const Series& self, const Series & other) {
  return mul(self, other);
}

Series & multiply_(Series& self, const Scalar & other) {
  return mul_(self, other);
}

Series & multiply_(Series& self, const Series & other) {
  return mul_(self, other);
}

Series mv(const Series& self, const Series & vec) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mv", self, vec);
}

Series mv(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mv", self, dim, keepdim);
}

Series mvlgamma(const Series& self, int64_t p) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("mvlgamma", self, p);
}

Series & mvlgamma_(Series& self, int64_t p) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("mvlgamma_", self, p);
}

Series nan_to_num(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nan_to_num", self);
}

Series nan_to_num(const Series& self, optional<double> nan, optional<double> posinf, optional<double> neginf) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nan_to_num", self, nan, posinf, neginf);
}

Series nan_to_num(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nan_to_num", self, dim, keepdim);
}

Series & nan_to_num_(Series& self, optional<double> nan, optional<double> posinf, optional<double> neginf) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("nan_to_num_", self, nan, posinf, neginf);
}

Series nanmean(const Series& self, OptionalIntArrayRef dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nanmean", self, dim, keepdim, dtype);
}

Series nanmedian(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nanmedian", self);
}

::std::tuple<Series,Series> nanmedian(const Series& self, Dimname dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("nanmedian", self, dim, keepdim);
}

::std::tuple<Series,Series> nanmedian(const Series& self, int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("nanmedian", self, dim, keepdim);
}

Series nanquantile(const Series& self, const Series & q, optional<int64_t> dim, bool keepdim, string_view interpolation) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nanquantile", self, q, dim, keepdim, interpolation);
}

Series nanquantile(const Series& self, double q, optional<int64_t> dim, bool keepdim, string_view interpolation) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nanquantile", self, q, dim, keepdim, interpolation);
}

Series nansum(const Series& self, OptionalIntArrayRef dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nansum", self, dim, keepdim, dtype);
}

Series narrow(const Series& self, int64_t dim, const Series & start, int64_t length) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("narrow", self, dim, start, length);
}

Series narrow(const Series& self, int64_t dim, int64_t start, int64_t length) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("narrow", self, dim, start, length);
}

Series narrow_copy(const Series& self, int64_t dim, int64_t start, int64_t length) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("narrow_copy", self, dim, start, length);
}

Series narrow_copy_symint(const Series& self, int64_t dim, SymInt start, SymInt length) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("narrow_copy_symint", self, dim, start, length);
}

Series narrow_symint(const Series& self, int64_t dim, SymInt start, SymInt length) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("narrow_symint", self, dim, start, length);
}

Series narrow_symint(const Series& self, int64_t dim, const Series & start, SymInt length) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("narrow_symint", self, dim, start, length);
}

Series ne(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ne", self, other);
}

Series ne(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ne", self, other);
}

Series ne(const Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ne", self, b);
}

Series & ne_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("ne_", self, other);
}

Series & ne_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("ne_", self, other);
}

Series& ne_(Series& self, double b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("ne_", self, b);
}

Series neg(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("neg", self);
}

Series& neg_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("neg_", self);
}

Series negative(const Series& self) {
  return neg(self);
}

Series & negative_(Series& self) {
  return neg_(self);
}

Series new_empty(const Series& self, IntArrayRef size, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty", self, size, options);
}

Series new_empty(const Series& self, IntArrayRef size, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty", self, size, dtype, layout, device, pin_memory);
}

Series new_empty_strided(const Series& self, IntArrayRef size, IntArrayRef stride, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty_strided", self, size, stride, options);
}

Series new_empty_strided(const Series& self, IntArrayRef size, IntArrayRef stride, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty_strided", self, size, stride, dtype, layout, device, pin_memory);
}

Series new_empty_strided_symint(const Series& self, SymIntArrayRef size, SymIntArrayRef stride, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty_strided_symint", self, size, stride, options);
}

Series new_empty_strided_symint(const Series& self, SymIntArrayRef size, SymIntArrayRef stride, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty_strided_symint", self, size, stride, dtype, layout, device, pin_memory);
}

Series new_empty_symint(const Series& self, SymIntArrayRef size, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty_symint", self, size, options);
}

Series new_empty_symint(const Series& self, SymIntArrayRef size, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_empty_symint", self, size, dtype, layout, device, pin_memory);
}

Series new_full(const Series& self, IntArrayRef size, const Scalar & fill_value, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_full", self, size, fill_value, options);
}

Series new_full(const Series& self, IntArrayRef size, const Scalar & fill_value, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_full", self, size, fill_value, dtype, layout, device, pin_memory);
}

Series new_full_symint(const Series& self, SymIntArrayRef size, const Scalar & fill_value, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_full_symint", self, size, fill_value, options);
}

Series new_full_symint(const Series& self, SymIntArrayRef size, const Scalar & fill_value, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_full_symint", self, size, fill_value, dtype, layout, device, pin_memory);
}

Series new_ones(const Series& self, IntArrayRef size, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_ones", self, size, options);
}

Series new_ones(const Series& self, IntArrayRef size, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_ones", self, size, dtype, layout, device, pin_memory);
}

Series new_ones_symint(const Series& self, SymIntArrayRef size, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_ones_symint", self, size, options);
}

Series new_ones_symint(const Series& self, SymIntArrayRef size, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_ones_symint", self, size, dtype, layout, device, pin_memory);
}

Series new_zeros(const Series& self, IntArrayRef size, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_zeros", self, size, options);
}

Series new_zeros(const Series& self, IntArrayRef size, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_zeros", self, size, dtype, layout, device, pin_memory);
}

Series new_zeros_symint(const Series& self, SymIntArrayRef size, SeriesOptions options) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_zeros_symint", self, size, options);
}

Series new_zeros_symint(const Series& self, SymIntArrayRef size, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("new_zeros_symint", self, size, dtype, layout, device, pin_memory);
}

Series nextafter(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nextafter", self, other);
}

Series & nextafter_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("nextafter_", self, other);
}

Series nonzero(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nonzero", self);
}

Series& nonzero_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("nonzero_", self);
}

::std::vector<Series> nonzero_numpy(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("nonzero_numpy", self);
}

Series nonzero_static(const Series& self, int64_t size, int64_t fill_value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("nonzero_static", self, size, fill_value);
}

Series norm(const Series& self, const Scalar & p) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, p);
}

Series norm(const Series& self, const optional<Scalar> & p, DimnameList dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, p, dim, keepdim);
}

Series norm(const Series& self, const optional<Scalar> & p, DimnameList dim, bool keepdim, ScalarType dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, p, dim, keepdim, dtype);
}

Series norm(const Series& self, const optional<Scalar> & p, IntArrayRef dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, p, dim, keepdim);
}

Series norm(const Series& self, const optional<Scalar> & p, IntArrayRef dim, bool keepdim, ScalarType dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, p, dim, keepdim, dtype);
}

Series norm(const Series& self, const optional<Scalar> & p, ScalarType dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, p, dtype);
}

Series norm(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("norm", self, dim, keepdim);
}

Series & normal_(Series& self, double mean, double std, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("normal_", self, mean, std, generator);
}

Series not_equal(const Series& self, const Scalar & other) {
  return ne(self, other);
}

Series not_equal(const Series& self, const Series & other) {
  return ne(self, other);
}

Series & not_equal_(Series& self, const Scalar & other) {
  return ne_(self, other);
}

Series & not_equal_(Series& self, const Series & other) {
  return ne_(self, other);
}

Series numpy_T(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("numpy_T", self);
}

SeriesOptions options_config(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<SeriesOptions>("options_config", self);
}

Series orgqr(const Series& self, const Series & input2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("orgqr", self, input2);
}

Series ormqr(const Series& self, const Series & input2, const Series & input3, bool left, bool transpose) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ormqr", self, input2, input3, left, transpose);
}

Series outer(const Series& self, const Series & vec2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("outer", self, vec2);
}

Series pad(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("pad", self, dim, keepdim);
}

Series permute(const Series& self, IntArrayRef dims) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("permute", self, dims);
}

Series pin_memory(const Series& self, optional<Device> device) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("pin_memory", self, device);
}

Series pinverse(const Series& self, double rcond) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("pinverse", self, rcond);
}

Device platform(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Device>("platform", self);
}

Series polygamma(const Series& self, int64_t n) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("polygamma", self, n);
}

Series & polygamma_(Series& self, int64_t n) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("polygamma_", self, n);
}

Series positive(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("positive", self);
}

Series pow(const Series& self, const Scalar & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("pow", self, exponent);
}

Series pow(const Series& self, const Series & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("pow", self, exponent);
}

Series pow(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("pow", self, rhs);
}

Series & pow_(Series& self, const Scalar & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("pow_", self, exponent);
}

Series & pow_(Series& self, const Series & exponent) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("pow_", self, exponent);
}

Series& pow_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("pow_", self, rhs);
}

Series& pow_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("pow_", self, rhs);
}

Series prelu(const Series& self, const Series & weight) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prelu", self, weight);
}

Series prod(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prod", self);
}

Series prod(const Series& self, Dimname dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prod", self, dim, keepdim, dtype);
}

Series prod(const Series& self, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prod", self, dtype);
}

Series prod(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prod", self, dim, keepdim);
}

Series prod(const Series& self, int64_t dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prod", self, dim, keepdim, dtype);
}

Series prod(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("prod", self, dims, keepdim);
}

Series put(const Series& self, const Series & index, const Series & source, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("put", self, index, source, accumulate);
}

Series put(const Series& self, const Series& index, const Series& src, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("put", self, index, src, accumulate);
}

Series & put_(Series& self, const Series & index, const Series & source, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("put_", self, index, source, accumulate);
}

Series& put_(Series& self, const Series& index, const Series& src, bool accumulate) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("put_", self, index, src, accumulate);
}

int64_t q_per_channel_axis(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("q_per_channel_axis", self);
}

Series q_per_channel_scales(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("q_per_channel_scales", self);
}

Series q_per_channel_zero_points(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("q_per_channel_zero_points", self);
}

double q_scale(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<double>("q_scale", self);
}

int64_t q_zero_point(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("q_zero_point", self);
}

::std::tuple<Series,Series> qr(const Series& self, bool some) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("qr", self, some);
}

QScheme qscheme(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<QScheme>("qscheme", self);
}

Series quantile(const Series& self, const Series & q, optional<int64_t> dim, bool keepdim, string_view interpolation) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("quantile", self, q, dim, keepdim, interpolation);
}

Series quantile(const Series& self, double q, optional<int64_t> dim, bool keepdim, string_view interpolation) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("quantile", self, q, dim, keepdim, interpolation);
}

Series quantile(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("quantile", self, dim, keepdim);
}

Series rad2deg(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("rad2deg", self);
}

Series & rad2deg_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("rad2deg_", self);
}

Series & random_(Series& self, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("random_", self, generator);
}

Series & random_(Series& self, int64_t to, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("random_", self, to, generator);
}

Series & random_(Series& self, int64_t from, optional<int64_t> to, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("random_", self, from, to, generator);
}

Series ravel(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("ravel", self);
}

Series reciprocal(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("reciprocal", self);
}

Series& reciprocal_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("reciprocal_", self);
}

void record_stream(const Series& self, Stream s) {
  ::pluss::codem::native::cpu::unimplemented_void("record_stream", self, s);
}

Series refine_names(const Series& self, DimnameList names) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("refine_names", self, names);
}

Series relu(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("relu", self);
}

Series& relu_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("relu_", self);
}

Series remainder(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("remainder", self, other);
}

Series remainder(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("remainder", self, other);
}

Series & remainder_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("remainder_", self, other);
}

Series & remainder_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("remainder_", self, other);
}

Series rename(const Series& self, optional<DimnameList> names) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("rename", self, names);
}

Series & rename_(Series& self, optional<DimnameList> names) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("rename_", self, names);
}

Series renorm(const Series& self, const Scalar & p, int64_t dim, const Scalar & maxnorm) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("renorm", self, p, dim, maxnorm);
}

Series & renorm_(Series& self, const Scalar & p, int64_t dim, const Scalar & maxnorm) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("renorm_", self, p, dim, maxnorm);
}

Series repeat(const Series& self, IntArrayRef repeats) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat", self, repeats);
}

Series repeat(const Series& self, std::span<const std::int64_t> repeats) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat", self, repeats);
}

Series repeat_interleave(const Series& self, const Series & repeats, optional<int64_t> dim, optional<int64_t> output_size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat_interleave", self, repeats, dim, output_size);
}

Series repeat_interleave(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat_interleave", self, dim, keepdim);
}

Series repeat_interleave(const Series& self, int64_t repeats, optional<int64_t> dim, optional<int64_t> output_size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat_interleave", self, repeats, dim, output_size);
}

Series repeat_interleave_symint(const Series& self, SymInt repeats, optional<int64_t> dim, optional<SymInt> output_size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat_interleave_symint", self, repeats, dim, output_size);
}

Series repeat_interleave_symint(const Series& self, const Series & repeats, optional<int64_t> dim, optional<SymInt> output_size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat_interleave_symint", self, repeats, dim, output_size);
}

Series repeat_symint(const Series& self, SymIntArrayRef repeats) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("repeat_symint", self, repeats);
}

Series reshape(const Series& self, IntArrayRef shape) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("reshape", self, shape);
}

Series reshape_as(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("reshape_as", self, other);
}

Series reshape_symint(const Series& self, SymIntArrayRef shape) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("reshape_symint", self, shape);
}

const Series & resize_(Series& self, IntArrayRef size, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("resize_", self, size, memory_format);
}

const Series & resize__symint(Series& self, SymIntArrayRef size, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("resize__symint", self, size, memory_format);
}

const Series & resize_as_(Series& self, const Series & the_template, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("resize_as_", self, the_template, memory_format);
}

const Series & resize_as_sparse_(Series& self, const Series & the_template) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("resize_as_sparse_", self, the_template);
}

Series resolve_conj(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("resolve_conj", self);
}

Series resolve_neg(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("resolve_neg", self);
}

Series roll(const Series& self, IntArrayRef shifts, IntArrayRef dims) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("roll", self, shifts, dims);
}

Series roll(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("roll", self, dim, keepdim);
}

Series roll_symint(const Series& self, SymIntArrayRef shifts, IntArrayRef dims) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("roll_symint", self, shifts, dims);
}

Series rot90(const Series& self, int64_t k, IntArrayRef dims, 1}) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("rot90", self, k, dims, 1});
}

Series rot90(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("rot90", self, dim, keepdim);
}

Series round(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("round", self);
}

Series round(const Series& self, int64_t decimals) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("round", self, decimals);
}

Series& round_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("round_", self);
}

Series & round_(Series& self, int64_t decimals) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("round_", self, decimals);
}

Series row_indices(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("row_indices", self);
}

Series rsqrt(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("rsqrt", self);
}

Series& rsqrt_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("rsqrt_", self);
}

Series scatter(const Series& self, Dimname dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter", self, dim, index, value);
}

Series scatter(const Series& self, Dimname dim, const Series & index, const Series & src) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter", self, dim, index, src);
}

Series scatter(const Series& self, int64_t dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter", self, dim, index, value);
}

Series scatter(const Series& self, int64_t dim, const Series & index, const Scalar & value, string_view reduce) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter", self, dim, index, value, reduce);
}

Series scatter(const Series& self, int64_t dim, const Series & index, const Series & src) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter", self, dim, index, src);
}

Series scatter(const Series& self, int64_t dim, const Series & index, const Series & src, string_view reduce) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter", self, dim, index, src, reduce);
}

Series & scatter_(Series& self, int64_t dim, const Series & index, const Scalar & value) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("scatter_", self, dim, index, value);
}

Series & scatter_(Series& self, int64_t dim, const Series & index, const Scalar & value, string_view reduce) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("scatter_", self, dim, index, value, reduce);
}

Series & scatter_(Series& self, int64_t dim, const Series & index, const Series & src) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("scatter_", self, dim, index, src);
}

Series & scatter_(Series& self, int64_t dim, const Series & index, const Series & src, string_view reduce) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("scatter_", self, dim, index, src, reduce);
}

Series scatter_add(const Series& self, Dimname dim, const Series & index, const Series & src) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter_add", self, dim, index, src);
}

Series scatter_add(const Series& self, int64_t dim, const Series & index, const Series & src) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter_add", self, dim, index, src);
}

Series & scatter_add_(Series& self, int64_t dim, const Series & index, const Series & src) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("scatter_add_", self, dim, index, src);
}

Series scatter_reduce(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter_reduce", self, dim, keepdim);
}

Series scatter_reduce(const Series& self, int64_t dim, const Series & index, const Series & src, string_view reduce, bool include_self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("scatter_reduce", self, dim, index, src, reduce, include_self);
}

Series & scatter_reduce_(Series& self, int64_t dim, const Series & index, const Series & src, string_view reduce, bool include_self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("scatter_reduce_", self, dim, index, src, reduce, include_self);
}

Series select(const Series& self, Dimname dim, int64_t index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("select", self, dim, index);
}

Series select(const Series& self, int64_t dim, int64_t index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("select", self, dim, index);
}

Series select_scatter(const Series& self, const Series & src, int64_t dim, int64_t index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("select_scatter", self, src, dim, index);
}

Series select_scatter_symint(const Series& self, const Series & src, int64_t dim, SymInt index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("select_scatter_symint", self, src, dim, index);
}

Series select_symint(const Series& self, int64_t dim, SymInt index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("select_symint", self, dim, index);
}

Series & set_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set_", self);
}

Series & set_(Series& self, Storage source) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set_", self, source);
}

Series & set_(Series& self, Storage source, int64_t storage_offset, IntArrayRef size, IntArrayRef stride) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set_", self, source, storage_offset, size, stride);
}

Series & set_(Series& self, const Series & source) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set_", self, source);
}

Series & set_(Series& self, const Series & source, int64_t storage_offset, IntArrayRef size, IntArrayRef stride) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set_", self, source, storage_offset, size, stride);
}

Series & set__symint(Series& self, Storage source, SymInt storage_offset, SymIntArrayRef size, SymIntArrayRef stride) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set__symint", self, source, storage_offset, size, stride);
}

Series & set__symint(Series& self, const Series & source, SymInt storage_offset, SymIntArrayRef size, SymIntArrayRef stride) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("set__symint", self, source, storage_offset, size, stride);
}

Series sgn(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sgn", self);
}

Series & sgn_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("sgn_", self);
}

Series sigmoid(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sigmoid", self);
}

Series& sigmoid_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sigmoid_", self);
}

Series sign(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sign", self);
}

Series& sign_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sign_", self);
}

Series signbit(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("signbit", self);
}

Series sin(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sin", self);
}

Series& sin_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sin_", self);
}

Series sinc(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sinc", self);
}

Series & sinc_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("sinc_", self);
}

Series sinh(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sinh", self);
}

Series& sinh_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sinh_", self);
}

int64_t size(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("size", self, dim);
}

Series slice(const Series& self, int64_t dim, optional<int64_t> start, optional<int64_t> end, int64_t step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("slice", self, dim, start, end, step);
}

Series slice_inverse(const Series& self, const Series & src, int64_t dim, optional<int64_t> start, optional<int64_t> end, int64_t step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("slice_inverse", self, src, dim, start, end, step);
}

Series slice_inverse_symint(const Series& self, const Series & src, int64_t dim, optional<SymInt> start, optional<SymInt> end, SymInt step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("slice_inverse_symint", self, src, dim, start, end, step);
}

Series slice_scatter(const Series& self, const Series & src, int64_t dim, optional<int64_t> start, optional<int64_t> end, int64_t step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("slice_scatter", self, src, dim, start, end, step);
}

Series slice_scatter_symint(const Series& self, const Series & src, int64_t dim, optional<SymInt> start, optional<SymInt> end, SymInt step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("slice_scatter_symint", self, src, dim, start, end, step);
}

Series slice_symint(const Series& self, int64_t dim, optional<SymInt> start, optional<SymInt> end, SymInt step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("slice_symint", self, dim, start, end, step);
}

::std::tuple<Series,Series> slogdet(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("slogdet", self);
}

Series smm(const Series& self, const Series & mat2) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("smm", self, mat2);
}

Series softmax(const Series& self, Dimname dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("softmax", self, dim, dtype);
}

Series softmax(const Series& self, std::int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("softmax", self, dim);
}

Series softmax(const Series& self, int64_t dim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("softmax", self, dim, dtype);
}

::std::tuple<Series,Series> sort(const Series& self, Dimname dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("sort", self, dim, descending);
}

::std::tuple<Series,Series> sort(const Series& self, optional<bool> stable, Dimname dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("sort", self, stable, dim, descending);
}

::std::tuple<Series,Series> sort(const Series& self, optional<bool> stable, int64_t dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("sort", self, stable, dim, descending);
}

::std::tuple<Series,Series> sort(const Series& self, int64_t dim, bool descending) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("sort", self, dim, descending);
}

int64_t sparse_dim(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("sparse_dim", self);
}

Series sparse_mask(const Series& self, const Series & mask) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sparse_mask", self, mask);
}

const Series & sparse_resize_(Series& self, IntArrayRef size, int64_t sparse_dim, int64_t dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("sparse_resize_", self, size, sparse_dim, dense_dim);
}

const Series & sparse_resize_and_clear_(Series& self, IntArrayRef size, int64_t sparse_dim, int64_t dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<const Series &>("sparse_resize_and_clear_", self, size, sparse_dim, dense_dim);
}

::std::vector<Series> split(const Series& self, IntArrayRef split_size, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("split", self, split_size, dim);
}

::std::vector<Series> split(const Series& self, int64_t split_size, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("split", self, split_size, dim);
}

::std::vector<Series> split_symint(const Series& self, SymInt split_size, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("split_symint", self, split_size, dim);
}

::std::vector<Series> split_symint(const Series& self, SymIntArrayRef split_size, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("split_symint", self, split_size, dim);
}

::std::vector<Series> split_with_sizes(const Series& self, IntArrayRef split_sizes, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("split_with_sizes", self, split_sizes, dim);
}

::std::vector<Series> split_with_sizes_symint(const Series& self, SymIntArrayRef split_sizes, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("split_with_sizes_symint", self, split_sizes, dim);
}

Series sqrt(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sqrt", self);
}

Series& sqrt_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sqrt_", self);
}

Series square(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("square", self);
}

Series& square_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("square_", self);
}

Series squeeze(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("squeeze", self);
}

Series squeeze(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("squeeze", self, dim);
}

Series squeeze(const Series& self, IntArrayRef dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("squeeze", self, dim);
}

Series squeeze(const Series& self, std::int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("squeeze", self, dim);
}

Series & squeeze_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("squeeze_", self);
}

Series & squeeze_(Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("squeeze_", self, dim);
}

Series & squeeze_(Series& self, IntArrayRef dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("squeeze_", self, dim);
}

Series & squeeze_(Series& self, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("squeeze_", self, dim);
}

Series sspaddmm(const Series& self, const Series & mat1, const Series & mat2, const Scalar & beta, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sspaddmm", self, mat1, mat2, beta, alpha);
}

Series std(const Series& self, DimnameList dim, bool unbiased, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("std", self, dim, unbiased, keepdim);
}

Series std(const Series& self, DimnameList dim, const optional<Scalar> & correction, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("std", self, dim, correction, keepdim);
}

Series std(const Series& self, OptionalIntArrayRef dim, bool unbiased, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("std", self, dim, unbiased, keepdim);
}

Series std(const Series& self, OptionalIntArrayRef dim, const optional<Scalar> & correction, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("std", self, dim, correction, keepdim);
}

Series std(const Series& self, bool unbiased) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("std", self, unbiased);
}

Series std(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("std", self, dim, keepdim);
}

Series stft(const Series& self, int64_t n_fft, optional<int64_t> hop_length, optional<int64_t> win_length, const optional<Series> & window, bool normalized, optional<bool> onesided, optional<bool> return_complex) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("stft", self, n_fft, hop_length, win_length, window, normalized, onesided, return_complex);
}

Series stft(const Series& self, int64_t n_fft, optional<int64_t> hop_length, optional<int64_t> win_length, const optional<Series> & window, bool center, string_view pad_mode, bool normalized, optional<bool> onesided, optional<bool> return_complex) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("stft", self, n_fft, hop_length, win_length, window, center, pad_mode, normalized, onesided, return_complex);
}

int64_t stride(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<int64_t>("stride", self, dim);
}

Series sub(const Series& self, const Scalar & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sub", self, other, alpha);
}

Series sub(const Series& self, const Series & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sub", self, other, alpha);
}

Series sub(const Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sub", self, rhs);
}

Series sub(const Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sub", self, rhs);
}

Series & sub_(Series& self, const Scalar & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("sub_", self, other, alpha);
}

Series & sub_(Series& self, const Series & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("sub_", self, other, alpha);
}

Series& sub_(Series& self, const Series& rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sub_", self, rhs);
}

Series& sub_(Series& self, double rhs) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sub_", self, rhs);
}

Series subtract(const Series& self, const Scalar & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("subtract", self, other, alpha);
}

Series subtract(const Series& self, const Series & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("subtract", self, other, alpha);
}

Series & subtract_(Series& self, const Scalar & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("subtract_", self, other, alpha);
}

Series & subtract_(Series& self, const Series & other, const Scalar & alpha) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("subtract_", self, other, alpha);
}

Series sum(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum", self);
}

Series sum(const Series& self, DimnameList dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum", self, dim, keepdim, dtype);
}

Series sum(const Series& self, OptionalIntArrayRef dim, bool keepdim, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum", self, dim, keepdim, dtype);
}

Series sum(const Series& self, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum", self, dtype);
}

Series sum(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum", self, dim, keepdim);
}

Series sum(const Series& self, std::span<const std::int64_t> dims, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum", self, dims, keepdim);
}

Series& sum_(Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sum_", self, dim, keepdim);
}

Series sum_all(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum_all", self);
}

Series& sum_all_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("sum_all_", self);
}

Series sum_to_size(const Series& self, IntArrayRef size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum_to_size", self, size);
}

Series sum_to_size_symint(const Series& self, SymIntArrayRef size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("sum_to_size_symint", self, size);
}

::std::tuple<Series,Series,Series> svd(const Series& self, bool some, bool compute_uv) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series,Series>>("svd", self, some, compute_uv);
}

Series swapaxes(const Series& self, int64_t axis0, int64_t axis1) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("swapaxes", self, axis0, axis1);
}

Series & swapaxes_(Series& self, int64_t axis0, int64_t axis1) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("swapaxes_", self, axis0, axis1);
}

Series swapdims(const Series& self, int64_t dim0, int64_t dim1) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("swapdims", self, dim0, dim1);
}

Series & swapdims_(Series& self, int64_t dim0, int64_t dim1) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("swapdims_", self, dim0, dim1);
}

Series t(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("t", self);
}

Series & t_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("t_", self);
}

Series take(const Series& self, const Series & index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("take", self, index);
}

Series take(const Series& self, double index) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("take", self, index);
}

Series& take_(Series& self, const Series& index) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("take_", self, index);
}

Series& take_(Series& self, double index) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("take_", self, index);
}

Series take_along_dim(const Series& self, const Series & indices, optional<int64_t> dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("take_along_dim", self, indices, dim);
}

Series take_along_dim(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("take_along_dim", self, dim, keepdim);
}

Series tan(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("tan", self);
}

Series& tan_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("tan_", self);
}

Series tanh(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("tanh", self);
}

Series& tanh_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("tanh_", self);
}

::std::vector<Series> tensor_split(const Series& self, IntArrayRef indices, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("tensor_split", self, indices, dim);
}

::std::vector<Series> tensor_split(const Series& self, const Series & tensor_indices_or_sections, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("tensor_split", self, tensor_indices_or_sections, dim);
}

::std::vector<Series> tensor_split(const Series& self, int64_t sections, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("tensor_split", self, sections, dim);
}

::std::vector<Series> tensor_split_symint(const Series& self, SymInt sections, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("tensor_split_symint", self, sections, dim);
}

::std::vector<Series> tensor_split_symint(const Series& self, SymIntArrayRef indices, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("tensor_split_symint", self, indices, dim);
}

Series tile(const Series& self, IntArrayRef dims) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("tile", self, dims);
}

Series tile(const Series& self, std::span<const std::int64_t> repeats) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("tile", self, repeats);
}

Series tile_symint(const Series& self, SymIntArrayRef dims) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("tile_symint", self, dims);
}

Series to(const Series& self, Device device, ScalarType dtype, bool non_blocking, bool copy, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to", self, device, dtype, non_blocking, copy, memory_format);
}

Series to(const Series& self, ScalarType dtype, bool non_blocking, bool copy, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to", self, dtype, non_blocking, copy, memory_format);
}

Series to(const Series& self, SeriesOptions options, bool non_blocking, bool copy, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to", self, options, non_blocking, copy, memory_format);
}

Series to(const Series& self, const Series & other, bool non_blocking, bool copy, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to", self, other, non_blocking, copy, memory_format);
}

Series to(const Series& self, optional<ScalarType> dtype, optional<Layout> layout, optional<Device> device, optional<bool> pin_memory, bool non_blocking, bool copy, optional<MemoryFormat> memory_format) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to", self, dtype, layout, device, pin_memory, non_blocking, copy, memory_format);
}

Series to_cpu(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_cpu", self);
}

Series to_cuda(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_cuda", self);
}

Series to_dense(const Series& self, optional<ScalarType> dtype, optional<bool> masked_grad) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_dense", self, dtype, masked_grad);
}

Series to_mkldnn(const Series& self, optional<ScalarType> dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_mkldnn", self, dtype);
}

Series to_mps(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_mps", self);
}

Series to_padded_tensor(const Series& self, double padding, OptionalIntArrayRef output_size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_padded_tensor", self, padding, output_size);
}

Series to_padded_tensor_symint(const Series& self, double padding, OptionalSymIntArrayRef output_size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_padded_tensor_symint", self, padding, output_size);
}

Series to_sparse(const Series& self, optional<Layout> layout, OptionalIntArrayRef blocksize, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_sparse", self, layout, blocksize, dense_dim);
}

Series to_sparse(const Series& self, int64_t sparse_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_sparse", self, sparse_dim);
}

Series to_sparse_bsc(const Series& self, IntArrayRef blocksize, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_sparse_bsc", self, blocksize, dense_dim);
}

Series to_sparse_bsr(const Series& self, IntArrayRef blocksize, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_sparse_bsr", self, blocksize, dense_dim);
}

Series to_sparse_csc(const Series& self, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_sparse_csc", self, dense_dim);
}

Series to_sparse_csr(const Series& self, optional<int64_t> dense_dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("to_sparse_csr", self, dense_dim);
}

Series topk(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("topk", self, dim, keepdim);
}

::std::tuple<Series,Series> topk(const Series& self, int64_t k, int64_t dim, bool largest, bool sorted) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("topk", self, k, dim, largest, sorted);
}

::std::tuple<Series,Series> topk_symint(const Series& self, SymInt k, int64_t dim, bool largest, bool sorted) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("topk_symint", self, k, dim, largest, sorted);
}

Series trace(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("trace", self);
}

Series transpose(const Series& self, Dimname dim0, Dimname dim1) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("transpose", self, dim0, dim1);
}

Series transpose(const Series& self, int64_t dim0, int64_t dim1) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("transpose", self, dim0, dim1);
}

Series & transpose_(Series& self, int64_t dim0, int64_t dim1) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("transpose_", self, dim0, dim1);
}

::std::tuple<Series,Series> triangular_solve(const Series& self, const Series & A, bool upper, bool transpose, bool unitriangular) {
  return ::pluss::codem::native::cpu::unimplemented<::std::tuple<Series,Series>>("triangular_solve", self, A, upper, transpose, unitriangular);
}

Series tril(const Series& self, int64_t diagonal) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("tril", self, diagonal);
}

Series & tril_(Series& self, int64_t diagonal) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("tril_", self, diagonal);
}

Series triu(const Series& self, int64_t diagonal) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("triu", self, diagonal);
}

Series & triu_(Series& self, int64_t diagonal) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("triu_", self, diagonal);
}

Series true_divide(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("true_divide", self, other);
}

Series true_divide(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("true_divide", self, other);
}

Series & true_divide_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("true_divide_", self, other);
}

Series & true_divide_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("true_divide_", self, other);
}

Series trunc(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("trunc", self);
}

Series& trunc_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("trunc_", self);
}

Series type_as(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("type_as", self, other);
}

::std::vector<Series> unbind(const Series& self, Dimname dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unbind", self, dim);
}

::std::vector<Series> unbind(const Series& self, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unbind", self, dim);
}

Series unflatten(const Series& self, Dimname dim, IntArrayRef sizes, DimnameList names) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unflatten", self, dim, sizes, names);
}

Series unflatten(const Series& self, int64_t dim, IntArrayRef sizes) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unflatten", self, dim, sizes);
}

Series unflatten_symint(const Series& self, Dimname dim, SymIntArrayRef sizes, DimnameList names) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unflatten_symint", self, dim, sizes, names);
}

Series unflatten_symint(const Series& self, int64_t dim, SymIntArrayRef sizes) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unflatten_symint", self, dim, sizes);
}

Series unfold(const Series& self, std::int64_t dim, std::int64_t size, std::int64_t step) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unfold", self, dim, size, step);
}

Series & uniform_(Series& self, double from, double to, optional<Generator> generator) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("uniform_", self, from, to, generator);
}

Series unique(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unique", self);
}

Series unique(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unique", self, dim, keepdim);
}

::std::vector<Series> unsafe_chunk(const Series& self, int64_t chunks, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unsafe_chunk", self, chunks, dim);
}

::std::vector<Series> unsafe_split(const Series& self, int64_t split_size, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unsafe_split", self, split_size, dim);
}

::std::vector<Series> unsafe_split_symint(const Series& self, SymInt split_size, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unsafe_split_symint", self, split_size, dim);
}

::std::vector<Series> unsafe_split_with_sizes(const Series& self, IntArrayRef split_sizes, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unsafe_split_with_sizes", self, split_sizes, dim);
}

::std::vector<Series> unsafe_split_with_sizes_symint(const Series& self, SymIntArrayRef split_sizes, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("unsafe_split_with_sizes_symint", self, split_sizes, dim);
}

Series unsqueeze(const Series& self, std::int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("unsqueeze", self, dim);
}

Series & unsqueeze_(Series& self, int64_t dim) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("unsqueeze_", self, dim);
}

Series values(const Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("values", self);
}

Series var(const Series& self, DimnameList dim, bool unbiased, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("var", self, dim, unbiased, keepdim);
}

Series var(const Series& self, DimnameList dim, const optional<Scalar> & correction, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("var", self, dim, correction, keepdim);
}

Series var(const Series& self, OptionalIntArrayRef dim, bool unbiased, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("var", self, dim, unbiased, keepdim);
}

Series var(const Series& self, OptionalIntArrayRef dim, const optional<Scalar> & correction, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("var", self, dim, correction, keepdim);
}

Series var(const Series& self, bool unbiased) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("var", self, unbiased);
}

Series var(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("var", self, dim, keepdim);
}

Series vdot(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("vdot", self, other);
}

Series vdot(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("vdot", self, dim, keepdim);
}

Series view(const Series& self, IntArrayRef size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("view", self, size);
}

Series view(const Series& self, ScalarType dtype) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("view", self, dtype);
}

Series view_as(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("view_as", self, other);
}

Series view_symint(const Series& self, SymIntArrayRef size) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("view_symint", self, size);
}

::std::vector<Series> vsplit(const Series& self, IntArrayRef indices) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("vsplit", self, indices);
}

::std::vector<Series> vsplit(const Series& self, int64_t sections) {
  return ::pluss::codem::native::cpu::unimplemented<::std::vector<Series>>("vsplit", self, sections);
}

Series where(const Series& self, const Series & condition, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("where", self, condition, other);
}

Series where(const Series& self, const Series & condition, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("where", self, condition, other);
}

Series where(const Series& self, const Series& a, const Series& b) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("where", self, a, b);
}

Series where(const Series& self, std::int64_t dim, bool keepdim) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("where", self, dim, keepdim);
}

Series& where_(Series& self, const Series& a, const Series& b) {
  return ::pluss::codem::native::cpu::unimplemented<Series&>("where_", self, a, b);
}

Series xlogy(const Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("xlogy", self, other);
}

Series xlogy(const Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series>("xlogy", self, other);
}

Series & xlogy_(Series& self, const Scalar & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("xlogy_", self, other);
}

Series & xlogy_(Series& self, const Series & other) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("xlogy_", self, other);
}

Series & zero_(Series& self) {
  return ::pluss::codem::native::cpu::unimplemented<Series &>("zero_", self);
}

} // namespace pluss::codem::ops_internal
