#pragma once

#include "pluss/msharp/msharp.h"
#include "CodeM/CodeM.h"
#include "CodeM/core/Printing.h"
#include "pluss/error.h"
#include "pluss/slicing.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

// Optional: LibTorch interop (enable PLUSS_ENABLE_TORCH)
#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>
#endif

namespace pluss::codem {

template <class T>
concept SeriesScalar =
    std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>;

inline Series make_scalar_like(const Series& like, double value) {
  return Series::full({}, value, like.scalar_type(), like.platform());
}

enum class TernaryCompareOp : std::uint8_t { Eq, Ne, Lt, Le, Gt, Ge };
enum class CudaTransferPolicy : std::uint8_t { Strict = 0, AllowCpuBridge = 1 };

using BlobDeleter = std::function<void(void*)>;

CODEM_API bool mps_backend_available() noexcept;
CODEM_API bool cuda_backend_available() noexcept;
CODEM_API CudaTransferPolicy cuda_transfer_policy() noexcept;
CODEM_API bool cuda_cpu_bridge_enabled() noexcept;
CODEM_API const char* cuda_transfer_policy_name(CudaTransferPolicy policy) noexcept;
CODEM_API std::int32_t cuda_device_count() noexcept;
CODEM_API std::int32_t cuda_runtime_version() noexcept;
CODEM_API std::int32_t cuda_driver_version() noexcept;
CODEM_API const char* cuda_primary_device_name() noexcept;
CODEM_API std::int32_t cuda_primary_device_cc_major() noexcept;
CODEM_API std::int32_t cuda_primary_device_cc_minor() noexcept;
CODEM_API std::size_t cuda_primary_device_global_memory_bytes() noexcept;
CODEM_API bool cuda_cublas_provider_available() noexcept;
CODEM_API const char* cuda_cublas_provider_error_cstr() noexcept;
CODEM_API bool cuda_cusolver_provider_available() noexcept;
CODEM_API const char* cuda_cusolver_provider_error_cstr() noexcept;
CODEM_API bool cuda_cudnn_provider_available() noexcept;
CODEM_API const char* cuda_cudnn_provider_error_cstr() noexcept;
CODEM_API bool cuda_native_baseline_enabled() noexcept;
CODEM_API bool cuda_supports_elementwise_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;
CODEM_API bool cuda_native_elementwise_enabled() noexcept;
CODEM_API bool cuda_native_elementwise_strict() noexcept;
CODEM_API bool cuda_supports_unary_elementwise_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_binary_elementwise_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;
CODEM_API bool cuda_supports_unary_elementwise(const Series& a) noexcept;
CODEM_API bool cuda_supports_binary_elementwise(const Series& a, const Series& b) noexcept;
CODEM_API bool cuda_supports_comparison_elementwise(const Series& a, const Series& b) noexcept;
CODEM_API bool cuda_native_reduction_enabled() noexcept;
CODEM_API bool cuda_native_reduction_strict() noexcept;
CODEM_API bool cuda_supports_sum_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_prod_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_mean_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_ordered_extrema_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_logical_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_arg_extrema_reduction_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_sum_reduction(const Series& a) noexcept;
CODEM_API bool cuda_supports_sum_reduction(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_prod_reduction(const Series& a) noexcept;
CODEM_API bool cuda_supports_prod_reduction(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_mean_reduction(const Series& a) noexcept;
CODEM_API bool cuda_supports_mean_reduction(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_ordered_extrema_reduction(const Series& a) noexcept;
CODEM_API bool cuda_supports_ordered_extrema_reduction(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_logical_reduction(const Series& a) noexcept;
CODEM_API bool cuda_supports_logical_reduction(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_arg_extrema_reduction(const Series& a) noexcept;
CODEM_API bool cuda_supports_arg_extrema_reduction(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_native_scan_enabled() noexcept;
CODEM_API bool cuda_native_scan_strict() noexcept;
CODEM_API bool cuda_supports_cumsum_scan_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_cumprod_scan_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_cumsum_scan(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_cumprod_scan(const Series& a, std::int64_t dim) noexcept;
CODEM_API bool cuda_native_masking_enabled() noexcept;
CODEM_API bool cuda_native_masking_strict() noexcept;
CODEM_API bool cuda_supports_masked_select_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept;
CODEM_API bool cuda_supports_masked_fill_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept;
CODEM_API bool cuda_supports_masked_scatter_baseline(
    ScalarType value_dtype,
    ScalarType mask_dtype,
    ScalarType src_dtype) noexcept;
CODEM_API bool cuda_supports_masked_select(const Series& value, const Series& mask) noexcept;
CODEM_API bool cuda_supports_masked_fill(const Series& value, const Series& mask, const Series& fill_value) noexcept;
CODEM_API bool cuda_supports_masked_scatter(const Series& value, const Series& mask, const Series& src) noexcept;
CODEM_API bool cuda_native_bitwise_enabled() noexcept;
CODEM_API bool cuda_native_bitwise_strict() noexcept;
CODEM_API bool cuda_supports_bitwise_unary_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_bitwise_binary_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;
CODEM_API bool cuda_supports_bitwise_unary(const Series& a) noexcept;
CODEM_API bool cuda_supports_bitwise_binary(const Series& a, const Series& b) noexcept;
CODEM_API bool cuda_native_compose_enabled() noexcept;
CODEM_API bool cuda_native_compose_strict() noexcept;
CODEM_API bool cuda_supports_compose_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_unite_compose(const Series& self, pluss::msharp::RangeView::Of<Series> others, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_pile_compose(const Series& self, pluss::msharp::RangeView::Of<Series> others, std::int64_t dim) noexcept;
CODEM_API bool cuda_native_counting_enabled() noexcept;
CODEM_API bool cuda_native_counting_strict() noexcept;
CODEM_API bool cuda_supports_unique_counting_baseline(ScalarType dtype, bool sorted) noexcept;
CODEM_API bool cuda_supports_bincount_counting_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_histc_counting_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_unique_counting(const Series& a, bool sorted) noexcept;
CODEM_API bool cuda_supports_bincount_counting(const Series& a, std::int64_t minlength) noexcept;
CODEM_API bool cuda_supports_histc_counting(const Series& a, std::int64_t bins) noexcept;
CODEM_API bool cuda_native_factory_enabled() noexcept;
CODEM_API bool cuda_native_factory_strict() noexcept;
CODEM_API bool cuda_supports_zeros_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_ones_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_full_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_arange_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_linspace_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_eye_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_rand_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_randn_factory_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_native_stochastic_enabled() noexcept;
CODEM_API bool cuda_native_stochastic_strict() noexcept;
CODEM_API bool cuda_supports_bernoulli_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_normal_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_log_normal_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_uniform_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_exponential_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_geometric_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_cauchy_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_random_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_multinomial_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_poisson_stochastic_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_bernoulli_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_normal_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_log_normal_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_uniform_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_exponential_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_geometric_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_cauchy_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_random_stochastic(const Series& a) noexcept;
CODEM_API bool cuda_supports_multinomial_stochastic(const Series& a, std::int64_t num_samples, bool replacement) noexcept;
CODEM_API bool cuda_supports_poisson_stochastic(ScalarType dtype) noexcept;
CODEM_API bool cuda_native_selection_enabled() noexcept;
CODEM_API bool cuda_native_selection_strict() noexcept;
CODEM_API bool cuda_supports_sort_selection_baseline(ScalarType dtype, bool stable) noexcept;
CODEM_API bool cuda_supports_topk_selection_baseline(ScalarType dtype, bool sorted) noexcept;
CODEM_API bool cuda_supports_kthvalue_selection_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_argsort_selection_baseline(ScalarType dtype, bool stable) noexcept;
CODEM_API bool cuda_supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept;
CODEM_API bool cuda_supports_topk_selection(const Series& a, std::int64_t k, std::int64_t dim, bool sorted) noexcept;
CODEM_API bool cuda_supports_kthvalue_selection(const Series& a, std::int64_t k, std::int64_t dim) noexcept;
CODEM_API bool cuda_supports_argsort_selection(const Series& a, std::int64_t dim, bool stable) noexcept;
CODEM_API bool cuda_native_linalg_enabled() noexcept;
CODEM_API bool cuda_native_linalg_strict() noexcept;
CODEM_API bool cuda_supports_det_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_inverse_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_cholesky_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_logdet_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_slogdet_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_pinverse_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_triangular_solve_linalg_baseline(ScalarType a_dtype, ScalarType b_dtype) noexcept;
CODEM_API bool cuda_supports_cholesky_solve_linalg_baseline(ScalarType factor_dtype, ScalarType b_dtype) noexcept;
CODEM_API bool cuda_supports_cholesky_inverse_linalg_baseline(ScalarType factor_dtype) noexcept;
CODEM_API bool cuda_supports_lu_solve_linalg_baseline(ScalarType lu_dtype, ScalarType b_dtype) noexcept;
CODEM_API bool cuda_supports_qr_linalg_baseline(ScalarType dtype, bool reduced) noexcept;
CODEM_API bool cuda_supports_svd_linalg_baseline(ScalarType dtype, bool full_matrices) noexcept;
CODEM_API bool cuda_supports_matrix_power_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_matrix_exp_linalg_baseline(ScalarType dtype) noexcept;
CODEM_API bool cuda_supports_det_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_inverse_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_cholesky_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_logdet_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_slogdet_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_pinverse_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_triangular_solve_linalg(const Series& a, const Series& b) noexcept;
CODEM_API bool cuda_supports_cholesky_solve_linalg(const Series& factor, const Series& b) noexcept;
CODEM_API bool cuda_supports_cholesky_inverse_linalg(const Series& factor) noexcept;
CODEM_API bool cuda_supports_lu_solve_linalg(const Series& lu, const Series& b) noexcept;
CODEM_API bool cuda_supports_qr_linalg(const Series& a, bool reduced) noexcept;
CODEM_API bool cuda_supports_svd_linalg(const Series& a, bool full_matrices) noexcept;
CODEM_API bool cuda_supports_matrix_power_linalg(const Series& a) noexcept;
CODEM_API bool cuda_supports_matrix_exp_linalg(const Series& a) noexcept;
CODEM_API bool cuda_native_indexing_enabled() noexcept;
CODEM_API bool cuda_native_indexing_strict() noexcept;
CODEM_API bool cuda_supports_take_put_indexing_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept;
CODEM_API bool cuda_supports_nonzero_indexing_baseline(ScalarType value_dtype) noexcept;
CODEM_API bool cuda_supports_index_indexing_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept;
CODEM_API bool cuda_supports_take_indexing(const Series& self, const Series& index) noexcept;
CODEM_API bool cuda_supports_put_indexing(const Series& self, const Series& index, const Series& source, bool accumulate) noexcept;
CODEM_API bool cuda_supports_nonzero_indexing(const Series& self) noexcept;
CODEM_API bool cuda_supports_index_indexing(const Series& self, pluss::msharp::RangeView::Of<Series> indices) noexcept;
CODEM_API bool cuda_native_matmul_enabled() noexcept;
CODEM_API bool cuda_native_matmul_strict() noexcept;
CODEM_API bool cuda_supports_matmul_native_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;
CODEM_API bool cuda_supports_matmul_native(const Series& a, const Series& b) noexcept;
CODEM_API bool cuda_native_scatter_gather_enabled() noexcept;
CODEM_API bool cuda_native_scatter_gather_strict() noexcept;
CODEM_API bool cuda_supports_scatter_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept;
CODEM_API bool cuda_supports_scatter_add_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept;
CODEM_API bool cuda_supports_scatter_reduce_baseline(
    ScalarType self_dtype,
    ScalarType index_dtype,
    ScalarType src_dtype,
    std::string_view reduce) noexcept;
CODEM_API bool cuda_supports_gather_baseline(ScalarType input_dtype, ScalarType index_dtype) noexcept;
CODEM_API bool cuda_supports_scatter_native(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept;
CODEM_API bool cuda_supports_scatter_add_native(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src) noexcept;
CODEM_API bool cuda_supports_scatter_reduce_native(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src,
    std::string_view reduce) noexcept;
CODEM_API bool cuda_supports_gather_native(const Series& input, std::int64_t dim, const Series& index) noexcept;
Series from_buffer(void* data,
                   std::span<const std::int64_t> sizes,
                   std::span<const std::int64_t> strides,
                   BlobDeleter deleter,
                   ScalarType dtype);

struct TernaryCompareBuilder {
  const Series& first;
};

struct TernaryCompareChain {
  const Series& first;
  const Series& second;
  TernaryCompareOp first_op;
};

inline TernaryCompareBuilder Series::tri() const noexcept { return TernaryCompareBuilder{*this}; }

inline Series ternary_compare_eval(const Series& a, const Series& b, TernaryCompareOp op) {
  switch (op) {
    case TernaryCompareOp::Eq: return a.eq(b);
    case TernaryCompareOp::Ne: return a.ne(b);
    case TernaryCompareOp::Lt: return a.lt(b);
    case TernaryCompareOp::Le: return a.le(b);
    case TernaryCompareOp::Gt: return a.gt(b);
    case TernaryCompareOp::Ge: return a.ge(b);
    default: return a.eq(b);
  }
}

inline TernaryCompareChain operator==(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Eq};
}
inline TernaryCompareChain operator!=(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Ne};
}
inline TernaryCompareChain operator<(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Lt};
}
inline TernaryCompareChain operator<=(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Le};
}
inline TernaryCompareChain operator>(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Gt};
}
inline TernaryCompareChain operator>=(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Ge};
}

inline Series ternary_compare_finish(const TernaryCompareChain& chain,
                                     const Series& third,
                                     TernaryCompareOp second_op) {
  const Series first_mask = ternary_compare_eval(chain.first, chain.second, chain.first_op);
  const Series second_mask = ternary_compare_eval(chain.second, third, second_op);
  return first_mask.logical_and(second_mask);
}

inline Series operator==(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Eq);
}
inline Series operator!=(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Ne);
}
inline Series operator<(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Lt);
}
inline Series operator<=(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Le);
}
inline Series operator>(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Gt);
}
inline Series operator>=(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Ge);
}

