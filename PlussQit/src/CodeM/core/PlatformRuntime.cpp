#include "pluss/codem/codem.h"
#include "CodeM/native/mps/MPSRuntime.h"
#include "CodeM/native/cuda/CUDARuntime.h"
#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/ElementwiseOps.h"
#include "CodeM/native/cuda/ReductionOps.h"
#include "CodeM/native/cuda/ScanOps.h"
#include "CodeM/native/cuda/MaskingOps.h"
#include "CodeM/native/cuda/BitwiseOps.h"
#include "CodeM/native/cuda/ComposeOps.h"
#include "CodeM/native/cuda/CountingOps.h"
#include "CodeM/native/cuda/FactoryOps.h"
#include "CodeM/native/cuda/StochasticOps.h"
#include "CodeM/native/cuda/SelectionOps.h"
#include "CodeM/native/cuda/LinalgOps.h"
#include "CodeM/native/cuda/IndexingOps.h"
#include "CodeM/native/cuda/MatmulOps.h"
#include "CodeM/native/cuda/ScatterGatherOps.h"

namespace pluss::codem {

bool mps_backend_available() noexcept {
#if defined(__APPLE__)
  return native::mps::runtime_available();
#else
  return false;
#endif
}

bool cuda_backend_available() noexcept {
  return native::cuda::runtime_available();
}

CudaTransferPolicy cuda_transfer_policy() noexcept {
  const auto native_policy = native::cuda::transfer_policy();
  if (native_policy == native::cuda::TransferPolicy::AllowCpuBridge) {
    return CudaTransferPolicy::AllowCpuBridge;
  }
  return CudaTransferPolicy::Strict;
}

bool cuda_cpu_bridge_enabled() noexcept {
  return cuda_transfer_policy() == CudaTransferPolicy::AllowCpuBridge;
}

const char* cuda_transfer_policy_name(CudaTransferPolicy policy) noexcept {
  switch (policy) {
    case CudaTransferPolicy::AllowCpuBridge:
      return "AllowCpuBridge";
    case CudaTransferPolicy::Strict:
    default:
      return "Strict";
  }
}

std::int32_t cuda_device_count() noexcept {
  return native::cuda::device_count();
}

std::int32_t cuda_runtime_version() noexcept {
  return native::cuda::runtime_version();
}

std::int32_t cuda_driver_version() noexcept {
  return native::cuda::driver_version();
}

const char* cuda_primary_device_name() noexcept {
  return native::cuda::primary_device_name();
}

std::int32_t cuda_primary_device_cc_major() noexcept {
  return native::cuda::primary_device_cc_major();
}

std::int32_t cuda_primary_device_cc_minor() noexcept {
  return native::cuda::primary_device_cc_minor();
}

std::size_t cuda_primary_device_global_memory_bytes() noexcept {
  return native::cuda::primary_device_global_memory_bytes();
}

bool cuda_cublas_provider_available() noexcept {
  return native::cuda::cublas_provider_available();
}

const char* cuda_cublas_provider_error_cstr() noexcept {
  return native::cuda::cublas_provider_error_cstr();
}

bool cuda_cusolver_provider_available() noexcept {
  return native::cuda::cusolver_provider_available();
}

const char* cuda_cusolver_provider_error_cstr() noexcept {
  return native::cuda::cusolver_provider_error_cstr();
}

bool cuda_cudnn_provider_available() noexcept {
  return native::cuda::cudnn_provider_available();
}

const char* cuda_cudnn_provider_error_cstr() noexcept {
  return native::cuda::cudnn_provider_error_cstr();
}

bool cuda_native_baseline_enabled() noexcept {
  return native::cuda::native_baseline_enabled();
}

bool cuda_supports_elementwise_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_elementwise_lane(dtype);
}

bool cuda_supports_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_reduction_lane(dtype);
}

bool cuda_supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return native::cuda::supports_matmul_lane(lhs_dtype, rhs_dtype);
}

