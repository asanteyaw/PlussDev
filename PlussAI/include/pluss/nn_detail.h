#pragma once

#include "pluss/core.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::detail::nn {

inline Config default_cfg_() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

inline Config floating_cfg_(Config cfg) {
  if (!pluss::codem::dtype_of(cfg.dtype).is_floating()) {
    cfg = cfg.with_dtype(ScalarType::Float32);
  }
  return cfg;
}

inline bool is_training_float_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float32 ||
         dtype == ScalarType::Float16 ||
         dtype == ScalarType::BFloat16 ||
         dtype == ScalarType::Float64;
}

inline void require_training_float_dtype_(ScalarType dtype, const char* what, const char* role) {
  PLUSS_CHECK(
      is_training_float_dtype_(dtype),
      std::string("pluss::") + what + ": expected " + role +
          " dtype in {Float32, Float16, BFloat16, Float64}; got enum=" +
          std::to_string(static_cast<int>(dtype)));
}

// PARAMETERS (unlike the loss they feed into, which must stay real-valued
// for "minimize" to mean anything) may additionally be complex -- matching
// real torch, whose Adam/AdamW/RMSprop/NAdam/Adagrad/SGD all accept complex
// Parameters (verified live: torch.optim.Adam trains a complex64 Parameter
// correctly). Deliberately a SEPARATE function from
// is_training_float_dtype_ rather than widening it in place: loss/target/
// prediction dtype checks throughout Loss.h reuse that one and must keep
// rejecting complex, since a loss value has to be real to have a
// well-defined descent direction.
inline bool is_training_numeric_dtype_(ScalarType dtype) noexcept {
  return is_training_float_dtype_(dtype) ||
         dtype == ScalarType::Complex64 ||
         dtype == ScalarType::Complex128;
}

inline void require_training_numeric_dtype_(ScalarType dtype, const char* what, const char* role) {
  PLUSS_CHECK(
      is_training_numeric_dtype_(dtype),
      std::string("pluss::") + what + ": expected " + role +
          " dtype in {Float32, Float16, BFloat16, Float64, Complex64, Complex128}; got enum=" +
          std::to_string(static_cast<int>(dtype)));
}

inline void require_defined_(const Series& x, const char* what) {
  PLUSS_CHECK(x.defined(), std::string("pluss::") + what + ": expected defined Series");
}

inline void require_rank_at_least_(const Series& x, std::int64_t rank, const char* what) {
  require_defined_(x, what);
  PLUSS_CHECK(x.dim() >= rank, std::string("pluss::") + what + ": expected rank >= " + std::to_string(rank));
}

inline Series select_dim_(const Series& x, std::int64_t dim, std::int64_t index) {
  require_defined_(x, "select_dim");
  dim = (dim < 0) ? dim + x.dim() : dim;
  PLUSS_CHECK(dim >= 0 && dim < x.dim(), "pluss::select_dim: dim out of range");
  const std::int64_t dim_size = x.sizes()[static_cast<std::size_t>(dim)];
  if (index < 0) index += dim_size;
  PLUSS_CHECK(index >= 0 && index < dim_size, "pluss::select_dim: index out of range");
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  const Series idx = pluss::series::arange(index, index + 1, 1, icfg);
  Series picked = x.index_select(dim, idx);
  std::vector<std::int64_t> out_shape(x.sizes().begin(), x.sizes().end());
  out_shape.erase(out_shape.begin() + static_cast<std::size_t>(dim));
  if (out_shape.empty()) out_shape.push_back(1);
  return picked.reshape(out_shape).contiguous();
}

inline Series narrow_dim_(const Series& x, std::int64_t dim, std::int64_t start, std::int64_t len) {
  require_defined_(x, "narrow_dim");
  dim = (dim < 0) ? dim + x.dim() : dim;
  PLUSS_CHECK(dim >= 0 && dim < x.dim(), "pluss::narrow_dim: dim out of range");
  PLUSS_CHECK(len >= 0, "pluss::narrow_dim: len must be >= 0");
  const std::int64_t dim_size = x.sizes()[static_cast<std::size_t>(dim)];
  if (start < 0) start += dim_size;
  PLUSS_CHECK(start >= 0 && start <= dim_size, "pluss::narrow_dim: start out of range");
  PLUSS_CHECK(start + len <= dim_size, "pluss::narrow_dim: start + len out of range");
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  return x.index_select(dim, pluss::series::arange(start, start + len, 1, icfg));
}

inline Series cast_like_(const Series& x, const Series& like) {
  if (!x.defined()) return x;
  if (x.platform() == like.platform() && x.scalar_type() == like.scalar_type()) return x;
  return x.to(like.platform(), like.scalar_type());
}