template <SeriesScalar T>
inline Series operator+(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).add(rhs);
}

template <SeriesScalar T>
inline Series operator-(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).sub(rhs);
}

template <SeriesScalar T>
inline Series operator*(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).mul(rhs);
}

template <SeriesScalar T>
inline Series operator/(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).div(rhs);
}

template <SeriesScalar T>
inline Series operator==(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).eq(rhs);
}

template <SeriesScalar T>
inline Series operator!=(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).ne(rhs);
}

template <SeriesScalar T>
inline Series operator<(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).lt(rhs);
}

template <SeriesScalar T>
inline Series operator<=(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).le(rhs);
}

template <SeriesScalar T>
inline Series operator>(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).gt(rhs);
}

template <SeriesScalar T>
inline Series operator>=(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).ge(rhs);
}

inline bool platform_available(Platform p) noexcept {
  switch (p) {
    case Platform::CPU:
      return true;
    case Platform::CUDA:
      return cuda_backend_available();
    case Platform::MPS:
      return mps_backend_available();
    default:
      return false;
  }
}

} // namespace pluss::codem

namespace pluss {
  using f32_t = float;
  using f64_t = double;
  using i8_t = std::int8_t;
  using i16_t = std::int16_t;
  using i32_t = std::int32_t;
  using i64_t = std::int64_t;
  using u8_t = std::uint8_t;
  using u16_t = std::uint16_t;
  using u32_t = std::uint32_t;
  using u64_t = std::uint64_t;