bool cuda_native_elementwise_enabled() noexcept {
  return native::cuda::native_elementwise_enabled();
}

bool cuda_native_elementwise_strict() noexcept {
  return native::cuda::native_elementwise_strict();
}

bool cuda_supports_unary_elementwise_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_unary_elementwise_baseline(dtype);
}

bool cuda_supports_binary_elementwise_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return native::cuda::supports_binary_elementwise_baseline(lhs_dtype, rhs_dtype);
}

bool cuda_supports_unary_elementwise(const Series& a) noexcept {
  return native::cuda::supports_unary_elementwise(a);
}

bool cuda_supports_binary_elementwise(const Series& a, const Series& b) noexcept {
  return native::cuda::supports_binary_elementwise(a, b);
}

bool cuda_supports_comparison_elementwise(const Series& a, const Series& b) noexcept {
  return native::cuda::supports_comparison_elementwise(a, b);
}

bool cuda_native_reduction_enabled() noexcept {
  return native::cuda::native_reduction_enabled();
}

bool cuda_native_reduction_strict() noexcept {
  return native::cuda::native_reduction_strict();
}

bool cuda_supports_sum_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_sum_reduction_baseline(dtype);
}

bool cuda_supports_prod_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_prod_reduction_baseline(dtype);
}

bool cuda_supports_mean_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_mean_reduction_baseline(dtype);
}

bool cuda_supports_ordered_extrema_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_ordered_extrema_reduction_baseline(dtype);
}

bool cuda_supports_logical_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_logical_reduction_baseline(dtype);
}

bool cuda_supports_arg_extrema_reduction_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_arg_extrema_reduction_baseline(dtype);
}

bool cuda_supports_sum_reduction(const Series& a) noexcept {
  return native::cuda::supports_sum_reduction(a);
}

bool cuda_supports_sum_reduction(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_sum_reduction(a, dim);
}

bool cuda_supports_prod_reduction(const Series& a) noexcept {
  return native::cuda::supports_prod_reduction(a);
}

bool cuda_supports_prod_reduction(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_prod_reduction(a, dim);
}

bool cuda_supports_mean_reduction(const Series& a) noexcept {
  return native::cuda::supports_mean_reduction(a);
}

bool cuda_supports_mean_reduction(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_mean_reduction(a, dim);
}

bool cuda_supports_ordered_extrema_reduction(const Series& a) noexcept {
  return native::cuda::supports_ordered_extrema_reduction(a);
}

bool cuda_supports_ordered_extrema_reduction(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_ordered_extrema_reduction(a, dim);
}

bool cuda_supports_logical_reduction(const Series& a) noexcept {
  return native::cuda::supports_logical_reduction(a);
}

bool cuda_supports_logical_reduction(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_logical_reduction(a, dim);
}

bool cuda_supports_arg_extrema_reduction(const Series& a) noexcept {
  return native::cuda::supports_arg_extrema_reduction(a);
}

bool cuda_supports_arg_extrema_reduction(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_arg_extrema_reduction(a, dim);
}

bool cuda_native_scan_enabled() noexcept {
  return native::cuda::native_scan_enabled();
}

bool cuda_native_scan_strict() noexcept {
  return native::cuda::native_scan_strict();
}

bool cuda_supports_cumsum_scan_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_cumsum_scan_baseline(dtype);
}

bool cuda_supports_cumprod_scan_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_cumprod_scan_baseline(dtype);
}

bool cuda_supports_cumsum_scan(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_cumsum_scan(a, dim);
}

bool cuda_supports_cumprod_scan(const Series& a, std::int64_t dim) noexcept {
  return native::cuda::supports_cumprod_scan(a, dim);
}

bool cuda_native_masking_enabled() noexcept {
  return native::cuda::native_masking_enabled();
}

bool cuda_native_masking_strict() noexcept {
  return native::cuda::native_masking_strict();
}

bool cuda_supports_masked_select_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept {
  return native::cuda::supports_masked_select_baseline(value_dtype, mask_dtype);
}