inline Series zeros_state_(pluss::msharp::RangeView::Int sizes, const Series& like) {
  return pluss::series::zeros(sizes, Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
}

inline Series zeros_state_(std::initializer_list<std::int64_t> sizes, const Series& like) {
  return zeros_state_(pluss::msharp::RangeView::Int(sizes), like);
}

inline Series uniform_(pluss::msharp::RangeView::Int sizes, double lo, double hi, Config cfg, std::uint64_t seed) {
  const Config fcfg = floating_cfg_(cfg);
  return pluss::series::rand(sizes, fcfg, seed).mul(hi - lo).add(lo);
}

inline Series uniform_(std::initializer_list<std::int64_t> sizes, double lo, double hi, Config cfg, std::uint64_t seed) {
  return uniform_(pluss::msharp::RangeView::Int(sizes), lo, hi, cfg, seed);
}

inline Series dropout_(const Series& x, double p, bool training, std::uint64_t seed) {
  require_defined_(x, "dropout");
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "pluss::dropout: expected p in [0, 1]");
  if (!training || p == 0.0) return x;
  if (p == 1.0) {
    return pluss::series::zeros(x.sizes(), Config{}.with_dtype(x.scalar_type()).with_platform(x.platform()));
  }
  const ScalarType mask_dtype = x.platform() == pMPS ? ScalarType::Float32 : ScalarType::Float64;
  const Series mask = pluss::series::rand(
                          x.sizes(),
                          Config{}.with_dtype(mask_dtype).with_platform(x.platform()),
                          seed)
                          .ge(p)
                          .to(x.scalar_type());
  return x.mul(mask).div(1.0 - p);
}

inline Series affine_(const Series& input, const Series& weight, const Series& bias, const char* what) {
  require_rank_at_least_(input, 1, what);
  require_rank_at_least_(weight, 2, what);
  PLUSS_CHECK(weight.dim() == 2, std::string("pluss::") + what + ": expected 2D weight");
  PLUSS_CHECK(input.sizes()[input.dim() - 1] == weight.sizes()[1],
              std::string("pluss::") + what + ": input feature size does not match weight");
  const Series w = cast_like_(weight, input);
  const std::int64_t in_features = input.sizes()[input.dim() - 1];
  const std::int64_t out_features = w.sizes()[0];
  std::vector<std::int64_t> flat_shape{
      input.dim() == 1 ? 1 : input.numel() / in_features,
      in_features,
  };
  const Series input_ready = input.is_contiguous() ? input : input.contiguous();
  const Series flat_input = input_ready.dim() == 2 ? input_ready : input_ready.reshape(flat_shape);
  const Series wt = w.t().contiguous();
  Series flat_out = flat_input.matmul(wt);
  if (bias.defined()) {
    const Series b = cast_like_(bias, input);
    PLUSS_CHECK(b.dim() == 1, std::string("pluss::") + what + ": expected 1D bias");
    PLUSS_CHECK(b.sizes()[0] == w.sizes()[0], std::string("pluss::") + what + ": bias size mismatch");
    flat_out = flat_out.add(b);
  }
  if (input.dim() == 2) return flat_out;
  std::vector<std::int64_t> out_shape(input.sizes().begin(), input.sizes().end());
  out_shape.back() = out_features;
  return flat_out.reshape(out_shape);
}

inline Series apply_nonlinearity_(const Series& x, std::string_view nonlinearity) {
  if (nonlinearity == "tanh") return x.tanh();
  if (nonlinearity == "relu") return x.relu();
  static const bool pluss_vocab_1 = pluss::exception::names::define("nonlinearity", {"tanh", "relu"});
  (void)pluss_vocab_1;
  PLUSS_FAIL(std::string("pluss::rnn: unsupported nonlinearity '") + std::string(nonlinearity) + "'");
}

inline std::vector<std::int64_t> reverse_indices_(std::int64_t n) {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(n));
  std::iota(idx.begin(), idx.end(), std::int64_t{0});
  std::reverse(idx.begin(), idx.end());
  return idx;
}

inline Series reverse_time_(const Series& x) {
  const auto idx = reverse_indices_(x.sizes()[0]);
  const Config cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  return x.index_select(0, pluss::make_series(idx, {static_cast<std::int64_t>(idx.size())}, cfg));
}

inline std::vector<std::int64_t> sorted_batch_indices_(const std::vector<std::int64_t>& lengths) {
  std::vector<std::int64_t> idx(lengths.size());
  for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = static_cast<std::int64_t>(i);
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    return lengths[static_cast<std::size_t>(a)] > lengths[static_cast<std::size_t>(b)];
  });
  return idx;
}

inline std::vector<std::int64_t> inverse_permutation_(const std::vector<std::int64_t>& perm) {
  std::vector<std::int64_t> inv(perm.size());
  for (std::size_t i = 0; i < perm.size(); ++i) inv[static_cast<std::size_t>(perm[i])] = static_cast<std::int64_t>(i);
  return inv;
}

} // namespace pluss::detail::nn