  using Series = codem::Series;
  using Config = codem::Config;
  using ScalarType = codem::ScalarType;
  using Platform = codem::Platform;
  template <class T>
  using RangeView = msharp::RangeView::Of<T>;
  inline constexpr Platform pCPU = Platform::CPU;
  inline constexpr Platform pCUDA = Platform::CUDA;
  inline constexpr Platform pMPS = Platform::MPS;
  using BlobDeleter = codem::BlobDeleter;
  using PrintOptions = codem::PrintOptions;

  using codem::to_string;
  using codem::platform_available;
  using codem::operator<<;

inline Series index_put(const Series& self,
                          pluss::msharp::RangeView::Of<Series> indices,
                          const Series& values,
                          bool accumulate=false) {
    return self.index_put(indices, values, accumulate);
  }

} // namespace pluss

namespace pluss::mps {

inline bool is_ready() noexcept {
  return codem::platform_available(codem::Platform::MPS);
}

inline bool is_usable() noexcept {
  return is_ready();
}

} // namespace pluss::mps

namespace pluss::cuda {

using TransferPolicy = codem::CudaTransferPolicy;

inline bool is_ready() noexcept {
  return codem::platform_available(codem::Platform::CUDA);
}

inline bool is_usable() noexcept {
  return is_ready();
}

inline TransferPolicy transfer_policy() noexcept {
  return codem::cuda_transfer_policy();
}

inline bool cpu_bridge_enabled() noexcept {
  return codem::cuda_cpu_bridge_enabled();
}

inline const char* transfer_policy_name(TransferPolicy policy) noexcept {
  return codem::cuda_transfer_policy_name(policy);
}

inline std::int32_t device_count() noexcept {
  return codem::cuda_device_count();
}

inline std::int32_t runtime_version() noexcept {
  return codem::cuda_runtime_version();
}

inline std::int32_t driver_version() noexcept {
  return codem::cuda_driver_version();
}

inline const char* primary_device_name() noexcept {
  return codem::cuda_primary_device_name();
}

inline std::int32_t primary_compute_capability_major() noexcept {
  return codem::cuda_primary_device_cc_major();
}

inline std::int32_t primary_compute_capability_minor() noexcept {
  return codem::cuda_primary_device_cc_minor();
}

inline std::size_t primary_global_memory_bytes() noexcept {
  return codem::cuda_primary_device_global_memory_bytes();
}

inline bool cublas_available() noexcept {
  return codem::cuda_cublas_provider_available();
}

inline const char* cublas_error() noexcept {
  return codem::cuda_cublas_provider_error_cstr();
}

inline bool cusolver_available() noexcept {
  return codem::cuda_cusolver_provider_available();
}

inline const char* cusolver_error() noexcept {
  return codem::cuda_cusolver_provider_error_cstr();
}

inline bool cudnn_ready() noexcept {
  return codem::cuda_cudnn_provider_available();
}

inline const char* cudnn_error() noexcept {
  return codem::cuda_cudnn_provider_error_cstr();
}

inline bool native_baseline_enabled() noexcept {
  return codem::cuda_native_baseline_enabled();
}

inline bool supports_elementwise_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_elementwise_baseline(dtype);
}