bool cuda_supports_masked_fill_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept {
  return native::cuda::supports_masked_fill_baseline(value_dtype, mask_dtype);
}

bool cuda_supports_masked_scatter_baseline(
    ScalarType value_dtype,
    ScalarType mask_dtype,
    ScalarType src_dtype) noexcept {
  return native::cuda::supports_masked_scatter_baseline(value_dtype, mask_dtype, src_dtype);
}

bool cuda_supports_masked_select(const Series& value, const Series& mask) noexcept {
  return native::cuda::supports_masked_select(value, mask);
}

bool cuda_supports_masked_fill(const Series& value, const Series& mask, const Series& fill_value) noexcept {
  return native::cuda::supports_masked_fill(value, mask, fill_value);
}

bool cuda_supports_masked_scatter(const Series& value, const Series& mask, const Series& src) noexcept {
  return native::cuda::supports_masked_scatter(value, mask, src);
}

bool cuda_native_bitwise_enabled() noexcept {
  return native::cuda::native_bitwise_enabled();
}

bool cuda_native_bitwise_strict() noexcept {
  return native::cuda::native_bitwise_strict();
}

bool cuda_supports_bitwise_unary_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_bitwise_unary_baseline(dtype);
}

bool cuda_supports_bitwise_binary_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return native::cuda::supports_bitwise_binary_baseline(lhs_dtype, rhs_dtype);
}

bool cuda_supports_bitwise_unary(const Series& a) noexcept {
  return native::cuda::supports_bitwise_unary(a);
}

bool cuda_supports_bitwise_binary(const Series& a, const Series& b) noexcept {
  return native::cuda::supports_bitwise_binary(a, b);
}

bool cuda_native_compose_enabled() noexcept {
  return native::cuda::native_compose_enabled();
}

bool cuda_native_compose_strict() noexcept {
  return native::cuda::native_compose_strict();
}

bool cuda_supports_compose_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_compose_baseline(dtype);
}

bool cuda_supports_unite_compose(
    const Series& self,
    pluss::msharp::RangeView::Of<Series> others,
    std::int64_t dim) noexcept {
  return native::cuda::supports_unite_compose(self, others, dim);
}

bool cuda_supports_pile_compose(
    const Series& self,
    pluss::msharp::RangeView::Of<Series> others,
    std::int64_t dim) noexcept {
  return native::cuda::supports_pile_compose(self, others, dim);
}

bool cuda_native_counting_enabled() noexcept {
  return native::cuda::native_counting_enabled();
}

bool cuda_native_counting_strict() noexcept {
  return native::cuda::native_counting_strict();
}

bool cuda_supports_unique_counting_baseline(ScalarType dtype, bool sorted) noexcept {
  return native::cuda::supports_unique_counting_baseline(dtype, sorted);
}

bool cuda_supports_bincount_counting_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_bincount_counting_baseline(dtype);
}

bool cuda_supports_histc_counting_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_histc_counting_baseline(dtype);
}

bool cuda_supports_unique_counting(const Series& a, bool sorted) noexcept {
  return native::cuda::supports_unique_counting(a, sorted);
}

bool cuda_supports_bincount_counting(const Series& a, std::int64_t minlength) noexcept {
  return native::cuda::supports_bincount_counting(a, minlength);
}

bool cuda_supports_histc_counting(const Series& a, std::int64_t bins) noexcept {
  return native::cuda::supports_histc_counting(a, bins);
}

bool cuda_native_factory_enabled() noexcept {
  return native::cuda::native_factory_enabled();
}

bool cuda_native_factory_strict() noexcept {
  return native::cuda::native_factory_strict();
}

bool cuda_supports_zeros_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_zeros_factory_baseline(dtype);
}

bool cuda_supports_ones_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_ones_factory_baseline(dtype);
}

bool cuda_supports_full_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_full_factory_baseline(dtype);
}

bool cuda_supports_arange_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_arange_factory_baseline(dtype);
}