inline bool supports_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_reduction_baseline(dtype);
}

inline bool supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return codem::cuda_supports_matmul_baseline(lhs_dtype, rhs_dtype);
}

inline bool native_elementwise_enabled() noexcept {
  return codem::cuda_native_elementwise_enabled();
}

inline bool native_elementwise_strict() noexcept {
  return codem::cuda_native_elementwise_strict();
}

inline bool supports_unary_elementwise_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_unary_elementwise_baseline(dtype);
}

inline bool supports_binary_elementwise_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return codem::cuda_supports_binary_elementwise_baseline(lhs_dtype, rhs_dtype);
}

inline bool supports_unary_elementwise(const Series& a) noexcept {
  return codem::cuda_supports_unary_elementwise(a);
}

inline bool supports_binary_elementwise(const Series& a, const Series& b) noexcept {
  return codem::cuda_supports_binary_elementwise(a, b);
}

inline bool supports_comparison_elementwise(const Series& a, const Series& b) noexcept {
  return codem::cuda_supports_comparison_elementwise(a, b);
}

inline bool native_reduction_enabled() noexcept {
  return codem::cuda_native_reduction_enabled();
}

inline bool native_reduction_strict() noexcept {
  return codem::cuda_native_reduction_strict();
}

inline bool supports_sum_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_sum_reduction_baseline(dtype);
}

inline bool supports_prod_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_prod_reduction_baseline(dtype);
}

inline bool supports_mean_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_mean_reduction_baseline(dtype);
}

inline bool supports_ordered_extrema_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_ordered_extrema_reduction_baseline(dtype);
}

inline bool supports_logical_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_logical_reduction_baseline(dtype);
}

inline bool supports_arg_extrema_reduction_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_arg_extrema_reduction_baseline(dtype);
}

inline bool supports_sum_reduction(const Series& a) noexcept {
  return codem::cuda_supports_sum_reduction(a);
}

inline bool supports_sum_reduction(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_sum_reduction(a, dim);
}

inline bool supports_prod_reduction(const Series& a) noexcept {
  return codem::cuda_supports_prod_reduction(a);
}

inline bool supports_prod_reduction(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_prod_reduction(a, dim);
}

inline bool supports_mean_reduction(const Series& a) noexcept {
  return codem::cuda_supports_mean_reduction(a);
}

inline bool supports_mean_reduction(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_mean_reduction(a, dim);
}

inline bool supports_ordered_extrema_reduction(const Series& a) noexcept {
  return codem::cuda_supports_ordered_extrema_reduction(a);
}

inline bool supports_ordered_extrema_reduction(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_ordered_extrema_reduction(a, dim);
}

inline bool supports_logical_reduction(const Series& a) noexcept {
  return codem::cuda_supports_logical_reduction(a);
}

inline bool supports_logical_reduction(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_logical_reduction(a, dim);
}

inline bool supports_arg_extrema_reduction(const Series& a) noexcept {
  return codem::cuda_supports_arg_extrema_reduction(a);
}

inline bool supports_arg_extrema_reduction(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_arg_extrema_reduction(a, dim);
}

inline bool native_scan_enabled() noexcept {
  return codem::cuda_native_scan_enabled();
}

inline bool native_scan_strict() noexcept {
  return codem::cuda_native_scan_strict();
}

inline bool supports_cumsum_scan_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_cumsum_scan_baseline(dtype);
}

inline bool supports_cumprod_scan_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_cumprod_scan_baseline(dtype);
}

inline bool supports_cumsum_scan(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_cumsum_scan(a, dim);
}

inline bool supports_cumprod_scan(const Series& a, std::int64_t dim) noexcept {
  return codem::cuda_supports_cumprod_scan(a, dim);
}

inline bool native_masking_enabled() noexcept {
  return codem::cuda_native_masking_enabled();
}

inline bool native_masking_strict() noexcept {
  return codem::cuda_native_masking_strict();
}

inline bool supports_masked_select_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept {
  return codem::cuda_supports_masked_select_baseline(value_dtype, mask_dtype);
}

inline bool supports_masked_fill_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept {
  return codem::cuda_supports_masked_fill_baseline(value_dtype, mask_dtype);
}

inline bool supports_masked_scatter_baseline(
    ScalarType value_dtype,
    ScalarType mask_dtype,
    ScalarType src_dtype) noexcept {
  return codem::cuda_supports_masked_scatter_baseline(value_dtype, mask_dtype, src_dtype);
}

inline bool supports_masked_select(const Series& value, const Series& mask) noexcept {
  return codem::cuda_supports_masked_select(value, mask);
}

inline bool supports_masked_fill(const Series& value, const Series& mask, const Series& fill_value) noexcept {
  return codem::cuda_supports_masked_fill(value, mask, fill_value);
}

inline bool supports_masked_scatter(const Series& value, const Series& mask, const Series& src) noexcept {
  return codem::cuda_supports_masked_scatter(value, mask, src);
}