bool cuda_supports_linspace_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_linspace_factory_baseline(dtype);
}

bool cuda_supports_eye_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_eye_factory_baseline(dtype);
}

bool cuda_supports_rand_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_rand_factory_baseline(dtype);
}

bool cuda_supports_randn_factory_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_randn_factory_baseline(dtype);
}

bool cuda_native_stochastic_enabled() noexcept {
  return native::cuda::native_stochastic_enabled();
}

bool cuda_native_stochastic_strict() noexcept {
  return native::cuda::native_stochastic_strict();
}

bool cuda_supports_bernoulli_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_bernoulli_stochastic_baseline(dtype);
}

bool cuda_supports_normal_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_normal_stochastic_baseline(dtype);
}

bool cuda_supports_log_normal_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_log_normal_stochastic_baseline(dtype);
}

bool cuda_supports_uniform_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_uniform_stochastic_baseline(dtype);
}

bool cuda_supports_exponential_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_exponential_stochastic_baseline(dtype);
}

bool cuda_supports_geometric_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_geometric_stochastic_baseline(dtype);
}

bool cuda_supports_cauchy_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_cauchy_stochastic_baseline(dtype);
}

bool cuda_supports_random_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_random_stochastic_baseline(dtype);
}

bool cuda_supports_multinomial_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_multinomial_stochastic_baseline(dtype);
}

bool cuda_supports_poisson_stochastic_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_poisson_stochastic_baseline(dtype);
}

bool cuda_supports_bernoulli_stochastic(const Series& a) noexcept {
  return native::cuda::supports_bernoulli_stochastic(a);
}

bool cuda_supports_normal_stochastic(const Series& a) noexcept {
  return native::cuda::supports_normal_stochastic(a);
}

bool cuda_supports_log_normal_stochastic(const Series& a) noexcept {
  return native::cuda::supports_log_normal_stochastic(a);
}

bool cuda_supports_uniform_stochastic(const Series& a) noexcept {
  return native::cuda::supports_uniform_stochastic(a);
}

bool cuda_supports_exponential_stochastic(const Series& a) noexcept {
  return native::cuda::supports_exponential_stochastic(a);
}

bool cuda_supports_geometric_stochastic(const Series& a) noexcept {
  return native::cuda::supports_geometric_stochastic(a);
}

bool cuda_supports_cauchy_stochastic(const Series& a) noexcept {
  return native::cuda::supports_cauchy_stochastic(a);
}

bool cuda_supports_random_stochastic(const Series& a) noexcept {
  return native::cuda::supports_random_stochastic(a);
}

bool cuda_supports_multinomial_stochastic(const Series& a, std::int64_t num_samples, bool replacement) noexcept {
  return native::cuda::supports_multinomial_stochastic(a, num_samples, replacement);
}

bool cuda_supports_poisson_stochastic(ScalarType dtype) noexcept {
  return native::cuda::supports_poisson_stochastic(dtype);
}

bool cuda_native_selection_enabled() noexcept {
  return native::cuda::native_selection_enabled();
}

bool cuda_native_selection_strict() noexcept {
  return native::cuda::native_selection_strict();
}

bool cuda_supports_sort_selection_baseline(ScalarType dtype, bool stable) noexcept {
  return native::cuda::supports_sort_selection_baseline(dtype, stable);
}

bool cuda_supports_topk_selection_baseline(ScalarType dtype, bool sorted) noexcept {
  return native::cuda::supports_topk_selection_baseline(dtype, sorted);
}

bool cuda_supports_kthvalue_selection_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_kthvalue_selection_baseline(dtype);
}

bool cuda_supports_argsort_selection_baseline(ScalarType dtype, bool stable) noexcept {
  return native::cuda::supports_argsort_selection_baseline(dtype, stable);
}

bool cuda_supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  return native::cuda::supports_sort_selection(a, dim, stable);
}

bool cuda_supports_topk_selection(const Series& a, std::int64_t k, std::int64_t dim, bool sorted) noexcept {
  return native::cuda::supports_topk_selection(a, k, dim, sorted);
}

bool cuda_supports_kthvalue_selection(const Series& a, std::int64_t k, std::int64_t dim) noexcept {
  return native::cuda::supports_kthvalue_selection(a, k, dim);
}

bool cuda_supports_argsort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  return native::cuda::supports_argsort_selection(a, dim, stable);
}

bool cuda_native_linalg_enabled() noexcept {
  return native::cuda::native_linalg_enabled();
}

bool cuda_native_linalg_strict() noexcept {
  return native::cuda::native_linalg_strict();
}

bool cuda_supports_det_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_det_baseline(dtype);
}

bool cuda_supports_inverse_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_inverse_baseline(dtype);
}

bool cuda_supports_cholesky_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_cholesky_baseline(dtype);
}

bool cuda_supports_logdet_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_logdet_baseline(dtype);
}

bool cuda_supports_slogdet_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_slogdet_baseline(dtype);
}

bool cuda_supports_pinverse_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_pinverse_baseline(dtype);
}

bool cuda_supports_triangular_solve_linalg_baseline(ScalarType a_dtype, ScalarType b_dtype) noexcept {
  return native::cuda::supports_triangular_solve_baseline(a_dtype, b_dtype);
}

bool cuda_supports_cholesky_solve_linalg_baseline(ScalarType factor_dtype, ScalarType b_dtype) noexcept {
  return native::cuda::supports_cholesky_solve_baseline(factor_dtype, b_dtype);
}

bool cuda_supports_cholesky_inverse_linalg_baseline(ScalarType factor_dtype) noexcept {
  return native::cuda::supports_cholesky_inverse_baseline(factor_dtype);
}

bool cuda_supports_lu_solve_linalg_baseline(ScalarType lu_dtype, ScalarType b_dtype) noexcept {
  return native::cuda::supports_lu_solve_baseline(lu_dtype, b_dtype);
}

bool cuda_supports_qr_linalg_baseline(ScalarType dtype, bool reduced) noexcept {
  return native::cuda::supports_qr_baseline(dtype, reduced);
}

bool cuda_supports_svd_linalg_baseline(ScalarType dtype, bool full_matrices) noexcept {
  return native::cuda::supports_svd_baseline(dtype, full_matrices);
}

bool cuda_supports_matrix_power_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_matrix_power_baseline(dtype);
}

bool cuda_supports_matrix_exp_linalg_baseline(ScalarType dtype) noexcept {
  return native::cuda::supports_matrix_exp_baseline(dtype);
}

bool cuda_supports_det_linalg(const Series& a) noexcept {
  return native::cuda::supports_det(a);
}

bool cuda_supports_inverse_linalg(const Series& a) noexcept {
  return native::cuda::supports_inverse(a);
}

bool cuda_supports_cholesky_linalg(const Series& a) noexcept {
  return native::cuda::supports_cholesky(a);
}

bool cuda_supports_logdet_linalg(const Series& a) noexcept {
  return native::cuda::supports_logdet(a);
}

bool cuda_supports_slogdet_linalg(const Series& a) noexcept {
  return native::cuda::supports_slogdet(a);
}

bool cuda_supports_pinverse_linalg(const Series& a) noexcept {
  return native::cuda::supports_pinverse(a);
}

bool cuda_supports_triangular_solve_linalg(const Series& a, const Series& b) noexcept {
  return native::cuda::supports_triangular_solve(a, b);
}

bool cuda_supports_cholesky_solve_linalg(const Series& factor, const Series& b) noexcept {
  return native::cuda::supports_cholesky_solve(factor, b);
}

bool cuda_supports_cholesky_inverse_linalg(const Series& factor) noexcept {
  return native::cuda::supports_cholesky_inverse(factor);
}

bool cuda_supports_lu_solve_linalg(const Series& lu, const Series& b) noexcept {
  return native::cuda::supports_lu_solve(lu, b);
}