inline bool native_bitwise_enabled() noexcept {
  return codem::cuda_native_bitwise_enabled();
}

inline bool native_bitwise_strict() noexcept {
  return codem::cuda_native_bitwise_strict();
}

inline bool supports_bitwise_unary_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_bitwise_unary_baseline(dtype);
}

inline bool supports_bitwise_binary_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return codem::cuda_supports_bitwise_binary_baseline(lhs_dtype, rhs_dtype);
}

inline bool supports_bitwise_unary(const Series& a) noexcept {
  return codem::cuda_supports_bitwise_unary(a);
}

inline bool supports_bitwise_binary(const Series& a, const Series& b) noexcept {
  return codem::cuda_supports_bitwise_binary(a, b);
}

inline bool native_compose_enabled() noexcept {
  return codem::cuda_native_compose_enabled();
}

inline bool native_compose_strict() noexcept {
  return codem::cuda_native_compose_strict();
}

inline bool supports_compose_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_compose_baseline(dtype);
}

inline bool supports_unite_compose(const Series& self, pluss::msharp::RangeView::Of<Series> others, std::int64_t dim) noexcept {
  return codem::cuda_supports_unite_compose(self, others, dim);
}

inline bool supports_pile_compose(const Series& self, pluss::msharp::RangeView::Of<Series> others, std::int64_t dim) noexcept {
  return codem::cuda_supports_pile_compose(self, others, dim);
}

inline bool native_counting_enabled() noexcept {
  return codem::cuda_native_counting_enabled();
}

inline bool native_counting_strict() noexcept {
  return codem::cuda_native_counting_strict();
}

inline bool supports_unique_counting_baseline(ScalarType dtype, bool sorted) noexcept {
  return codem::cuda_supports_unique_counting_baseline(dtype, sorted);
}

inline bool supports_bincount_counting_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_bincount_counting_baseline(dtype);
}

inline bool supports_histc_counting_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_histc_counting_baseline(dtype);
}

inline bool supports_unique_counting(const Series& a, bool sorted) noexcept {
  return codem::cuda_supports_unique_counting(a, sorted);
}

inline bool supports_bincount_counting(const Series& a, std::int64_t minlength) noexcept {
  return codem::cuda_supports_bincount_counting(a, minlength);
}

inline bool supports_histc_counting(const Series& a, std::int64_t bins) noexcept {
  return codem::cuda_supports_histc_counting(a, bins);
}

inline bool native_factory_enabled() noexcept {
  return codem::cuda_native_factory_enabled();
}

inline bool native_factory_strict() noexcept {
  return codem::cuda_native_factory_strict();
}

inline bool supports_zeros_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_zeros_factory_baseline(dtype);
}

inline bool supports_ones_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_ones_factory_baseline(dtype);
}

inline bool supports_full_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_full_factory_baseline(dtype);
}

inline bool supports_arange_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_arange_factory_baseline(dtype);
}

inline bool supports_linspace_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_linspace_factory_baseline(dtype);
}

inline bool supports_eye_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_eye_factory_baseline(dtype);
}

inline bool supports_rand_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_rand_factory_baseline(dtype);
}

inline bool supports_randn_factory_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_randn_factory_baseline(dtype);
}

inline bool native_stochastic_enabled() noexcept {
  return codem::cuda_native_stochastic_enabled();
}

inline bool native_stochastic_strict() noexcept {
  return codem::cuda_native_stochastic_strict();
}

inline bool supports_bernoulli_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_bernoulli_stochastic_baseline(dtype);
}

inline bool supports_normal_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_normal_stochastic_baseline(dtype);
}

inline bool supports_log_normal_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_log_normal_stochastic_baseline(dtype);
}

inline bool supports_uniform_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_uniform_stochastic_baseline(dtype);
}

inline bool supports_exponential_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_exponential_stochastic_baseline(dtype);
}

inline bool supports_geometric_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_geometric_stochastic_baseline(dtype);
}

inline bool supports_cauchy_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_cauchy_stochastic_baseline(dtype);
}

inline bool supports_random_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_random_stochastic_baseline(dtype);
}

inline bool supports_multinomial_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_multinomial_stochastic_baseline(dtype);
}

inline bool supports_poisson_stochastic_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_poisson_stochastic_baseline(dtype);
}

inline bool supports_bernoulli_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_bernoulli_stochastic(a);
}

inline bool supports_normal_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_normal_stochastic(a);
}

inline bool supports_log_normal_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_log_normal_stochastic(a);
}

inline bool supports_uniform_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_uniform_stochastic(a);
}

inline bool supports_exponential_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_exponential_stochastic(a);
}

inline bool supports_geometric_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_geometric_stochastic(a);
}

inline bool supports_cauchy_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_cauchy_stochastic(a);
}

inline bool supports_random_stochastic(const Series& a) noexcept {
  return codem::cuda_supports_random_stochastic(a);
}

inline bool supports_multinomial_stochastic(const Series& a, std::int64_t num_samples, bool replacement) noexcept {
  return codem::cuda_supports_multinomial_stochastic(a, num_samples, replacement);
}

inline bool supports_poisson_stochastic(ScalarType dtype) noexcept {
  return codem::cuda_supports_poisson_stochastic(dtype);
}

inline bool native_selection_enabled() noexcept {
  return codem::cuda_native_selection_enabled();
}

inline bool native_selection_strict() noexcept {
  return codem::cuda_native_selection_strict();
}

inline bool supports_sort_selection_baseline(ScalarType dtype, bool stable) noexcept {
  return codem::cuda_supports_sort_selection_baseline(dtype, stable);
}

inline bool supports_topk_selection_baseline(ScalarType dtype, bool sorted) noexcept {
  return codem::cuda_supports_topk_selection_baseline(dtype, sorted);
}

inline bool supports_kthvalue_selection_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_kthvalue_selection_baseline(dtype);
}

inline bool supports_argsort_selection_baseline(ScalarType dtype, bool stable) noexcept {
  return codem::cuda_supports_argsort_selection_baseline(dtype, stable);
}

inline bool supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  return codem::cuda_supports_sort_selection(a, dim, stable);
}

inline bool supports_topk_selection(const Series& a, std::int64_t k, std::int64_t dim, bool sorted) noexcept {
  return codem::cuda_supports_topk_selection(a, k, dim, sorted);
}

inline bool supports_kthvalue_selection(const Series& a, std::int64_t k, std::int64_t dim) noexcept {
  return codem::cuda_supports_kthvalue_selection(a, k, dim);
}

inline bool supports_argsort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  return codem::cuda_supports_argsort_selection(a, dim, stable);
}

inline bool native_linalg_enabled() noexcept {
  return codem::cuda_native_linalg_enabled();
}

inline bool native_linalg_strict() noexcept {
  return codem::cuda_native_linalg_strict();
}

inline bool supports_det_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_det_linalg_baseline(dtype);
}

inline bool supports_inverse_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_inverse_linalg_baseline(dtype);
}

inline bool supports_cholesky_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_cholesky_linalg_baseline(dtype);
}

inline bool supports_logdet_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_logdet_linalg_baseline(dtype);
}

inline bool supports_slogdet_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_slogdet_linalg_baseline(dtype);
}

inline bool supports_pinverse_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_pinverse_linalg_baseline(dtype);
}

inline bool supports_triangular_solve_linalg_baseline(ScalarType a_dtype, ScalarType b_dtype) noexcept {
  return codem::cuda_supports_triangular_solve_linalg_baseline(a_dtype, b_dtype);
}

inline bool supports_cholesky_solve_linalg_baseline(ScalarType factor_dtype, ScalarType b_dtype) noexcept {
  return codem::cuda_supports_cholesky_solve_linalg_baseline(factor_dtype, b_dtype);
}

inline bool supports_cholesky_inverse_linalg_baseline(ScalarType factor_dtype) noexcept {
  return codem::cuda_supports_cholesky_inverse_linalg_baseline(factor_dtype);
}

inline bool supports_lu_solve_linalg_baseline(ScalarType lu_dtype, ScalarType b_dtype) noexcept {
  return codem::cuda_supports_lu_solve_linalg_baseline(lu_dtype, b_dtype);
}

inline bool supports_qr_linalg_baseline(ScalarType dtype, bool reduced) noexcept {
  return codem::cuda_supports_qr_linalg_baseline(dtype, reduced);
}

inline bool supports_svd_linalg_baseline(ScalarType dtype, bool full_matrices) noexcept {
  return codem::cuda_supports_svd_linalg_baseline(dtype, full_matrices);
}

inline bool supports_matrix_power_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_matrix_power_linalg_baseline(dtype);
}

inline bool supports_matrix_exp_linalg_baseline(ScalarType dtype) noexcept {
  return codem::cuda_supports_matrix_exp_linalg_baseline(dtype);
}

inline bool supports_det_linalg(const Series& a) noexcept {
  return codem::cuda_supports_det_linalg(a);
}

inline bool supports_inverse_linalg(const Series& a) noexcept {
  return codem::cuda_supports_inverse_linalg(a);
}

inline bool supports_cholesky_linalg(const Series& a) noexcept {
  return codem::cuda_supports_cholesky_linalg(a);
}

inline bool supports_logdet_linalg(const Series& a) noexcept {
  return codem::cuda_supports_logdet_linalg(a);
}

inline bool supports_slogdet_linalg(const Series& a) noexcept {
  return codem::cuda_supports_slogdet_linalg(a);
}

inline bool supports_pinverse_linalg(const Series& a) noexcept {
  return codem::cuda_supports_pinverse_linalg(a);
}