bool cuda_supports_qr_linalg(const Series& a, bool reduced) noexcept {
  return native::cuda::supports_qr(a, reduced);
}

bool cuda_supports_svd_linalg(const Series& a, bool full_matrices) noexcept {
  return native::cuda::supports_svd(a, full_matrices);
}

bool cuda_supports_matrix_power_linalg(const Series& a) noexcept {
  return native::cuda::supports_matrix_power(a);
}

bool cuda_supports_matrix_exp_linalg(const Series& a) noexcept {
  return native::cuda::supports_matrix_exp(a);
}

bool cuda_native_indexing_enabled() noexcept {
  return native::cuda::native_indexing_enabled();
}

bool cuda_native_indexing_strict() noexcept {
  return native::cuda::native_indexing_strict();
}

bool cuda_supports_take_put_indexing_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept {
  return native::cuda::supports_take_put_baseline(value_dtype, index_dtype);
}

bool cuda_supports_nonzero_indexing_baseline(ScalarType value_dtype) noexcept {
  return native::cuda::supports_nonzero_baseline(value_dtype);
}

bool cuda_supports_index_indexing_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept {
  return native::cuda::supports_index_baseline(value_dtype, index_dtype);
}

bool cuda_supports_take_indexing(const Series& self, const Series& index) noexcept {
  return native::cuda::supports_take(self, index);
}

bool cuda_supports_put_indexing(const Series& self, const Series& index, const Series& source, bool accumulate) noexcept {
  return native::cuda::supports_put(self, index, source, accumulate);
}

bool cuda_supports_nonzero_indexing(const Series& self) noexcept {
  return native::cuda::supports_nonzero(self);
}

bool cuda_supports_index_indexing(const Series& self, pluss::msharp::RangeView::Of<Series> indices) noexcept {
  return native::cuda::supports_index(self, indices);
}

bool cuda_native_matmul_enabled() noexcept {
  return native::cuda::native_matmul_enabled();
}

bool cuda_native_matmul_strict() noexcept {
  return native::cuda::native_matmul_strict();
}

bool cuda_supports_matmul_native_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  return native::cuda::supports_matmul_baseline(lhs_dtype, rhs_dtype);
}

bool cuda_supports_matmul_native(const Series& a, const Series& b) noexcept {
  return native::cuda::supports_matmul(a, b);
}

bool cuda_native_scatter_gather_enabled() noexcept {
  return native::cuda::native_scatter_gather_enabled();
}

bool cuda_native_scatter_gather_strict() noexcept {
  return native::cuda::native_scatter_gather_strict();
}

bool cuda_supports_scatter_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept {
  return native::cuda::supports_scatter_baseline(self_dtype, index_dtype, src_dtype);
}

bool cuda_supports_scatter_add_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept {
  return native::cuda::supports_scatter_add_baseline(self_dtype, index_dtype, src_dtype);
}

bool cuda_supports_scatter_reduce_baseline(
    ScalarType self_dtype,
    ScalarType index_dtype,
    ScalarType src_dtype,
    std::string_view reduce) noexcept {
  return native::cuda::supports_scatter_reduce_baseline(self_dtype, index_dtype, src_dtype, reduce);
}

bool cuda_supports_gather_baseline(ScalarType input_dtype, ScalarType index_dtype) noexcept {
  return native::cuda::supports_gather_baseline(input_dtype, index_dtype);
}

bool cuda_supports_scatter_native(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept {
  return native::cuda::supports_scatter(self, dim, index, src);
}

bool cuda_supports_scatter_add_native(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src) noexcept {
  return native::cuda::supports_scatter_add(self, dim, index, src);
}

bool cuda_supports_scatter_reduce_native(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src,
    std::string_view reduce) noexcept {
  return native::cuda::supports_scatter_reduce(self, dim, index, src, reduce);
}

bool cuda_supports_gather_native(const Series& input, std::int64_t dim, const Series& index) noexcept {
  return native::cuda::supports_gather(input, dim, index);
}

} // namespace pluss::codem