inline bool supports_triangular_solve_linalg(const Series& a, const Series& b) noexcept {
  return codem::cuda_supports_triangular_solve_linalg(a, b);
}

inline bool supports_cholesky_solve_linalg(const Series& factor, const Series& b) noexcept {
  return codem::cuda_supports_cholesky_solve_linalg(factor, b);
}

inline bool supports_cholesky_inverse_linalg(const Series& factor) noexcept {
  return codem::cuda_supports_cholesky_inverse_linalg(factor);
}

inline bool supports_lu_solve_linalg(const Series& lu, const Series& b) noexcept {
  return codem::cuda_supports_lu_solve_linalg(lu, b);
}

inline bool supports_qr_linalg(const Series& a, bool reduced) noexcept {
  return codem::cuda_supports_qr_linalg(a, reduced);
}

inline bool supports_svd_linalg(const Series& a, bool full_matrices) noexcept {
  return codem::cuda_supports_svd_linalg(a, full_matrices);
}

inline bool supports_matrix_power_linalg(const Series& a) noexcept {
  return codem::cuda_supports_matrix_power_linalg(a);
}

inline bool supports_matrix_exp_linalg(const Series& a) noexcept {
  return codem::cuda_supports_matrix_exp_linalg(a);
}

inline bool native_indexing_enabled() noexcept {
  return codem::cuda_native_indexing_enabled();
}

inline bool native_indexing_strict() noexcept {
  return codem::cuda_native_indexing_strict();
}

inline bool supports_take_put_indexing_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept {
  return codem::cuda_supports_take_put_indexing_baseline(value_dtype, index_dtype);
}

inline bool supports_nonzero_indexing_baseline(ScalarType value_dtype) noexcept {
  return codem::cuda_supports_nonzero_indexing_baseline(value_dtype);
}

inline bool supports_index_indexing_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept {
  return codem::cuda_supports_index_indexing_baseline(value_dtype, index_dtype);
}

inline bool supports_take_indexing(const Series& self, const Series& index) noexcept {
  return codem::cuda_supports_take_indexing(self, index);
}

inline bool supports_put_indexing(const Series& self, const Series& index, const Series& source, bool accumulate) noexcept {
  return codem::cuda_supports_put_indexing(self, index, source, accumulate);
}

inline bool supports_nonzero_indexing(const Series& self) noexcept {
  return codem::cuda_supports_nonzero_indexing(self);
}

inline bool supports_index_indexing(const Series& self, pluss::msharp::RangeView::Of<Series> indices) noexcept {
  return codem::cuda_supports_index_indexing(self, indices);
}

inline bool native_matmul_enabled() noexcept {
  return codem::cuda_native_matmul_enabled();
}

inline bool native_matmul_strict() noexcept {
  return codem::cuda_native_matmul_strict();
}

inline bool supports_matmul_native_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return codem::cuda_supports_matmul_native_baseline(lhs_dtype, rhs_dtype);
}

inline bool supports_matmul_native(const Series& a, const Series& b) noexcept {
  return codem::cuda_supports_matmul_native(a, b);
}

inline bool native_scatter_gather_enabled() noexcept {
  return codem::cuda_native_scatter_gather_enabled();
}

inline bool native_scatter_gather_strict() noexcept {
  return codem::cuda_native_scatter_gather_strict();
}

inline bool supports_scatter_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept {
  return codem::cuda_supports_scatter_baseline(self_dtype, index_dtype, src_dtype);
}

inline bool supports_scatter_add_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept {
  return codem::cuda_supports_scatter_add_baseline(self_dtype, index_dtype, src_dtype);
}

inline bool supports_scatter_reduce_baseline(
    ScalarType self_dtype,
    ScalarType index_dtype,
    ScalarType src_dtype,
    std::string_view reduce) noexcept {
  return codem::cuda_supports_scatter_reduce_baseline(self_dtype, index_dtype, src_dtype, reduce);
}

inline bool supports_gather_baseline(ScalarType input_dtype, ScalarType index_dtype) noexcept {
  return codem::cuda_supports_gather_baseline(input_dtype, index_dtype);
}

inline bool supports_scatter_native(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept {
  return codem::cuda_supports_scatter_native(self, dim, index, src);
}

inline bool supports_scatter_add_native(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src) noexcept {
  return codem::cuda_supports_scatter_add_native(self, dim, index, src);
}

inline bool supports_scatter_reduce_native(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src,
    std::string_view reduce) noexcept {
  return codem::cuda_supports_scatter_reduce_native(self, dim, index, src, reduce);
}

inline bool supports_gather_native(const Series& input, std::int64_t dim, const Series& index) noexcept {
  return codem::cuda_supports_gather_native(input, dim, index);
}

} // namespace pluss::cuda
