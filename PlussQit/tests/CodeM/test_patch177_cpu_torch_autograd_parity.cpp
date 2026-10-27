#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#if defined(PLUSSQIT_AUTODIFF_TORCH_ORACLE)

#include <pluss/codem/codem.h>
#include <pluss/codem/series_factory.h>
#include <pluss/subsp/linearalg/linearalg.h>
#include <pluss/subsp/math/diff.h>

#include <torch/csrc/autograd/autograd.h>

#include <ATen/core/Tensor.h>
#include <ATen/ops/_addmm_activation.h>
#include <ATen/ops/_make_dual.h>
#include <ATen/ops/_unpack_dual.h>
#include <ATen/ops/addmm.h>
#include <ATen/ops/amax.h>
#include <ATen/ops/amin.h>
#include <ATen/ops/atan2.h>
#include <ATen/ops/hypot.h>
#include <ATen/ops/relu.h>
#include <ATen/ops/fmax.h>
#include <ATen/ops/fmin.h>
#include <ATen/ops/gelu.h>
#include <ATen/ops/linalg_cholesky.h>
#include <ATen/ops/linalg_det.h>
#include <ATen/ops/linalg_inv.h>
#include <ATen/ops/linalg_matrix_exp.h>
#include <ATen/ops/linalg_matrix_power.h>
#include <ATen/ops/linalg_solve_triangular.h>
#include <ATen/ops/linalg_qr.h>
#include <ATen/ops/linalg_slogdet.h>
#include <ATen/ops/linalg_svd.h>
#include <ATen/ops/linalg_pinv.h>
#include <ATen/ops/logdet.h>
#include <ATen/ops/matrix_power.h>
#include <ATen/ops/maximum.h>
#include <ATen/ops/minimum.h>
#include <ATen/ops/mul.h>
#include <ATen/ops/celu.h>
#include <ATen/ops/cross_entropy_loss.h>
#include <ATen/ops/elu.h>
#include <ATen/ops/embedding.h>
#include <ATen/ops/hann_window.h>
#include <ATen/ops/hardsigmoid.h>
#include <ATen/ops/hardswish.h>
#include <ATen/ops/istft.h>
#include <ATen/ops/layer_norm.h>
#include <ATen/ops/leaky_relu.h>
#include <ATen/ops/mish.h>
#include <ATen/ops/nan_to_num.h>
#include <ATen/ops/ones.h>
#include <ATen/ops/rrelu.h>
#include <ATen/ops/selu.h>
#include <ATen/ops/silu.h>
#include <ATen/ops/softplus.h>
#include <ATen/ops/cat.h>
#include <ATen/ops/hardshrink.h>
#include <ATen/ops/hardshrink_backward.h>
#include <ATen/ops/stack.h>
#include <ATen/ops/std.h>
#include <ATen/ops/threshold.h>
#include <ATen/ops/stft.h>
#include <ATen/ops/tensor.h>
#include <ATen/ops/var.h>
#include <ATen/ops/zeros.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::jvp;

// libtorch's own forward-mode AD (dual numbers): wraps `f` between
// enter/exit_dual_level so the returned tangent is libtorch's actual JVP,
// not a hand re-derivation -- the same "check against local torch, not our
// own math" standard the reverse-mode oracle tests hold to.
std::pair<at::Tensor, at::Tensor> torch_jvp(
    const std::function<at::Tensor(const at::Tensor&)>& f,
    const at::Tensor& primal,
    const at::Tensor& tangent) {
  const auto level = torch::autograd::forward_ad::enter_dual_level();
  at::Tensor out_primal;
  at::Tensor out_tangent;
  try {
    const at::Tensor dual_in = at::_make_dual(primal, tangent, static_cast<std::int64_t>(level));
    const at::Tensor dual_out = f(dual_in);
    const auto unpacked = at::_unpack_dual(dual_out, static_cast<std::int64_t>(level));
    out_primal = std::get<0>(unpacked).clone();
    out_tangent = std::get<1>(unpacked).clone();
  } catch (...) {
    torch::autograd::forward_ad::exit_dual_level(level);
    throw;
  }
  torch::autograd::forward_ad::exit_dual_level(level);
  return {out_primal, out_tangent};
}

// Multi-input variant, for ops (scatter/scatter_add/pile) that need more
// than one differentiable operand traced simultaneously.
std::pair<at::Tensor, at::Tensor> torch_jvp_n(
    const std::function<at::Tensor(const std::vector<at::Tensor>&)>& f,
    const std::vector<at::Tensor>& primals,
    const std::vector<at::Tensor>& tangents) {
  const auto level = torch::autograd::forward_ad::enter_dual_level();
  at::Tensor out_primal;
  at::Tensor out_tangent;
  try {
    std::vector<at::Tensor> duals;
    duals.reserve(primals.size());
    for (std::size_t i = 0; i < primals.size(); ++i) {
      duals.push_back(at::_make_dual(primals[i], tangents[i], static_cast<std::int64_t>(level)));
    }
    const at::Tensor dual_out = f(duals);
    const auto unpacked = at::_unpack_dual(dual_out, static_cast<std::int64_t>(level));
    out_primal = std::get<0>(unpacked).clone();
    out_tangent = std::get<1>(unpacked).clone();
  } catch (...) {
    torch::autograd::forward_ad::exit_dual_level(level);
    throw;
  }
  torch::autograd::forward_ad::exit_dual_level(level);
  return {out_primal, out_tangent};
}

std::vector<double> torch_values(const at::Tensor& value) {
  const at::Tensor cpu = value.detach().to(at::kCPU, at::kDouble).contiguous();
  const double* data = cpu.const_data_ptr<double>();
  return std::vector<double>(data, data + cpu.numel());
}

void require_close(const std::vector<double>& got,
                   const std::vector<double>& expected,
                   double margin = 1e-12) {
  REQUIRE(got.size() == expected.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO("element " << i << ": Pluss=" << got[i] << ", torch=" << expected[i]);
    if (std::isnan(expected[i])) {
      REQUIRE(std::isnan(got[i]));
      continue;
    }
    REQUIRE(got[i] == Approx(expected[i]).margin(margin));
  }
}

std::vector<std::complex<double>> torch_complex_values(const at::Tensor& value) {
  const at::Tensor cpu = value.detach().to(at::kCPU, at::kComplexDouble).contiguous();
  const c10::complex<double>* data = cpu.const_data_ptr<c10::complex<double>>();
  std::vector<std::complex<double>> out;
  out.reserve(static_cast<std::size_t>(cpu.numel()));
  for (std::int64_t i = 0; i < cpu.numel(); ++i) {
    out.emplace_back(data[i].real(), data[i].imag());
  }
  return out;
}

std::vector<double> spectral_signal(std::int64_t n, std::uint64_t seed) {
  std::vector<double> v(static_cast<std::size_t>(n));
  std::uint64_t s = seed;
  for (auto& e : v) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    e = static_cast<double>(s >> 40) / static_cast<double>(1ULL << 24) * 2.0 - 1.0;
  }
  return v;
}

void require_complex_close(const std::vector<std::complex<double>>& got,
                           const std::vector<std::complex<double>>& expected,
                           double margin = 1e-11) {
  REQUIRE(got.size() == expected.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO("element " << i << ": Pluss=" << got[i] << ", torch=" << expected[i]);
    REQUIRE(got[i].real() == Approx(expected[i].real()).margin(margin));
    REQUIRE(got[i].imag() == Approx(expected[i].imag()).margin(margin));
  }
}

struct ScatterCase final {
  std::string reduce;
  std::vector<double> receiver;
  std::vector<double> source;
};

struct UnaryCase final {
  std::string name;
  std::vector<double> input;
  std::function<Series(const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&)> torch_op;
};

struct ComplexUnaryCase final {
  std::string name;
  std::function<Series(const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&)> torch_op;
};

struct ComplexBinaryCase final {
  std::string name;
  std::function<Series(const Series&, const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&, const at::Tensor&)> torch_op;
};

Series complex_series(const std::vector<std::complex<double>>& values,
                      const std::vector<std::int64_t>& shape) {
  return make_series(values, shape).to(ScalarType::Complex128);
}

at::Tensor torch_complex_series(const std::vector<c10::complex<double>>& values,
                                const std::vector<std::int64_t>& shape,
                                bool tracks_slope = false) {
  return at::tensor(values, at::TensorOptions().dtype(at::kComplexDouble))
      .reshape(shape)
      .set_requires_grad(tracks_slope);
}

} // namespace

TEST_CASE("complex CPU Cholesky reverse matches libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {4.0, 0.0}, {1.0, 1.0}, {1.0, -1.0}, {3.0, 0.0}};
  const std::vector<std::complex<double>> seed_values{
      {0.4, -0.2}, {0.0, 0.0}, {-0.7, 0.3}, {1.1, -0.5}};
  const std::vector<c10::complex<double>> torch_input_values{
      {4.0, 0.0}, {1.0, 1.0}, {1.0, -1.0}, {3.0, 0.0}};
  const std::vector<c10::complex<double>> torch_seed_values{
      {0.4, -0.2}, {0.0, 0.0}, {-0.7, 0.3}, {1.1, -0.5}};

  for (bool upper : {false, true}) {
    DYNAMIC_SECTION("upper=" << upper) {
      Series input = complex_series(values, {2, 2});
      input.tracks_slope_(true);
      const Series output = pluss::subsp::linearalg::cholesky(input, upper);
      output.reverse(complex_series(seed_values, {2, 2}));

      at::Tensor torch_input = torch_complex_series(torch_input_values, {2, 2}, true);
      const at::Tensor torch_output = at::linalg_cholesky(torch_input, upper);
      torch_output.backward(torch_complex_series(torch_seed_values, {2, 2}));

      require_complex_close(input.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_input.grad()), 2e-10);
    }
  }
}

TEST_CASE("complex CPU reduced QR reverse matches libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {1.2, 0.4}, {-0.3, 0.7}, {0.5, -0.2}, {1.7, 0.1},
      {-0.8, 0.6}, {0.9, -0.5}};
  const std::vector<c10::complex<double>> torch_input_values{
      {1.2, 0.4}, {-0.3, 0.7}, {0.5, -0.2}, {1.7, 0.1},
      {-0.8, 0.6}, {0.9, -0.5}};

  for (bool q_path : {true, false}) {
    DYNAMIC_SECTION("output=" << (q_path ? "Q" : "R")) {
      Series input = complex_series(values, {3, 2});
      input.tracks_slope_(true);
      const Series output = q_path
          ? pluss::subsp::linearalg::qr_q(input, true)
          : pluss::subsp::linearalg::qr_r(input, true);
      std::vector<std::complex<double>> seed_values(
          static_cast<std::size_t>(output.numel()));
      for (std::size_t i = 0; i < seed_values.size(); ++i) {
        seed_values[i] = {0.2 + 0.07 * static_cast<double>(i),
                          -0.3 + 0.04 * static_cast<double>(i)};
      }
      output.reverse(complex_series(
          seed_values,
          std::vector<std::int64_t>(output.sizes().begin(), output.sizes().end())));

      at::Tensor torch_input = torch_complex_series(torch_input_values, {3, 2}, true);
      auto [torch_q, torch_r] = at::linalg_qr(torch_input, "reduced");
      const at::Tensor torch_output = q_path ? torch_q : torch_r;
      std::vector<c10::complex<double>> torch_seed_values;
      torch_seed_values.reserve(seed_values.size());
      for (const auto& value : seed_values) {
        torch_seed_values.emplace_back(value.real(), value.imag());
      }
      torch_output.backward(torch_complex_series(
          torch_seed_values,
          std::vector<std::int64_t>(torch_output.sizes().begin(), torch_output.sizes().end())));

      require_complex_close(input.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_input.grad()), 3e-10);
    }
  }
}

TEST_CASE("complex CPU wide QR reverse matches reduced and complete libtorch modes",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {1.2, 0.4}, {-0.3, 0.7}, {0.8, -0.6},
      {0.5, -0.2}, {1.7, 0.1}, {-0.4, 0.9}};
  const std::vector<c10::complex<double>> torch_input_values{
      {1.2, 0.4}, {-0.3, 0.7}, {0.8, -0.6},
      {0.5, -0.2}, {1.7, 0.1}, {-0.4, 0.9}};

  for (bool reduced : {true, false}) {
    for (bool q_path : {true, false}) {
      DYNAMIC_SECTION("reduced=" << reduced << ", output=" << (q_path ? "Q" : "R")) {
        Series input = complex_series(values, {2, 3});
        input.tracks_slope_(true);
        const Series output = q_path
            ? pluss::subsp::linearalg::qr_q(input, reduced)
            : pluss::subsp::linearalg::qr_r(input, reduced);
        std::vector<std::complex<double>> seed_values(
            static_cast<std::size_t>(output.numel()));
        for (std::size_t i = 0; i < seed_values.size(); ++i) {
          seed_values[i] = {-0.15 + 0.05 * static_cast<double>(i),
                            0.22 - 0.03 * static_cast<double>(i)};
        }
        output.reverse(complex_series(
            seed_values,
            std::vector<std::int64_t>(output.sizes().begin(), output.sizes().end())));

        at::Tensor torch_input = torch_complex_series(torch_input_values, {2, 3}, true);
        auto [torch_q, torch_r] = at::linalg_qr(
            torch_input, reduced ? "reduced" : "complete");
        const at::Tensor torch_output = q_path ? torch_q : torch_r;
        std::vector<c10::complex<double>> torch_seed_values;
        for (const auto& value : seed_values) {
          torch_seed_values.emplace_back(value.real(), value.imag());
        }
        torch_output.backward(torch_complex_series(
            torch_seed_values,
            std::vector<std::int64_t>(torch_output.sizes().begin(), torch_output.sizes().end())));

        require_complex_close(input.sensitivity().host<std::complex<double>>(),
                              torch_complex_values(torch_input.grad()), 4e-10);
      }
    }
  }
}

TEST_CASE("complex CPU reduced SVD reverse matches phase-invariant libtorch paths",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};
  const std::vector<c10::complex<double>> torch_input_values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};

  for (int slot = 0; slot < 3; ++slot) {
    DYNAMIC_SECTION("slot=" << slot) {
      Series input = complex_series(values, {3, 2});
      input.tracks_slope_(true);
      const auto decomposition = pluss::subsp::linearalg::svd(input, false);
      Series output = slot == 0 ? decomposition.u :
          (slot == 1 ? decomposition.s : decomposition.vh);
      Series seed = pluss::series::undefined();
      if (slot == 0) {
        seed = output.mul(make_series(
            std::vector<double>{0.4, 1.1, 0.7, 0.4, 1.1, 0.7}, {3, 2})
                              .to(ScalarType::Complex128));
      } else if (slot == 1) {
        seed = make_series(std::vector<double>{0.6, -0.9}, {2});
      } else {
        seed = output.mul(make_series(
            std::vector<double>{0.3, 1.2, 0.3, 1.2}, {2, 2})
                              .to(ScalarType::Complex128));
      }
      output.reverse(seed);

      at::Tensor torch_input = torch_complex_series(torch_input_values, {3, 2}, true);
      auto [torch_u, torch_s, torch_vh] = at::linalg_svd(torch_input, false);
      at::Tensor torch_output = slot == 0 ? torch_u : (slot == 1 ? torch_s : torch_vh);
      at::Tensor torch_seed;
      if (slot == 0) {
        torch_seed = at::mul(torch_output, at::tensor(
            std::vector<double>{0.4, 1.1, 0.7, 0.4, 1.1, 0.7},
            at::TensorOptions().dtype(at::kDouble)).reshape({3, 2}));
      } else if (slot == 1) {
        torch_seed = at::tensor(
            std::vector<double>{0.6, -0.9}, at::TensorOptions().dtype(at::kDouble));
      } else {
        torch_seed = at::mul(torch_output, at::tensor(
            std::vector<double>{0.3, 1.2, 0.3, 1.2},
            at::TensorOptions().dtype(at::kDouble)).reshape({2, 2}));
      }
      torch_output.backward(torch_seed);

      require_complex_close(input.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_input.grad()), 8e-10);
    }
  }
}

TEST_CASE("complex CPU SVD reconstruction couples all output cotangents like libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};
  const std::vector<c10::complex<double>> torch_input_values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};
  const std::complex<double> seed_value{0.6, -0.35};

  for (bool full_matrices : {false, true}) {
    DYNAMIC_SECTION("full_matrices=" << full_matrices) {
      Series input = complex_series(values, {3, 2});
      input.tracks_slope_(true);
      const auto decomposition = pluss::subsp::linearalg::svd(input, full_matrices);
      const std::int64_t k = decomposition.s.sizes().back();
      const Series u = full_matrices
          ? decomposition.u.slice(-1, 0, k).contiguous()
          : decomposition.u;
      const Series vh = full_matrices
          ? decomposition.vh.slice(-2, 0, k).contiguous()
          : decomposition.vh;
      const Series reconstruction =
          u.mul(decomposition.s.to(ScalarType::Complex128).unsqueeze(0)).matmul(vh);
      reconstruction.sum().reverse(complex_series({seed_value}, {}));

      at::Tensor torch_input = torch_complex_series(torch_input_values, {3, 2}, true);
      auto [torch_u_full, torch_s, torch_vh_full] =
          at::linalg_svd(torch_input, full_matrices);
      const at::Tensor torch_u = full_matrices ? torch_u_full.slice(-1, 0, 2) : torch_u_full;
      const at::Tensor torch_vh = full_matrices ? torch_vh_full.slice(-2, 0, 2) : torch_vh_full;
      const at::Tensor torch_reconstruction =
          at::mul(torch_u, torch_s.unsqueeze(0)).matmul(torch_vh);
      torch_reconstruction.sum().backward(torch_complex_series(
          {c10::complex<double>{seed_value.real(), seed_value.imag()}}, {}));

      require_complex_close(input.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_input.grad()), 1e-9);
    }
  }
}

TEST_CASE("complex CPU SVD rejects phase-dependent singular-vector losses like libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};
  const std::vector<c10::complex<double>> torch_input_values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};
  const std::vector<std::complex<double>> seed_values{
      {0.4, -0.2}, {0.7, 0.1}, {-0.3, 0.8},
      {0.2, 0.5}, {-0.6, -0.4}, {0.9, 0.3}};
  const std::vector<c10::complex<double>> torch_seed_values{
      {0.4, -0.2}, {0.7, 0.1}, {-0.3, 0.8},
      {0.2, 0.5}, {-0.6, -0.4}, {0.9, 0.3}};

  Series input = complex_series(values, {3, 2});
  input.tracks_slope_(true);
  const auto decomposition = pluss::subsp::linearalg::svd(input, false);
  REQUIRE_THROWS_AS(
      decomposition.u.reverse(complex_series(seed_values, {3, 2})),
      pluss::Error);

  at::Tensor torch_input = torch_complex_series(torch_input_values, {3, 2}, true);
  auto [torch_u, torch_s, torch_vh] = at::linalg_svd(torch_input, false);
  (void)torch_s;
  (void)torch_vh;
  REQUIRE_THROWS(torch_u.backward(torch_complex_series(torch_seed_values, {3, 2})));

  Series standalone = complex_series(values, {3, 2});
  standalone.tracks_slope_(true);
  const Series standalone_u = pluss::subsp::linearalg::svd_u(standalone, false);
  REQUIRE_THROWS_AS(
      standalone_u.reverse(complex_series(seed_values, {3, 2})),
      pluss::Error);
}

TEST_CASE("complex CPU full SVD ignores null-space cotangents like libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {2.0, 0.3}, {-0.4, 0.8}, {0.7, -0.2}, {1.4, 0.5},
      {-0.6, 0.9}, {0.2, -0.7}};
  Series input = complex_series(values, {3, 2});
  input.tracks_slope_(true);
  const auto decomposition = pluss::subsp::linearalg::svd(input, true);
  const Series null_column = decomposition.u.slice(-1, 2, 3).contiguous();
  null_column.reverse(complex_series({{0.2, 0.4}, {-0.3, 0.8}, {0.7, -0.1}}, {3, 1}));
  require_complex_close(
      input.sensitivity().host<std::complex<double>>(),
      std::vector<std::complex<double>>(6), 1e-12);
}

TEST_CASE("complex CPU matrix powers and pseudoinverse reverse match libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> square_values{
      {1.8, 0.4}, {-0.3, 0.7}, {0.5, -0.2}, {1.4, 0.6}};
  const std::vector<c10::complex<double>> torch_square_values{
      {1.8, 0.4}, {-0.3, 0.7}, {0.5, -0.2}, {1.4, 0.6}};
  const std::vector<std::complex<double>> square_seed_values{
      {0.4, -0.1}, {0.7, 0.3}, {-0.2, 0.8}, {1.0, -0.5}};
  const std::vector<c10::complex<double>> torch_square_seed_values{
      {0.4, -0.1}, {0.7, 0.3}, {-0.2, 0.8}, {1.0, -0.5}};

  for (std::int64_t exponent : {-2, 0, 3}) {
    DYNAMIC_SECTION("matrix_power exponent=" << exponent) {
      Series input = complex_series(square_values, {2, 2});
      input.tracks_slope_(true);
      const Series output = pluss::subsp::linearalg::matrix_power(input, exponent);
      output.reverse(complex_series(square_seed_values, {2, 2}));

      at::Tensor torch_input = torch_complex_series(torch_square_values, {2, 2}, true);
      const at::Tensor torch_output = at::matrix_power(torch_input, exponent);
      torch_output.backward(torch_complex_series(torch_square_seed_values, {2, 2}));
      require_complex_close(input.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_input.grad()), 8e-10);
    }
  }

  const std::vector<std::complex<double>> rectangular_values{
      {1.7, 0.2}, {-0.4, 0.8}, {0.6, -0.3}, {1.2, 0.5},
      {-0.7, 0.4}, {0.3, -0.9}};
  const std::vector<c10::complex<double>> torch_rectangular_values{
      {1.7, 0.2}, {-0.4, 0.8}, {0.6, -0.3}, {1.2, 0.5},
      {-0.7, 0.4}, {0.3, -0.9}};
  const std::vector<std::complex<double>> pinv_seed_values{
      {0.3, -0.2}, {-0.6, 0.5}, {0.8, 0.1},
      {-0.4, -0.7}, {0.2, 0.9}, {1.1, -0.3}};
  const std::vector<c10::complex<double>> torch_pinv_seed_values{
      {0.3, -0.2}, {-0.6, 0.5}, {0.8, 0.1},
      {-0.4, -0.7}, {0.2, 0.9}, {1.1, -0.3}};

  Series input = complex_series(rectangular_values, {3, 2});
  input.tracks_slope_(true);
  const Series output = pluss::subsp::linearalg::pinverse(input, 1e-12);
  output.reverse(complex_series(pinv_seed_values, {2, 3}));

  at::Tensor torch_input = torch_complex_series(torch_rectangular_values, {3, 2}, true);
  const at::Tensor torch_output = at::linalg_pinv(torch_input, 1e-12, false);
  torch_output.backward(torch_complex_series(torch_pinv_seed_values, {2, 3}));
  require_complex_close(input.sensitivity().host<std::complex<double>>(),
                        torch_complex_values(torch_input.grad()), 2e-9);
}

TEST_CASE("complex CPU matrix exponential reverse matches libtorch block differential",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {0.3, 0.5}, {-0.7, 0.2}, {0.4, -0.6}, {0.1, 0.8}};
  const std::vector<c10::complex<double>> torch_values_in{
      {0.3, 0.5}, {-0.7, 0.2}, {0.4, -0.6}, {0.1, 0.8}};
  const std::vector<std::complex<double>> seed_values{
      {0.4, -0.3}, {0.8, 0.2}, {-0.5, 0.7}, {0.1, -0.9}};
  const std::vector<c10::complex<double>> torch_seed_values{
      {0.4, -0.3}, {0.8, 0.2}, {-0.5, 0.7}, {0.1, -0.9}};

  Series input = complex_series(values, {2, 2});
  input.tracks_slope_(true);
  const Series output = pluss::subsp::linearalg::matrix_exp(input);
  output.reverse(complex_series(seed_values, {2, 2}));

  at::Tensor torch_input = torch_complex_series(torch_values_in, {2, 2}, true);
  const at::Tensor torch_output = at::linalg_matrix_exp(torch_input);
  torch_output.backward(torch_complex_series(torch_seed_values, {2, 2}));

  require_complex_close(output.host<std::complex<double>>(),
                        torch_complex_values(torch_output), 2e-11);
  require_complex_close(input.sensitivity().host<std::complex<double>>(),
                        torch_complex_values(torch_input.grad()), 3e-10);
}

TEST_CASE("complex CPU determinant decompositions reverse match libtorch",
          "[autodiff][parity][cpu][torch][complex][linalg]") {
  const std::vector<std::complex<double>> values{
      {1.8, 0.2}, {-0.4, 0.7}, {0.3, -0.1},
      {0.5, -0.6}, {1.4, 0.3}, {-0.2, 0.8},
      {-0.1, 0.4}, {0.6, -0.5}, {1.7, 0.1}};
  const std::vector<c10::complex<double>> torch_values_in{
      {1.8, 0.2}, {-0.4, 0.7}, {0.3, -0.1},
      {0.5, -0.6}, {1.4, 0.3}, {-0.2, 0.8},
      {-0.1, 0.4}, {0.6, -0.5}, {1.7, 0.1}};

  SECTION("logdet") {
    const std::complex<double> seed{0.6, -0.35};
    Series input = complex_series(values, {3, 3});
    input.tracks_slope_(true);
    const Series output = pluss::subsp::linearalg::logdet(input);
    output.reverse(complex_series({seed}, {}));

    at::Tensor torch_input = torch_complex_series(torch_values_in, {3, 3}, true);
    const at::Tensor torch_output = at::logdet(torch_input);
    torch_output.backward(torch_complex_series(
        {c10::complex<double>{seed.real(), seed.imag()}}, {}));

    require_complex_close(output.host<std::complex<double>>(),
                          torch_complex_values(torch_output), 2e-11);
    require_complex_close(input.sensitivity().host<std::complex<double>>(),
                          torch_complex_values(torch_input.grad()), 3e-10);
  }

  SECTION("slogdet phase and log magnitude") {
    const std::complex<double> sign_seed{0.4, -0.7};
    const double logabs_seed = 0.65;
    Series input = complex_series(values, {3, 3});
    input.tracks_slope_(true);
    const Series output = pluss::subsp::linearalg::slogdet(input);
    output.reverse(complex_series({sign_seed, {logabs_seed, 0.0}}, {2}));

    at::Tensor torch_input = torch_complex_series(torch_values_in, {3, 3}, true);
    auto [torch_sign, torch_logabs] = at::linalg_slogdet(torch_input);
    torch_sign.backward(
        torch_complex_series(
            {c10::complex<double>{sign_seed.real(), sign_seed.imag()}}, {}),
        /*keep_graph=*/true);
    torch_logabs.backward(
        at::tensor(logabs_seed, at::TensorOptions().dtype(at::kDouble)));

    const auto output_values = output.host<std::complex<double>>();
    require_complex_close({output_values[0]}, torch_complex_values(torch_sign), 2e-11);
    require_close({output_values[1].real()}, torch_values(torch_logabs), 2e-11);
    require_complex_close(input.sensitivity().host<std::complex<double>>(),
                          torch_complex_values(torch_input.grad()), 3e-10);
  }
}

TEST_CASE("CPU pow reverse matches libtorch at zero bases and exponents",
          "[autodiff][parity][cpu][torch]") {
  const std::vector<double> base_values{0.0, 0.0, 2.0, 3.0};
  const std::vector<double> exponent_values{0.0, 2.0, 3.0, 0.0};

  Series base = make_series(base_values, {4}).to(ScalarType::Float64);
  Series exponent = make_series(exponent_values, {4}).to(ScalarType::Float64);
  base.tracks_slope_(true);
  exponent.tracks_slope_(true);
  const Series result = base.pow(exponent);
  result.sum().reverse();

  auto options = at::TensorOptions().dtype(at::kDouble);
  at::Tensor torch_base = at::tensor(base_values, options).set_requires_grad(true);
  at::Tensor torch_exponent = at::tensor(exponent_values, options).set_requires_grad(true);
  at::Tensor torch_result = torch_base.pow(torch_exponent);
  torch_result.sum().backward();

  require_close(result.host<double>(), torch_values(torch_result));
  require_close(base.sensitivity().host<double>(), torch_values(torch_base.grad()));
  require_close(exponent.sensitivity().host<double>(), torch_values(torch_exponent.grad()));
}

const std::vector<UnaryCase>& unary_reverse_cases() {
  static const std::vector<double> general{-0.4, 0.0, 0.2, 1.3};
  static const std::vector<double> positive{0.2, 0.7, 1.3, 2.1};
  static const std::vector<double> unit{-0.8, -0.2, 0.0, 0.7};
  static const std::vector<double> acosh_input{1.1, 1.2, 2.0, 4.0};
  static const std::vector<UnaryCase> cases{
      {"sqrt", positive, [](const Series& x) { return x.sqrt(); }, [](const at::Tensor& x) { return x.sqrt(); }},
      {"exp", general, [](const Series& x) { return x.exp(); }, [](const at::Tensor& x) { return x.exp(); }},
      {"log", positive, [](const Series& x) { return x.log(); }, [](const at::Tensor& x) { return x.log(); }},
      {"sin", general, [](const Series& x) { return x.sin(); }, [](const at::Tensor& x) { return x.sin(); }},
      {"cos", general, [](const Series& x) { return x.cos(); }, [](const at::Tensor& x) { return x.cos(); }},
      {"tanh", general, [](const Series& x) { return x.tanh(); }, [](const at::Tensor& x) { return x.tanh(); }},
      {"tan", unit, [](const Series& x) { return x.tan(); }, [](const at::Tensor& x) { return x.tan(); }},
      {"asin", unit, [](const Series& x) { return x.asin(); }, [](const at::Tensor& x) { return x.asin(); }},
      {"acos", unit, [](const Series& x) { return x.acos(); }, [](const at::Tensor& x) { return x.acos(); }},
      {"atan", general, [](const Series& x) { return x.atan(); }, [](const at::Tensor& x) { return x.atan(); }},
      {"sinh", general, [](const Series& x) { return x.sinh(); }, [](const at::Tensor& x) { return x.sinh(); }},
      {"cosh", general, [](const Series& x) { return x.cosh(); }, [](const at::Tensor& x) { return x.cosh(); }},
      {"log1p", unit, [](const Series& x) { return x.log1p(); }, [](const at::Tensor& x) { return x.log1p(); }},
      {"expm1", general, [](const Series& x) { return x.expm1(); }, [](const at::Tensor& x) { return x.expm1(); }},
      {"rsqrt", positive, [](const Series& x) { return x.rsqrt(); }, [](const at::Tensor& x) { return x.rsqrt(); }},
      {"reciprocal", positive, [](const Series& x) { return x.reciprocal(); }, [](const at::Tensor& x) { return x.reciprocal(); }},
      {"erf", general, [](const Series& x) { return x.erf(); }, [](const at::Tensor& x) { return x.erf(); }},
      {"erfc", general, [](const Series& x) { return x.erfc(); }, [](const at::Tensor& x) { return x.erfc(); }},
      {"asinh", general, [](const Series& x) { return x.asinh(); }, [](const at::Tensor& x) { return x.asinh(); }},
      {"acosh", acosh_input, [](const Series& x) { return x.acosh(); }, [](const at::Tensor& x) { return x.acosh(); }},
      {"atanh", unit, [](const Series& x) { return x.atanh(); }, [](const at::Tensor& x) { return x.atanh(); }},
      {"relu", general, [](const Series& x) { return x.relu(); }, [](const at::Tensor& x) { return x.relu(); }},
      {"sigmoid", general, [](const Series& x) { return x.sigmoid(); }, [](const at::Tensor& x) { return x.sigmoid(); }},
      {"square", general, [](const Series& x) { return x.square(); }, [](const at::Tensor& x) { return x.square(); }},
      {"gelu", general, [](const Series& x) { return x.gelu(); }, [](const at::Tensor& x) { return at::gelu(x); }},
      {"log2", positive, [](const Series& x) { return x.log2(); }, [](const at::Tensor& x) { return x.log2(); }},
      {"log10", positive, [](const Series& x) { return x.log10(); }, [](const at::Tensor& x) { return x.log10(); }},
      {"exp2", general, [](const Series& x) { return x.exp2(); }, [](const at::Tensor& x) { return x.exp2(); }},
  };
  return cases;
}

// Run one unary case at a chosen dtype and stride layout, comparing the forward
// value and the reverse gradient to libtorch. `strided` feeds a non-contiguous
// input (a transposed [2,N] leaf) so the backward is exercised on real strides,
// not just packed buffers.
void check_unary_reverse(const UnaryCase& test, ScalarType st, at::ScalarType at_st,
                         bool strided, double value_tol, double grad_tol) {
  const auto n = static_cast<std::int64_t>(test.input.size());

  Series pluss_leaf = pluss::series::undefined();
  at::Tensor torch_leaf;
  std::function<Series(const Series&)> pluss_view;
  std::function<at::Tensor(const at::Tensor&)> torch_view;

  if (!strided) {
    pluss_leaf = make_series(test.input, {n}).to(st);
    torch_leaf = at::tensor(test.input, at::TensorOptions().dtype(at::kDouble)).to(at_st);
    pluss_view = [](const Series& s) { return s; };
    torch_view = [](const at::Tensor& t) { return t; };
  } else {
    std::vector<double> doubled;
    doubled.reserve(static_cast<std::size_t>(2 * n));
    for (double v : test.input) doubled.push_back(v);
    for (double v : test.input) doubled.push_back(v * 0.5 - 0.1);
    pluss_leaf = make_series(doubled, {2, n}).to(st);
    torch_leaf = at::tensor(doubled, at::TensorOptions().dtype(at::kDouble)).to(at_st).reshape({2, n});
    pluss_view = [](const Series& s) { return s.transpose(0, 1).select(1, 0); };
    torch_view = [](const at::Tensor& t) { return t.transpose(0, 1).select(1, 0); };
  }

  pluss_leaf.tracks_slope_(true);
  const Series result = test.pluss_op(pluss_view(pluss_leaf));
  result.sum().reverse();

  torch_leaf.set_requires_grad(true);
  const at::Tensor torch_result = test.torch_op(torch_view(torch_leaf));
  torch_result.sum().backward();

  require_close(result.to(ScalarType::Float64).host<double>(), torch_values(torch_result), value_tol);
  require_close(pluss_leaf.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(torch_leaf.grad()), grad_tol);
}

TEST_CASE("generated CPU unary reverse rules match libtorch",
          "[autodiff][parity][cpu][torch][generated]") {
  for (const UnaryCase& test : unary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_unary_reverse(test, ScalarType::Float64, at::kDouble, /*strided=*/false, 2e-12, 2e-11);
    }
  }
}

void require_close_rel(const std::vector<double>& got, const std::vector<double>& expected,
                       double rel, double abs_floor) {
  REQUIRE(got.size() == expected.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO("element " << i << ": got=" << got[i] << ", ref=" << expected[i]);
    const double tol = abs_floor + rel * std::abs(expected[i]);
    REQUIRE(std::abs(got[i] - expected[i]) <= tol);
  }
}

// Reduced-precision (Float16 / BFloat16) has no trustworthy external oracle --
// libtorch's own half backward loses precision and finite differences are
// hopeless -- so the check is self-consistency: the same op's forward value and
// reverse gradient computed at the reduced dtype must agree with the Float64
// computation to within that dtype's representable precision. This catches a
// structurally wrong half backward (missing term, wrong factor, no upcast in an
// accumulation) without demanding f64 accuracy.
void check_unary_reverse_lowp(const UnaryCase& test, ScalarType st, double rel) {
  const auto n = static_cast<std::int64_t>(test.input.size());

  Series lowp_leaf = make_series(test.input, {n}).to(st);
  lowp_leaf.tracks_slope_(true);
  const Series lowp_out = test.pluss_op(lowp_leaf);
  lowp_out.sum().reverse();

  Series ref_leaf = make_series(test.input, {n}).to(ScalarType::Float64);
  ref_leaf.tracks_slope_(true);
  const Series ref_out = test.pluss_op(ref_leaf);
  ref_out.sum().reverse();

  require_close_rel(lowp_out.to(ScalarType::Float64).host<double>(),
                    ref_out.host<double>(), rel, rel);
  require_close_rel(lowp_leaf.sensitivity().to(ScalarType::Float64).host<double>(),
                    ref_leaf.sensitivity().host<double>(), rel, rel);
}

TEST_CASE("generated CPU unary reverse rules stay self-consistent in float16 and bfloat16",
          "[autodiff][parity][cpu][torch][generated][dtype]") {
  for (const UnaryCase& test : unary_reverse_cases()) {
    DYNAMIC_SECTION(test.name + " float16") {
      check_unary_reverse_lowp(test, ScalarType::Float16, 3e-2);
    }
    DYNAMIC_SECTION(test.name + " bfloat16") {
      check_unary_reverse_lowp(test, ScalarType::BFloat16, 8e-2);
    }
  }
}

TEST_CASE("generated CPU unary reverse rules match libtorch in float32",
          "[autodiff][parity][cpu][torch][generated][dtype]") {
  for (const UnaryCase& test : unary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_unary_reverse(test, ScalarType::Float32, at::kFloat, /*strided=*/false, 3e-6, 3e-5);
    }
  }
}

TEST_CASE("generated CPU unary reverse rules match libtorch on non-contiguous input",
          "[autodiff][parity][cpu][torch][generated][strided]") {
  for (const UnaryCase& test : unary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_unary_reverse(test, ScalarType::Float64, at::kDouble, /*strided=*/true, 2e-12, 2e-11);
    }
  }
}

struct BinaryReverseCase final {
  std::string name;
  std::vector<double> lhs;
  std::vector<double> rhs;
  std::function<Series(const Series&, const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&, const at::Tensor&)> torch_op;
};

const std::vector<BinaryReverseCase>& binary_reverse_cases() {
  static const std::vector<double> a{-0.6, 0.3, 1.1, -0.2};
  static const std::vector<double> pa{0.4, 0.9, 1.7, 2.3};      // strictly positive lhs
  static const std::vector<double> b{0.7, -0.4, 0.9, 1.5};
  static const std::vector<double> pb{0.5, 1.2, 0.8, 1.9};      // strictly positive rhs
  static const std::vector<double> eb{-0.3, 0.6, 1.4, 0.2};     // exponent, base>0
  static const std::vector<BinaryReverseCase> cases{
      {"add", a, b, [](const Series& x, const Series& y) { return x.add(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.add(y); }},
      {"sub", a, b, [](const Series& x, const Series& y) { return x.sub(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.sub(y); }},
      {"mul", a, b, [](const Series& x, const Series& y) { return x.mul(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.mul(y); }},
      {"div", a, pb, [](const Series& x, const Series& y) { return x.div(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.div(y); }},
      {"pow", pa, eb, [](const Series& x, const Series& y) { return x.pow(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.pow(y); }},
      {"maximum", a, b, [](const Series& x, const Series& y) { return x.maximum(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return at::maximum(x, y); }},
      {"minimum", a, b, [](const Series& x, const Series& y) { return x.minimum(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return at::minimum(x, y); }},
      {"atan2", a, pb, [](const Series& x, const Series& y) { return x.atan2(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return at::atan2(x, y); }},
      {"hypot", pa, pb, [](const Series& x, const Series& y) { return x.hypot(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return at::hypot(x, y); }},
  };
  return cases;
}

void check_binary_reverse(const BinaryReverseCase& test, ScalarType st, at::ScalarType at_st,
                          bool strided, double value_tol, double grad_tol) {
  const auto n = static_cast<std::int64_t>(test.lhs.size());

  auto make_leaf_pluss = [&](const std::vector<double>& v) {
    if (!strided) return make_series(v, {n}).to(st);
    std::vector<double> doubled(v);
    for (double e : v) doubled.push_back(e * 0.5 - 0.1);
    return make_series(doubled, {2, n}).to(st);
  };
  auto make_leaf_torch = [&](const std::vector<double>& v) {
    if (!strided) return at::tensor(v, at::TensorOptions().dtype(at::kDouble)).to(at_st);
    std::vector<double> doubled(v);
    for (double e : v) doubled.push_back(e * 0.5 - 0.1);
    return at::tensor(doubled, at::TensorOptions().dtype(at::kDouble)).to(at_st).reshape({2, n});
  };
  auto view_pluss = [&](const Series& s) {
    return strided ? s.transpose(0, 1).select(1, 0) : s;
  };
  auto view_torch = [&](const at::Tensor& t) {
    return strided ? t.transpose(0, 1).select(1, 0) : t;
  };

  Series la = make_leaf_pluss(test.lhs), lb = make_leaf_pluss(test.rhs);
  la.tracks_slope_(true);
  lb.tracks_slope_(true);
  const Series result = test.pluss_op(view_pluss(la), view_pluss(lb));
  result.sum().reverse();

  at::Tensor ta = make_leaf_torch(test.lhs), tb = make_leaf_torch(test.rhs);
  ta.set_requires_grad(true);
  tb.set_requires_grad(true);
  const at::Tensor torch_result = test.torch_op(view_torch(ta), view_torch(tb));
  torch_result.sum().backward();

  require_close(result.to(ScalarType::Float64).host<double>(), torch_values(torch_result), value_tol);
  require_close(la.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(ta.grad()), grad_tol);
  require_close(lb.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(tb.grad()), grad_tol);
}

TEST_CASE("generated CPU binary reverse rules match libtorch",
          "[autodiff][parity][cpu][torch][generated]") {
  for (const BinaryReverseCase& test : binary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_binary_reverse(test, ScalarType::Float64, at::kDouble, /*strided=*/false, 2e-12, 2e-11);
    }
  }
}

TEST_CASE("generated CPU binary reverse rules match libtorch in float32",
          "[autodiff][parity][cpu][torch][generated][dtype]") {
  for (const BinaryReverseCase& test : binary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_binary_reverse(test, ScalarType::Float32, at::kFloat, /*strided=*/false, 3e-6, 3e-5);
    }
  }
}

TEST_CASE("generated CPU binary reverse rules match libtorch on non-contiguous input",
          "[autodiff][parity][cpu][torch][generated][strided]") {
  for (const BinaryReverseCase& test : binary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_binary_reverse(test, ScalarType::Float64, at::kDouble, /*strided=*/true, 2e-12, 2e-11);
    }
  }
}

void check_binary_reverse_lowp(const BinaryReverseCase& test, ScalarType st, double rel) {
  const auto n = static_cast<std::int64_t>(test.lhs.size());
  auto run = [&](ScalarType dt, std::vector<double>& da, std::vector<double>& db) {
    Series la = make_series(test.lhs, {n}).to(dt);
    Series lb = make_series(test.rhs, {n}).to(dt);
    la.tracks_slope_(true);
    lb.tracks_slope_(true);
    const Series out = test.pluss_op(la, lb);
    out.sum().reverse();
    da = la.sensitivity().to(ScalarType::Float64).host<double>();
    db = lb.sensitivity().to(ScalarType::Float64).host<double>();
    return out.to(ScalarType::Float64).host<double>();
  };
  std::vector<double> lowp_ga, lowp_gb, ref_ga, ref_gb;
  const auto lowp_v = run(st, lowp_ga, lowp_gb);
  const auto ref_v = run(ScalarType::Float64, ref_ga, ref_gb);
  require_close_rel(lowp_v, ref_v, rel, rel);
  require_close_rel(lowp_ga, ref_ga, rel, rel);
  require_close_rel(lowp_gb, ref_gb, rel, rel);
}

TEST_CASE("generated CPU binary reverse rules stay self-consistent in float16 and bfloat16",
          "[autodiff][parity][cpu][torch][generated][dtype]") {
  for (const BinaryReverseCase& test : binary_reverse_cases()) {
    DYNAMIC_SECTION(test.name + " float16") {
      check_binary_reverse_lowp(test, ScalarType::Float16, 4e-2);
    }
    DYNAMIC_SECTION(test.name + " bfloat16") {
      check_binary_reverse_lowp(test, ScalarType::BFloat16, 1e-1);
    }
  }
}

// Reduction / cumulative / structural / indexing ops on a shaped [R,C] leaf,
// checked forward + reverse vs libtorch at a chosen dtype and stride layout.
// `strided` gives the leaf a trailing size-2 dim and operates on select(-1,0),
// so every op sees a non-contiguous input.
struct ShapedCase final {
  std::string name;
  std::function<Series(const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&)> torch_op;
  bool positive = false;  // needs a strictly-positive leaf (prod, cumprod, log-ish)
};

const std::vector<ShapedCase>& shaped_reverse_cases() {
  static const std::vector<ShapedCase> cases{
      {"sum_dim", [](const Series& x) { return x.sum(1, false); },
       [](const at::Tensor& x) { return x.sum(1); }},
      {"mean_dim", [](const Series& x) { return x.mean(1, false); },
       [](const at::Tensor& x) { return x.mean(1); }},
      {"prod_dim", [](const Series& x) { return x.prod(1, false); },
       [](const at::Tensor& x) { return x.prod(1); }, true},
      {"amax_dim", [](const Series& x) { return x.amax(1, false); },
       [](const at::Tensor& x) { return x.amax(1); }},
      {"amin_dim", [](const Series& x) { return x.amin(1, false); },
       [](const at::Tensor& x) { return x.amin(1); }},
      {"logsumexp_dim", [](const Series& x) { return x.logsumexp(1, false); },
       [](const at::Tensor& x) { return x.logsumexp(1); }},
      {"cumsum_dim", [](const Series& x) { return x.cumsum(1, false); },
       [](const at::Tensor& x) { return x.cumsum(1); }},
      {"cumprod_dim", [](const Series& x) { return x.cumprod(1, false); },
       [](const at::Tensor& x) { return x.cumprod(1); }, true},
      {"cummax_dim", [](const Series& x) { return x.cummax(1); },
       [](const at::Tensor& x) { return std::get<0>(x.cummax(1)); }},
      {"cummin_dim", [](const Series& x) { return x.cummin(1); },
       [](const at::Tensor& x) { return std::get<0>(x.cummin(1)); }},
      {"softmax_dim", [](const Series& x) { return x.softmax(1); },
       [](const at::Tensor& x) { return x.softmax(1); }},
      {"log_softmax_dim", [](const Series& x) { return x.log_softmax(1); },
       [](const at::Tensor& x) { return x.log_softmax(1); }},
      {"transpose", [](const Series& x) { return x.transpose(0, 1); },
       [](const at::Tensor& x) { return x.transpose(0, 1); }},
      {"diagonal", [](const Series& x) { return x.diagonal(0, 0, 1); },
       [](const at::Tensor& x) { return x.diagonal(0, 0, 1); }},
      {"index_select_rows", [](const Series& x) {
         return x.index_select(0, make_series(std::vector<std::int64_t>{2, 0, 2, 1}, {4}));
       },
       [](const at::Tensor& x) {
         return x.index_select(0, at::tensor(std::vector<std::int64_t>{2, 0, 2, 1},
                                             at::TensorOptions().dtype(at::kLong)));
       }},
      {"gather_cols", [](const Series& x) {
         return x.gather(1, make_series(std::vector<std::int64_t>{2, 0, 1, 2, 0, 1, 1, 2, 0}, {3, 3}));
       },
       [](const at::Tensor& x) {
         return x.gather(1, at::tensor(std::vector<std::int64_t>{2, 0, 1, 2, 0, 1, 1, 2, 0},
                                       at::TensorOptions().dtype(at::kLong)).reshape({3, 3}));
       }},
  };
  return cases;
}

void check_shaped_reverse(const ShapedCase& c, ScalarType st, at::ScalarType at_st,
                          bool strided, double value_tol, double grad_tol) {
  const std::int64_t R = 3, C = 3;
  const std::vector<double> base_pos{0.4, 1.1, 0.7, 1.9, 0.3, 2.2, 0.9, 1.5, 0.6};
  const std::vector<double> base_gen{0.4, -1.1, 0.7, -0.3, 1.2, -0.8, 0.9, -0.2, 1.5};
  const std::vector<double>& data = c.positive ? base_pos : base_gen;

  Series pluss_leaf = pluss::series::undefined();
  at::Tensor torch_leaf;
  std::function<Series(const Series&)> pv;
  std::function<at::Tensor(const at::Tensor&)> tv;

  if (!strided) {
    pluss_leaf = make_series(data, {R, C}).to(st);
    torch_leaf = at::tensor(data, at::TensorOptions().dtype(at::kDouble)).to(at_st).reshape({R, C});
    pv = [](const Series& s) { return s; };
    tv = [](const at::Tensor& t) { return t; };
  } else {
    std::vector<double> inter(data.size() * 2);
    for (std::size_t i = 0; i < data.size(); ++i) {
      inter[2 * i] = data[i];
      inter[2 * i + 1] = data[i] * 0.5 - 0.1;
    }
    pluss_leaf = make_series(inter, {R, C, 2}).to(st);
    torch_leaf = at::tensor(inter, at::TensorOptions().dtype(at::kDouble)).to(at_st).reshape({R, C, 2});
    pv = [](const Series& s) { return s.select(2, 0); };
    tv = [](const at::Tensor& t) { return t.select(2, 0); };
  }

  pluss_leaf.tracks_slope_(true);
  const Series result = c.pluss_op(pv(pluss_leaf));
  result.sum().reverse();

  torch_leaf.set_requires_grad(true);
  const at::Tensor torch_result = c.torch_op(tv(torch_leaf));
  torch_result.sum().backward();

  require_close(result.to(ScalarType::Float64).host<double>(), torch_values(torch_result), value_tol);
  require_close(pluss_leaf.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(torch_leaf.grad()), grad_tol);
}

TEST_CASE("CPU reduction/cumulative/index reverse matches libtorch",
          "[autodiff][parity][cpu][torch][generated]") {
  for (const ShapedCase& c : shaped_reverse_cases()) {
    DYNAMIC_SECTION(c.name) {
      check_shaped_reverse(c, ScalarType::Float64, at::kDouble, /*strided=*/false, 2e-12, 2e-11);
    }
  }
}

TEST_CASE("CPU reduction/cumulative/index reverse matches libtorch in float32",
          "[autodiff][parity][cpu][torch][generated][dtype]") {
  for (const ShapedCase& c : shaped_reverse_cases()) {
    DYNAMIC_SECTION(c.name) {
      check_shaped_reverse(c, ScalarType::Float32, at::kFloat, /*strided=*/false, 3e-6, 3e-5);
    }
  }
}

TEST_CASE("CPU reduction/cumulative/index reverse matches libtorch on non-contiguous input",
          "[autodiff][parity][cpu][torch][generated][strided]") {
  for (const ShapedCase& c : shaped_reverse_cases()) {
    DYNAMIC_SECTION(c.name) {
      check_shaped_reverse(c, ScalarType::Float64, at::kDouble, /*strided=*/true, 2e-12, 2e-11);
    }
  }
}

// Real-valued linear-algebra reverse across dtype and stride. The pre-existing
// linalg parity cases are all Complex128; this adds Float64 / Float32 and a
// non-contiguous layout for the same family.
struct LinalgCase final {
  std::string name;
  std::function<Series(const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&)> torch_op;
  bool spd = false;  // needs a symmetric positive-definite input
};

const std::vector<LinalgCase>& linalg_real_cases() {
  namespace la = pluss::subsp::linearalg;
  static const std::vector<LinalgCase> cases{
      {"det", [](const Series& a) { return la::det(a); },
       [](const at::Tensor& a) { return at::linalg_det(a); }},
      {"logdet", [](const Series& a) { return la::logdet(a); },
       [](const at::Tensor& a) { return at::logdet(a); }, true},
      {"inverse", [](const Series& a) { return la::inverse(a); },
       [](const at::Tensor& a) { return at::linalg_inv(a); }},
      {"pinverse", [](const Series& a) { return la::pinverse(a); },
       [](const at::Tensor& a) { return at::linalg_pinv(a); }},
      {"cholesky", [](const Series& a) { return la::cholesky(a, false); },
       [](const at::Tensor& a) { return at::linalg_cholesky(a, false); }, true},
      {"matrix_exp", [](const Series& a) { return la::matrix_exp(a); },
       [](const at::Tensor& a) { return at::linalg_matrix_exp(a); }},
      {"matrix_power3", [](const Series& a) { return la::matrix_power(a, 3); },
       [](const at::Tensor& a) { return at::linalg_matrix_power(a, 3); }},
  };
  return cases;
}

void check_linalg_real_reverse(const LinalgCase& c, ScalarType st, at::ScalarType at_st,
                               bool strided, double grad_tol) {
  const std::int64_t N = 3;
  // Well-conditioned general matrix; SPD variant = M Mᵀ + N·I.
  const std::vector<double> m{1.2, 0.3, -0.4, 0.1, 1.5, 0.2, -0.3, 0.25, 1.1};
  std::vector<double> data(m);
  if (c.spd) {
    data.assign(static_cast<std::size_t>(N * N), 0.0);
    for (std::int64_t i = 0; i < N; ++i)
      for (std::int64_t j = 0; j < N; ++j) {
        double v = (i == j) ? static_cast<double>(N) : 0.0;
        for (std::int64_t k = 0; k < N; ++k)
          v += m[static_cast<std::size_t>(i * N + k)] * m[static_cast<std::size_t>(j * N + k)];
        data[static_cast<std::size_t>(i * N + j)] = v;
      }
  }

  Series leaf = pluss::series::undefined();
  at::Tensor tleaf;
  std::function<Series(const Series&)> pv;
  std::function<at::Tensor(const at::Tensor&)> tv;
  if (!strided) {
    leaf = make_series(data, {N, N}).to(st);
    tleaf = at::tensor(data, at::TensorOptions().dtype(at::kDouble)).to(at_st).reshape({N, N});
    pv = [](const Series& s) { return s; };
    tv = [](const at::Tensor& t) { return t; };
  } else {
    std::vector<double> inter(data.size() * 2);
    for (std::size_t i = 0; i < data.size(); ++i) { inter[2 * i] = data[i]; inter[2 * i + 1] = 0.0; }
    leaf = make_series(inter, {N, N, 2}).to(st);
    tleaf = at::tensor(inter, at::TensorOptions().dtype(at::kDouble)).to(at_st).reshape({N, N, 2});
    pv = [](const Series& s) { return s.select(2, 0); };
    tv = [](const at::Tensor& t) { return t.select(2, 0); };
  }

  leaf.tracks_slope_(true);
  c.pluss_op(pv(leaf)).sum().reverse();

  tleaf.set_requires_grad(true);
  at::Tensor tout = c.torch_op(tv(tleaf));
  tout.sum().backward();

  require_close(leaf.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(tleaf.grad()), grad_tol);
}

TEST_CASE("CPU linalg reverse matches libtorch in float64 and float32",
          "[autodiff][parity][cpu][torch][linalg][dtype]") {
  for (const LinalgCase& c : linalg_real_cases()) {
    DYNAMIC_SECTION(c.name + " float64") {
      check_linalg_real_reverse(c, ScalarType::Float64, at::kDouble, false, 2e-9);
    }
    DYNAMIC_SECTION(c.name + " float32") {
      check_linalg_real_reverse(c, ScalarType::Float32, at::kFloat, false, 3e-3);
    }
  }
}

TEST_CASE("CPU linalg reverse matches libtorch on non-contiguous input",
          "[autodiff][parity][cpu][torch][linalg][strided]") {
  for (const LinalgCase& c : linalg_real_cases()) {
    DYNAMIC_SECTION(c.name) {
      check_linalg_real_reverse(c, ScalarType::Float64, at::kDouble, true, 2e-9);
    }
  }
}

// ---------------------------------------------------------------------------
// Pass: batched tensors. Everything checked so far is unbatched (a single
// matrix / a single row); real training runs a leading batch dimension through
// matmul, linalg, and multi-axis reductions with broadcasting across it.
// ---------------------------------------------------------------------------

void check_linalg_batched_reverse(const LinalgCase& c, double grad_tol) {
  const std::int64_t B = 2, N = 3;
  const std::vector<double> m0{1.2, 0.3, -0.4, 0.1, 1.5, 0.2, -0.3, 0.25, 1.1};
  const std::vector<double> m1{0.9, -0.2, 0.15, 0.4, 1.3, -0.25, 0.1, 0.2, 1.4};
  std::vector<double> data;
  data.reserve(static_cast<std::size_t>(B * N * N));
  for (const auto& m : {m0, m1}) {
    if (!c.spd) {
      data.insert(data.end(), m.begin(), m.end());
      continue;
    }
    for (std::int64_t i = 0; i < N; ++i)
      for (std::int64_t j = 0; j < N; ++j) {
        double v = (i == j) ? static_cast<double>(N) : 0.0;
        for (std::int64_t k = 0; k < N; ++k)
          v += m[static_cast<std::size_t>(i * N + k)] * m[static_cast<std::size_t>(j * N + k)];
        data.push_back(v);
      }
  }

  Series leaf = make_series(data, {B, N, N}).to(ScalarType::Float64);
  leaf.tracks_slope_(true);
  c.pluss_op(leaf).sum().reverse();

  at::Tensor tleaf =
      at::tensor(data, at::TensorOptions().dtype(at::kDouble)).reshape({B, N, N}).set_requires_grad(true);
  c.torch_op(tleaf).sum().backward();

  require_close(leaf.sensitivity().host<double>(), torch_values(tleaf.grad()), grad_tol);
}

TEST_CASE("CPU batched linalg reverse matches libtorch", "[autodiff][parity][cpu][torch][linalg][batched]") {
  for (const LinalgCase& c : linalg_real_cases()) {
    DYNAMIC_SECTION(c.name) { check_linalg_batched_reverse(c, 2e-9); }
  }
}

TEST_CASE("CPU batched matmul chain reverse matches libtorch",
          "[autodiff][parity][cpu][torch][batched]") {
  // [B, M, K] @ [B, K, N] + bias, broadcasting a single [K, N] weight and a
  // [N] bias across the batch -- the shape every dense layer forward uses.
  const std::int64_t B = 3, M = 2, K = 4, N = 3;
  const auto xv = spectral_signal(B * M * K, 0x1111);
  const auto wv = spectral_signal(K * N, 0x2222);
  const auto bv = spectral_signal(N, 0x3333);

  Series x = make_series(xv, {B, M, K}).to(ScalarType::Float64);
  Series w = make_series(wv, {K, N}).to(ScalarType::Float64);
  Series b = make_series(bv, {N}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  w.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series out = x.matmul(w).add(b).tanh();
  out.sum().reverse();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor tx = at::tensor(xv, opt).reshape({B, M, K}).set_requires_grad(true);
  at::Tensor tw = at::tensor(wv, opt).reshape({K, N}).set_requires_grad(true);
  at::Tensor tb = at::tensor(bv, opt).reshape({N}).set_requires_grad(true);
  const at::Tensor tout = tx.matmul(tw).add(tb).tanh();
  tout.sum().backward();

  require_close(out.host<double>(), torch_values(tout), 2e-11);
  require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-10);
  require_close(w.sensitivity().host<double>(), torch_values(tw.grad()), 2e-10);
  require_close(b.sensitivity().host<double>(), torch_values(tb.grad()), 2e-10);
}

// Multi-axis reduction (`x.sum({1,2}, keepdim)` etc.) -- added to close the gap
// the batched-tensor pass above flagged. Implemented as a fold of the already-
// verified single-dim reduction, so this proves the composition (dim order,
// the keepdim squeeze pass) rather than a new kernel.
TEST_CASE("CPU multi-axis reduction reverse matches libtorch",
          "[autodiff][parity][cpu][torch][batched]") {
  const std::int64_t B = 2, C = 3, L = 4;
  const auto xv = spectral_signal(B * C * L, 0x4444);
  struct Cfg { std::string name; std::vector<std::int64_t> dims; bool keepdim; };
  const std::vector<Cfg> cfgs{
      {"sum_BC_keep", {0, 1}, true},
      {"mean_CL_nokeep", {1, 2}, false},
      {"amax_all_nokeep", {0, 1, 2}, false},
      {"amin_negdims_keep", {-1, -3}, true},
  };
  for (const Cfg& cfg : cfgs) {
    DYNAMIC_SECTION(cfg.name) {
      Series x = make_series(xv, {B, C, L}).to(ScalarType::Float64);
      x.tracks_slope_(true);
      Series y = cfg.name.substr(0, 3) == "sum"   ? x.sum(cfg.dims, cfg.keepdim)
                : cfg.name.substr(0, 4) == "mean" ? x.mean(cfg.dims, cfg.keepdim)
                : cfg.name.substr(0, 4) == "amax" ? x.amax(cfg.dims, cfg.keepdim)
                                                   : x.amin(cfg.dims, cfg.keepdim);
      y.mul(y).sum().reverse();

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble))
                          .reshape({B, C, L}).set_requires_grad(true);
      at::Tensor ty = cfg.name.substr(0, 3) == "sum"   ? tx.sum(cfg.dims, cfg.keepdim)
                     : cfg.name.substr(0, 4) == "mean" ? tx.mean(cfg.dims, cfg.keepdim)
                     : cfg.name.substr(0, 4) == "amax" ? tx.amax(cfg.dims, cfg.keepdim)
                                                        : tx.amin(cfg.dims, cfg.keepdim);
      ty.mul(ty).sum().backward();

      require_close(y.host<double>(), torch_values(ty), 2e-10);
      require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-9);
    }
  }
}

// ---------------------------------------------------------------------------
// Track B verification: fusing softmax/log_softmax/logsumexp/silu/gelu/
// leaky_relu into a single analytic reverse node (from the prior multi-node
// composition) must not break double-backward (`create_graph`/
// `track_higher_order`). Each fused backward recomputes its needed
// intermediate (e.g. `softmax(a, dim)` inside softmax's own reverse) as a
// fresh, *tracked* call rather than closing over a detached forward value --
// this checks that choice actually keeps the graph connected for a second
// differentiation, against libtorch's own double-backward as ground truth.
// ---------------------------------------------------------------------------
TEST_CASE("CPU fused-activation double-backward matches libtorch",
          "[autodiff][parity][cpu][torch][doublebackward]") {
  // leaky_relu is deliberately excluded here: its first derivative is
  // piecewise-constant, so its second derivative is exactly zero almost
  // everywhere and the incoming cotangent for x's gradient carries no
  // dependency on x at all (the comparison that picks the slope branch is
  // not itself differentiable). That is correct, verified behavior -- see
  // the finite-difference sweep and the plain single-order parity case for
  // leaky_relu -- it just isn't a case with any curvature to double-check.
  const std::vector<double> xv{-0.9, -0.2, 0.15, 0.6, 1.3, -1.4};
  const std::vector<double> seed_v{0.4, -0.3, 0.7, -0.5, 0.2, 0.6};
  const std::vector<double> seed_reduced_v{0.4, -0.3};

  struct Case {
    std::string name;
    std::function<Series(const Series&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
    bool reduces_dim1 = false;
  };
  const std::vector<Case> cases{
      {"softmax", [](const Series& x) { return x.reshape({2, 3}).softmax(1); },
       [](const at::Tensor& x) { return x.reshape({2, 3}).softmax(1); }, false},
      {"log_softmax", [](const Series& x) { return x.reshape({2, 3}).log_softmax(1); },
       [](const at::Tensor& x) { return x.reshape({2, 3}).log_softmax(1); }, false},
      {"logsumexp", [](const Series& x) { return x.reshape({2, 3}).logsumexp(1, false); },
       [](const at::Tensor& x) { return x.reshape({2, 3}).logsumexp(1, false); }, true},
      {"silu", [](const Series& x) { return x.silu(); },
       [](const at::Tensor& x) { return at::silu(x); }, false},
      {"gelu_exact", [](const Series& x) { return x.gelu(); },
       [](const at::Tensor& x) { return at::gelu(x); }, false},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      const bool elementwise = (c.name == "silu" || c.name == "gelu_exact");
      Series x = make_series(xv, {6}).to(ScalarType::Float64);
      x.tracks_slope_(true);
      const Series seed = make_series(c.reduces_dim1 ? seed_reduced_v : seed_v,
                                       c.reduces_dim1 ? std::vector<std::int64_t>{2}
                                                      : std::vector<std::int64_t>{2, 3})
                               .to(ScalarType::Float64);
      Series y = c.pop(x);
      if (elementwise) {
        y.sum().reverse(/*keep_graph=*/true, /*accumulate=*/false, /*track_higher_order=*/true);
      } else {
        y.reverse(seed, /*keep_graph=*/true, /*accumulate=*/false, /*track_higher_order=*/true);
      }
      REQUIRE(x.has_sensitivity());
      Series g1 = x.sensitivity();
      REQUIRE(g1.tracks_slope());
      x.clear();
      g1.sum().reverse();
      REQUIRE(x.has_sensitivity());
      const auto got_g2 = x.sensitivity().host<double>();

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
      at::Tensor ty = c.top(tx);
      if (elementwise) {
        ty.sum().backward({}, /*retain_graph=*/true, /*create_graph=*/true);
      } else {
        at::Tensor tseed = at::tensor(c.reduces_dim1 ? seed_reduced_v : seed_v,
                                       at::TensorOptions().dtype(at::kDouble))
                               .reshape(c.reduces_dim1 ? std::vector<int64_t>{2} : std::vector<int64_t>{2, 3});
        ty.backward(tseed, /*retain_graph=*/true, /*create_graph=*/true);
      }
      at::Tensor tg1 = tx.grad();
      REQUIRE(tg1.defined());
      tx.mutable_grad() = at::Tensor();
      tg1.sum().backward();
      at::Tensor tg2 = tx.grad();

      require_close(got_g2, torch_values(tg2), 2e-7);
    }
  }
}

// ---------------------------------------------------------------------------
// Pass: binary-op broadcasting edge cases. binary_reverse_cases() above only
// exercises same-shape 1D operands; broadcasting reduction across mismatched
// ranks and interior size-1 dims is a classic autodiff bug source (the
// reverse must sum the cotangent back down to each operand's original shape).
// ---------------------------------------------------------------------------

struct BroadcastCase final {
  std::string name;
  std::vector<std::int64_t> lhs_shape;
  std::vector<std::int64_t> rhs_shape;
};

const std::vector<BroadcastCase>& broadcast_cases() {
  static const std::vector<BroadcastCase> cases{
      {"trailing_dim", {3, 4}, {4}},
      {"scalar_rhs", {3, 4}, {1}},
      {"leading_singleton", {1, 3, 4}, {2, 3, 4}},
      {"middle_singleton_both_sides", {5, 1, 4}, {1, 3, 1}},
      {"reduce_two_axes", {2, 3, 4}, {1, 3, 1}},
  };
  return cases;
}

void check_broadcast_reverse(const BroadcastCase& c,
                             const std::function<Series(const Series&, const Series&)>& pop,
                             const std::function<at::Tensor(const at::Tensor&, const at::Tensor&)>& top,
                             double tol) {
  const auto lhs_n = std::accumulate(c.lhs_shape.begin(), c.lhs_shape.end(), std::int64_t{1},
                                     std::multiplies<>());
  const auto rhs_n = std::accumulate(c.rhs_shape.begin(), c.rhs_shape.end(), std::int64_t{1},
                                     std::multiplies<>());
  const auto lv = spectral_signal(lhs_n, 0x5A5A);
  auto rv = spectral_signal(rhs_n, 0xA5A5);
  for (auto& v : rv) v += (v >= 0 ? 1.0 : -1.0) * 0.6;  // keep away from 0 for div

  Series a = make_series(lv, c.lhs_shape).to(ScalarType::Float64);
  Series b = make_series(rv, c.rhs_shape).to(ScalarType::Float64);
  a.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series out = pop(a, b);
  out.sum().reverse();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor ta = at::tensor(lv, opt).reshape(c.lhs_shape).set_requires_grad(true);
  at::Tensor tb = at::tensor(rv, opt).reshape(c.rhs_shape).set_requires_grad(true);
  const at::Tensor tout = top(ta, tb);
  tout.sum().backward();

  require_close(out.host<double>(), torch_values(tout), tol);
  require_close(a.sensitivity().host<double>(), torch_values(ta.grad()), tol);
  require_close(b.sensitivity().host<double>(), torch_values(tb.grad()), tol);
}

TEST_CASE("CPU binary broadcasting reverse matches libtorch across shape combinations",
          "[autodiff][parity][cpu][torch][broadcast]") {
  struct Op {
    std::string name;
    std::function<Series(const Series&, const Series&)> pop;
    std::function<at::Tensor(const at::Tensor&, const at::Tensor&)> top;
  };
  const std::vector<Op> ops{
      {"mul", [](const Series& x, const Series& y) { return x.mul(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.mul(y); }},
      {"div", [](const Series& x, const Series& y) { return x.div(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.div(y); }},
      {"add", [](const Series& x, const Series& y) { return x.add(y); },
       [](const at::Tensor& x, const at::Tensor& y) { return x.add(y); }},
  };
  for (const Op& op : ops) {
    for (const BroadcastCase& c : broadcast_cases()) {
      DYNAMIC_SECTION(op.name + " " + c.name) {
        check_broadcast_reverse(c, op.pop, op.top, 2e-10);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Pass: QR / SVD / triangular_solve in Float32. The pre-existing coverage for
// these decompositions is Complex128 only; real Float64/Float32 is untested.
// QR uses Q and R directly (both algorithms use the same Householder sign
// convention here, already proven equal in the complex case); SVD checks only
// the singular values `s`, which have a gauge-free gradient (U/V columns are
// only defined up to a sign/phase, so their raw gradients are not comparable
// without the seed tricks the complex SVD test uses).
// ---------------------------------------------------------------------------

TEST_CASE("CPU QR reverse matches libtorch in float64 and float32",
          "[autodiff][parity][cpu][torch][linalg][dtype]") {
  const std::vector<double> values{1.2, -0.3, 0.5, 1.7, -0.8, 0.9};  // [3,2]
  for (const bool q_path : {true, false}) {
    for (const auto& dt : {std::pair{ScalarType::Float64, at::kDouble}, {ScalarType::Float32, at::kFloat}}) {
      DYNAMIC_SECTION((q_path ? "Q " : "R ") << (dt.first == ScalarType::Float64 ? "float64" : "float32")) {
        Series input = make_series(values, {3, 2}).to(dt.first);
        input.tracks_slope_(true);
        const Series output =
            q_path ? pluss::subsp::linearalg::qr_q(input, true) : pluss::subsp::linearalg::qr_r(input, true);
        output.sum().reverse();

        at::Tensor torch_input =
            at::tensor(values, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({3, 2}).set_requires_grad(true);
        auto [torch_q, torch_r] = at::linalg_qr(torch_input, "reduced");
        (q_path ? torch_q : torch_r).sum().backward();

        require_close(input.sensitivity().to(ScalarType::Float64).host<double>(),
                      torch_values(torch_input.grad()), dt.first == ScalarType::Float64 ? 2e-9 : 3e-3);
      }
    }
  }
}

TEST_CASE("CPU SVD singular-value reverse matches libtorch in float64 and float32",
          "[autodiff][parity][cpu][torch][linalg][dtype]") {
  const std::vector<double> values{2.0, -0.4, 0.7, 1.4, -0.6, 0.2};  // [3,2]
  for (const auto& dt : {std::pair{ScalarType::Float64, at::kDouble}, {ScalarType::Float32, at::kFloat}}) {
    DYNAMIC_SECTION((dt.first == ScalarType::Float64 ? "float64" : "float32")) {
      Series input = make_series(values, {3, 2}).to(dt.first);
      input.tracks_slope_(true);
      const Series s = pluss::subsp::linearalg::svd_s(input, false);
      s.sum().reverse();

      at::Tensor torch_input =
          at::tensor(values, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({3, 2}).set_requires_grad(true);
      auto [torch_u, torch_s, torch_vh] = at::linalg_svd(torch_input, false);
      torch_s.sum().backward();

      require_close(s.to(ScalarType::Float64).host<double>(), torch_values(torch_s),
                    dt.first == ScalarType::Float64 ? 2e-10 : 3e-4);
      require_close(input.sensitivity().to(ScalarType::Float64).host<double>(),
                    torch_values(torch_input.grad()), dt.first == ScalarType::Float64 ? 2e-9 : 3e-3);
    }
  }
}

TEST_CASE("CPU triangular_solve reverse matches libtorch in float64 and float32",
          "[autodiff][parity][cpu][torch][linalg][dtype]") {
  // Lower-triangular factor (positive diagonal) solving factor @ X = rhs.
  const std::vector<double> factor_v{2.0, 0.0, 0.0, 0.5, 1.5, 0.0, -0.3, 0.4, 1.2};  // [3,3]
  const std::vector<double> rhs_v{1.0, 0.5, -0.2, 0.3, 1.1, 0.4};                    // [3,2]
  for (const auto& dt : {std::pair{ScalarType::Float64, at::kDouble}, {ScalarType::Float32, at::kFloat}}) {
    DYNAMIC_SECTION((dt.first == ScalarType::Float64 ? "float64" : "float32")) {
      Series factor = make_series(factor_v, {3, 3}).to(dt.first);
      Series rhs = make_series(rhs_v, {3, 2}).to(dt.first);
      factor.tracks_slope_(true);
      rhs.tracks_slope_(true);
      const Series x = pluss::subsp::linearalg::triangular_solve(factor, rhs, /*upper=*/false, /*unitriangular=*/false);
      x.sum().reverse();

      auto opt = at::TensorOptions().dtype(at::kDouble);
      at::Tensor tfactor = at::tensor(factor_v, opt).to(dt.second).reshape({3, 3}).set_requires_grad(true);
      at::Tensor trhs = at::tensor(rhs_v, opt).to(dt.second).reshape({3, 2}).set_requires_grad(true);
      const at::Tensor tx = at::linalg_solve_triangular(tfactor, trhs, /*upper=*/false);
      tx.sum().backward();

      const double tol = dt.first == ScalarType::Float64 ? 2e-9 : 3e-3;
      require_close(x.to(ScalarType::Float64).host<double>(), torch_values(tx), tol);
      require_close(factor.sensitivity().to(ScalarType::Float64).host<double>(),
                    torch_values(tfactor.grad()), tol);
      require_close(rhs.sensitivity().to(ScalarType::Float64).host<double>(),
                    torch_values(trhs.grad()), tol);
    }
  }
}

// ---------------------------------------------------------------------------
// Pass: normalization / loss / embedding chains, checked end-to-end -- value
// and gradient (and, for layer_norm, double-backward) -- against libtorch's
// actual fused ops. This is the pattern PlussAI training code hits directly;
// layer_norm now has its own fused single-node reverse (see Series::layer_norm
// in OpsInternal.cpp), cross-entropy/embedding are still hand-composed from
// Series primitives (pluss ships no fused kernel for either yet).
// ---------------------------------------------------------------------------

TEST_CASE("CPU layer-norm chain matches libtorch value and reverse",
          "[autodiff][parity][cpu][torch][training]") {
  const std::int64_t B = 2, D = 4;
  const double eps = 1e-5;
  const auto xv = spectral_signal(B * D, 0x7777);
  const auto gv = spectral_signal(D, 0x8888);
  const auto bv = spectral_signal(D, 0x9999);

  Series x = make_series(xv, {B, D}).to(ScalarType::Float64);
  Series g = make_series(gv, {D}).to(ScalarType::Float64);
  Series b = make_series(bv, {D}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  g.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series out = x.layer_norm(std::vector<std::int64_t>{D}, g, b, eps);
  out.sum().reverse();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor tx = at::tensor(xv, opt).reshape({B, D}).set_requires_grad(true);
  at::Tensor tg = at::tensor(gv, opt).reshape({D}).set_requires_grad(true);
  at::Tensor tb = at::tensor(bv, opt).reshape({D}).set_requires_grad(true);
  const at::Tensor tout = at::layer_norm(tx, {D}, tg, tb, eps);
  tout.sum().backward();

  require_close(out.host<double>(), torch_values(tout), 2e-9);
  require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-8);
  require_close(g.sensitivity().host<double>(), torch_values(tg.grad()), 2e-8);
  require_close(b.sensitivity().host<double>(), torch_values(tb.grad()), 2e-8);
}

// The fused layer_norm's weight/bias gradients reduce over EVERY leading
// (batch) dim, not just one -- a 3D [B,T,D] input (the actual transformer
// shape: batch x sequence x feature, normalized over the last axis) is the
// real test of that reduction, not the 2D case above.
TEST_CASE("CPU batched layer-norm reverse matches libtorch across leading dims",
          "[autodiff][parity][cpu][torch][training]") {
  const std::int64_t B = 2, T = 3, D = 4;
  const double eps = 1e-5;
  const auto xv = spectral_signal(B * T * D, 0x1234);
  const auto gv = spectral_signal(D, 0x5678);
  const auto bv = spectral_signal(D, 0x9abc);

  Series x = make_series(xv, {B, T, D}).to(ScalarType::Float64);
  Series g = make_series(gv, {D}).to(ScalarType::Float64);
  Series b = make_series(bv, {D}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  g.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series out = x.layer_norm(std::vector<std::int64_t>{D}, g, b, eps);
  out.mul(out).sum().reverse();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor tx = at::tensor(xv, opt).reshape({B, T, D}).set_requires_grad(true);
  at::Tensor tg = at::tensor(gv, opt).reshape({D}).set_requires_grad(true);
  at::Tensor tb = at::tensor(bv, opt).reshape({D}).set_requires_grad(true);
  const at::Tensor tout = at::layer_norm(tx, {D}, tg, tb, eps);
  tout.mul(tout).sum().backward();

  require_close(out.host<double>(), torch_values(tout), 2e-9);
  require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-8);
  require_close(g.sensitivity().host<double>(), torch_values(tg.grad()), 2e-8);
  require_close(b.sensitivity().host<double>(), torch_values(tb.grad()), 2e-8);
}

TEST_CASE("CPU layer-norm double-backward matches libtorch",
          "[autodiff][parity][cpu][torch][training][doublebackward]") {
  const std::int64_t B = 2, D = 4;
  const double eps = 1e-5;
  const auto xv = spectral_signal(B * D, 0xdead);
  const auto gv = spectral_signal(D, 0xbeef);
  const auto bv = spectral_signal(D, 0xf00d);
  const auto seed_v = spectral_signal(B * D, 0xc0de);

  Series x = make_series(xv, {B, D}).to(ScalarType::Float64);
  Series g = make_series(gv, {D}).to(ScalarType::Float64);
  Series b = make_series(bv, {D}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  g.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series seed = make_series(seed_v, {B, D}).to(ScalarType::Float64);
  Series out = x.layer_norm(std::vector<std::int64_t>{D}, g, b, eps);
  out.reverse(seed, /*keep_graph=*/true, /*accumulate=*/false, /*track_higher_order=*/true);

  REQUIRE(x.has_sensitivity());
  REQUIRE(g.has_sensitivity());
  REQUIRE(b.has_sensitivity());
  Series gx1 = x.sensitivity();
  Series gg1 = g.sensitivity();
  Series gb1 = b.sensitivity();
  REQUIRE(gx1.tracks_slope());
  x.clear();
  g.clear();
  b.clear();
  gx1.sum().add(gg1.sum()).add(gb1.sum()).reverse();
  REQUIRE(x.has_sensitivity());
  const auto got_gx2 = x.sensitivity().host<double>();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor tx = at::tensor(xv, opt).reshape({B, D}).set_requires_grad(true);
  at::Tensor tg = at::tensor(gv, opt).reshape({D}).set_requires_grad(true);
  at::Tensor tb = at::tensor(bv, opt).reshape({D}).set_requires_grad(true);
  at::Tensor tseed = at::tensor(seed_v, opt).reshape({B, D});
  at::Tensor tout = at::layer_norm(tx, {D}, tg, tb, eps);
  tout.backward(tseed, /*retain_graph=*/true, /*create_graph=*/true);
  at::Tensor tgx1 = tx.grad();
  at::Tensor tgg1 = tg.grad();
  at::Tensor tgb1 = tb.grad();
  REQUIRE(tgx1.defined());
  tx.mutable_grad() = at::Tensor();
  tg.mutable_grad() = at::Tensor();
  tb.mutable_grad() = at::Tensor();
  tgx1.sum().add(tgg1.sum()).add(tgb1.sum()).backward();
  at::Tensor tgx2 = tx.grad();

  require_close(got_gx2, torch_values(tgx2), 2e-6);
}

// Series::elu (Track B: was previously only reachable as a 7-node
// where/mul/sub/exp composite behind the untyped invoke("elu", ...) string
// dispatch -- see OpsInternal.cpp) is also the shared implementation behind
// celu/selu now, so all three are checked here against their own libtorch
// ops. elu has genuine curvature in its negative branch (unlike leaky_relu),
// so its double-backward is a real check, not a degenerate zero case.
TEST_CASE("CPU elu family value/reverse/double-backward matches libtorch",
          "[autodiff][parity][cpu][torch][doublebackward]") {
  const std::vector<double> xv{-1.7, -0.4, -0.05, 0.2, 0.9, 2.3};
  const std::vector<double> seed_v{0.3, -0.6, 0.5, 0.8, -0.2, 0.4};

  struct Case {
    std::string name;
    std::function<Series(const Series&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
  };
  const std::vector<Case> cases{
      {"elu_default", [](const Series& x) { return x.elu(); },
       [](const at::Tensor& x) { return at::elu(x); }},
      {"elu_alpha", [](const Series& x) { return x.elu(0.7); },
       [](const at::Tensor& x) { return at::elu(x, 0.7); }},
      {"elu_full", [](const Series& x) { return x.elu(1.3, 0.8, 1.1); },
       [](const at::Tensor& x) { return at::elu(x, 1.3, 0.8, 1.1); }},
      {"celu", [](const Series& x) { return pluss::codem::_compat::invoke("celu", x, {0.6}); },
       [](const at::Tensor& x) { return at::celu(x, 0.6); }},
      {"selu", [](const Series& x) { return pluss::codem::_compat::invoke("selu", x); },
       [](const at::Tensor& x) { return at::selu(x); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      Series x = make_series(xv, {6}).to(ScalarType::Float64);
      x.tracks_slope_(true);
      const Series seed = make_series(seed_v, {6}).to(ScalarType::Float64);
      Series y = c.pop(x);
      y.reverse(seed, /*keep_graph=*/true, /*accumulate=*/false, /*track_higher_order=*/true);
      REQUIRE(x.has_sensitivity());
      Series g1 = x.sensitivity();
      REQUIRE(g1.tracks_slope());
      x.clear();
      g1.sum().reverse();
      REQUIRE(x.has_sensitivity());
      const auto got_g2 = x.sensitivity().host<double>();

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
      at::Tensor ty = c.top(tx);
      at::Tensor tseed = at::tensor(seed_v, at::TensorOptions().dtype(at::kDouble));
      ty.backward(tseed, /*retain_graph=*/true, /*create_graph=*/true);
      at::Tensor tg1 = tx.grad();
      REQUIRE(tg1.defined());

      require_close(y.host<double>(), torch_values(ty), 2e-10);
      require_close(g1.host<double>(), torch_values(tg1), 2e-9);

      tx.mutable_grad() = at::Tensor();
      tg1.sum().backward();
      at::Tensor tg2 = tx.grad();
      require_close(got_g2, torch_values(tg2), 2e-8);
    }
  }
}

// hardsigmoid/hardswish/mish (Track B): were previously only reachable via
// the untyped invoke("hardsigmoid", ...) string dispatch (see
// hardsigmoid_impl_/hardswish_impl_/mish_impl_ in Invoke.cpp), now single
// fused analytic nodes with real typed Series members. hardsigmoid's
// interior derivative is a piecewise CONSTANT (1/6) -- like leaky_relu, its
// true second derivative is zero a.e. and its backward cotangent carries no
// differentiable dependency on x, so it's checked for value/first-order only
// (no double-backward, that would be a degenerate zero case, not a bug).
// hardswish's interior derivative (2x+3)/6 is genuinely x-dependent, and
// mish is fully smooth -- both have real curvature and get the same
// libtorch double-backward proof as the other Track B activations.
TEST_CASE("CPU hardsigmoid value/reverse matches libtorch",
          "[autodiff][parity][cpu][torch]") {
  const std::vector<double> xv{-4.5, -3.0, -2.1, -0.3, 0.0, 0.8, 2.9, 3.0, 4.2};
  Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series y = x.hardsigmoid();
  y.sum().reverse();

  at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
  const at::Tensor ty = at::hardsigmoid(tx);
  ty.sum().backward();

  require_close(y.host<double>(), torch_values(ty), 2e-12);
  require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-12);
}

TEST_CASE("CPU hardswish/mish value/reverse/double-backward matches libtorch",
          "[autodiff][parity][cpu][torch][doublebackward]") {
  // Deliberately avoids x == +-3 exactly: hardswish's first derivative has a
  // genuine kink there (by design -- see the comment above), so its second
  // derivative is ill-defined at that single point; everywhere else on this
  // sample it's the smooth interior/exterior piece.
  const std::vector<double> xv{-4.5, -3.4, -2.1, -0.3, 0.2, 0.8, 2.6, 3.4, 4.2};
  const std::vector<double> seed_v{0.4, -0.3, 0.7, -0.5, 0.2, 0.6, -0.8, 0.3, -0.1};

  struct Case {
    std::string name;
    std::function<Series(const Series&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
  };
  const std::vector<Case> cases{
      {"hardswish", [](const Series& x) { return x.hardswish(); },
       [](const at::Tensor& x) { return at::hardswish(x); }},
      {"mish", [](const Series& x) { return x.mish(); },
       [](const at::Tensor& x) { return at::mish(x); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
      x.tracks_slope_(true);
      const Series seed = make_series(seed_v, {static_cast<std::int64_t>(seed_v.size())}).to(ScalarType::Float64);
      Series y = c.pop(x);
      y.reverse(seed, /*keep_graph=*/true, /*accumulate=*/false, /*track_higher_order=*/true);
      REQUIRE(x.has_sensitivity());
      Series g1 = x.sensitivity();
      REQUIRE(g1.tracks_slope());
      x.clear();
      g1.sum().reverse();
      REQUIRE(x.has_sensitivity());
      const auto got_g2 = x.sensitivity().host<double>();

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
      at::Tensor ty = c.top(tx);
      at::Tensor tseed = at::tensor(seed_v, at::TensorOptions().dtype(at::kDouble));
      ty.backward(tseed, /*retain_graph=*/true, /*create_graph=*/true);
      at::Tensor tg1 = tx.grad();
      REQUIRE(tg1.defined());

      require_close(y.host<double>(), torch_values(ty), 2e-10);
      require_close(g1.host<double>(), torch_values(tg1), 2e-9);

      tx.mutable_grad() = at::Tensor();
      tg1.sum().backward();
      at::Tensor tg2 = tx.grad();
      require_close(got_g2, torch_values(tg2), 2e-8);
    }
  }
}

// threshold/rrelu (Track B): threshold was already effectively a single
// tape node via where() (value is a plain constant, no grad), now it has
// its own named node and a typed Series member instead of the generic
// binary where() path. Its derivative is piecewise-CONSTANT (1 or 0), same
// zero-curvature story as leaky_relu, so no double-backward case here.
TEST_CASE("CPU threshold value/reverse matches libtorch",
          "[autodiff][parity][cpu][torch]") {
  const std::vector<double> xv{-2.1, -0.4, -0.05, 0.0, 0.2, 0.9, 2.3};
  Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series y = x.threshold(0.0, -1.0);
  y.sum().reverse();

  at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
  const at::Tensor ty = at::threshold(tx, 0.0, -1.0);
  ty.sum().backward();

  require_close(y.host<double>(), torch_values(ty), 2e-12);
  require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-12);
}

// rrelu's random branch can't be checked bit-for-bit against libtorch (the
// two RNG streams differ), so: (a) eval mode (training=false, the fixed-
// midpoint-slope branch that delegates to the already-fused leaky_relu) is
// checked exactly against libtorch, and (b) training mode is checked for
// internal self-consistency -- for every negative-input element the forward
// output IS `x * realized_slope`, so the realized slope is recoverable as
// `y/x` and the analytic backward must reproduce exactly that value on
// those elements (and exactly 1 on positive elements) -- without needing to
// know or reproduce the actual random draw.
TEST_CASE("CPU rrelu eval mode matches libtorch; training mode is self-consistent",
          "[autodiff][parity][cpu][torch]") {
  const std::vector<double> xv{-2.1, -0.9, -0.4, -0.05, 0.2, 0.9, 2.3};

  SECTION("eval mode (training=false) matches libtorch exactly") {
    Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.rrelu(0.2, 0.8, false);
    y.sum().reverse();

    at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
    const at::Tensor ty = at::rrelu(tx, 0.2, 0.8, false);
    ty.sum().backward();

    require_close(y.host<double>(), torch_values(ty), 2e-12);
    require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-12);
  }

  SECTION("training mode reproduces its own realized per-element slope in backward") {
    Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.rrelu(0.2, 0.8, true);
    y.sum().reverse();

    const auto xs = x.host<double>();
    const auto ys = y.host<double>();
    const auto got_g = x.sensitivity().host<double>();
    std::vector<double> expected_g(xs.size());
    for (std::size_t i = 0; i < xs.size(); ++i) {
      if (xs[i] > 0.0) {
        expected_g[i] = 1.0;
        REQUIRE(ys[i] == Approx(xs[i]));
      } else {
        const double slope = ys[i] / xs[i];
        REQUIRE(slope >= 0.2 - 1e-9);
        REQUIRE(slope <= 0.8 + 1e-9);
        expected_g[i] = slope;
      }
    }
    require_close(got_g, expected_g, 1e-10);
  }
}

TEST_CASE("CPU cross-entropy chain matches libtorch value and reverse",
          "[autodiff][parity][cpu][torch][training]") {
  const std::int64_t B = 3, C = 4;
  const auto logits_v = spectral_signal(B * C, 0xAAAA);
  const std::vector<std::int64_t> target_v{2, 0, 3};

  Series logits = make_series(logits_v, {B, C}).to(ScalarType::Float64);
  logits.tracks_slope_(true);
  const Series target = make_series(target_v, {B, 1});
  const Series logp = logits.log_softmax(1);
  const Series picked = logp.gather(1, target).reshape({B});
  const Series loss = picked.neg().mean();
  loss.reverse();

  at::Tensor tlogits =
      at::tensor(logits_v, at::TensorOptions().dtype(at::kDouble)).reshape({B, C}).set_requires_grad(true);
  at::Tensor ttarget = at::tensor(target_v, at::TensorOptions().dtype(at::kLong));
  const at::Tensor tloss = at::cross_entropy_loss(tlogits, ttarget);
  tloss.backward();

  require_close(loss.host<double>(), torch_values(tloss), 2e-11);
  require_close(logits.sensitivity().host<double>(), torch_values(tlogits.grad()), 2e-10);
}

TEST_CASE("CPU embedding lookup matches libtorch value and reverse",
          "[autodiff][parity][cpu][torch][training]") {
  const std::int64_t V = 5, D = 3;
  const auto table_v = spectral_signal(V * D, 0xBBBB);
  const std::vector<std::int64_t> idx_v{3, 0, 3, 1};  // repeated index -> grad accumulates

  Series table = make_series(table_v, {V, D}).to(ScalarType::Float64);
  table.tracks_slope_(true);
  const Series idx = make_series(idx_v, {static_cast<std::int64_t>(idx_v.size())});
  const Series out = table.index_select(0, idx);
  out.mul(out).sum().reverse();

  at::Tensor ttable =
      at::tensor(table_v, at::TensorOptions().dtype(at::kDouble)).reshape({V, D}).set_requires_grad(true);
  at::Tensor tidx = at::tensor(idx_v, at::TensorOptions().dtype(at::kLong));
  const at::Tensor tout = at::embedding(ttable, tidx);
  tout.mul(tout).sum().backward();

  require_close(out.host<double>(), torch_values(tout), 2e-11);
  require_close(table.sensitivity().host<double>(), torch_values(ttable.grad()), 2e-10);
}

// Composite ops (no primitive-rule entry): their gradient is assembled from
// several primitives, so a dtype/stride bug in a constituent kernel only shows
// up here, not in the primitive-rule sweep. Each case is written as a function
// of a single leaf `x` so it flows through check_unary_reverse.
const std::vector<UnaryCase>& composite_unary_cases() {
  static const std::vector<double> gen{-1.3, -0.4, 0.05, 0.35, 1.1};
  static const std::vector<double> pos{0.2, 0.6, 1.1, 1.7, 2.3};
  static const std::vector<UnaryCase> cases{
      {"silu", gen, [](const Series& x) { return x.silu(); },
       [](const at::Tensor& x) { return at::silu(x); }},
      {"softplus", gen, [](const Series& x) { return x.softplus(); },
       [](const at::Tensor& x) { return at::softplus(x); }},
      {"logsumexp_row", gen,
       [](const Series& x) { return x.reshape({1, 5}).logsumexp(1, false); },
       [](const at::Tensor& x) { return x.reshape({1, 5}).logsumexp(1); }},
      {"log_softmax_row", gen,
       [](const Series& x) { return x.reshape({1, 5}).log_softmax(1); },
       [](const at::Tensor& x) { return x.reshape({1, 5}).log_softmax(1); }},
      {"addcmul", gen,
       [](const Series& x) { return x.addcmul(x.mul(0.7), x.add(1.0), 0.5); },
       [](const at::Tensor& x) { return x.addcmul(x.mul(0.7), x.add(1.0), 0.5); }},
      {"addcdiv", pos,
       [](const Series& x) { return x.addcdiv(x.mul(0.7), x.add(1.0), 0.5); },
       [](const at::Tensor& x) { return x.addcdiv(x.mul(0.7), x.add(1.0), 0.5); }},
      {"lerp", gen,
       [](const Series& x) { return x.lerp(x.mul(2.0).add(0.5), 0.3); },
       [](const at::Tensor& x) { return x.lerp(x.mul(2.0).add(0.5), 0.3); }},
  };
  return cases;
}

TEST_CASE("CPU composite op reverse matches libtorch across dtype and stride",
          "[autodiff][parity][cpu][torch][composite]") {
  for (const UnaryCase& test : composite_unary_cases()) {
    DYNAMIC_SECTION(test.name + " float64") {
      check_unary_reverse(test, ScalarType::Float64, at::kDouble, false, 2e-12, 2e-11);
    }
    DYNAMIC_SECTION(test.name + " float32") {
      check_unary_reverse(test, ScalarType::Float32, at::kFloat, false, 3e-6, 3e-5);
    }
    DYNAMIC_SECTION(test.name + " strided_f64") {
      check_unary_reverse(test, ScalarType::Float64, at::kDouble, true, 2e-12, 2e-11);
    }
    DYNAMIC_SECTION(test.name + " bfloat16_selfconsistent") {
      check_unary_reverse_lowp(test, ScalarType::BFloat16, 8e-2);
    }
  }
}

// Indexed-write / gather family, each as a function of a single tracked source
// `s` (shape [6]), checked forward + reverse vs libtorch in Float64 and
// Float32. (The F64-only in-place suite already covers correctness; this pins
// the reduced-precision gradient path for the routing.)
struct IndexCase final {
  std::string name;
  std::function<Series(const Series&)> pluss_op;
  std::function<at::Tensor(const at::Tensor&)> torch_op;
};

const std::vector<IndexCase>& index_family_cases() {
  static const std::vector<IndexCase> cases{
      {"scatter", [](const Series& s) {
         Series base = make_series(std::vector<double>{0, 0, 0, 0, 0, 0}, {6}).to(s.scalar_type());
         return base.scatter(0, make_series(std::vector<std::int64_t>{5, 0, 3, 1, 4, 2}, {6}), s);
       },
       [](const at::Tensor& s) {
         return at::zeros({6}, s.options())
             .scatter(0, at::tensor(std::vector<std::int64_t>{5, 0, 3, 1, 4, 2}, at::kLong), s);
       }},
      {"scatter_add", [](const Series& s) {
         Series base = make_series(std::vector<double>{0, 0, 0}, {3}).to(s.scalar_type());
         return base.scatter_add(0, make_series(std::vector<std::int64_t>{0, 1, 2, 0, 1, 2}, {6}), s);
       },
       [](const at::Tensor& s) {
         return at::zeros({3}, s.options())
             .scatter_add(0, at::tensor(std::vector<std::int64_t>{0, 1, 2, 0, 1, 2}, at::kLong), s);
       }},
      {"index_add", [](const Series& s) {
         Series base = make_series(std::vector<double>{1, 1, 1}, {3}).to(s.scalar_type());
         return base.index_add(0, make_series(std::vector<std::int64_t>{0, 2, 1, 0, 2, 1}, {6}), s, 1.0);
       },
       [](const at::Tensor& s) {
         return at::ones({3}, s.options())
             .index_add(0, at::tensor(std::vector<std::int64_t>{0, 2, 1, 0, 2, 1}, at::kLong), s, 1.0);
       }},
      {"masked_fill", [](const Series& s) {
         return s.masked_fill(
             make_series(std::vector<std::int64_t>{1, 0, 1, 0, 1, 0}, {6}).to(ScalarType::Bool), 2.0);
       },
       [](const at::Tensor& s) {
         return s.masked_fill(at::tensor(std::vector<std::uint8_t>{1, 0, 1, 0, 1, 0}, at::kByte).to(at::kBool), 2.0);
       }},
      {"masked_select", [](const Series& s) {
         return s.masked_select(
             make_series(std::vector<std::int64_t>{1, 0, 1, 1, 0, 1}, {6}).to(ScalarType::Bool));
       },
       [](const at::Tensor& s) {
         return s.masked_select(at::tensor(std::vector<std::uint8_t>{1, 0, 1, 1, 0, 1}, at::kByte).to(at::kBool));
       }},
      {"take", [](const Series& s) {
         return s.take(make_series(std::vector<std::int64_t>{4, 1, 1, 5, 0}, {5}));
       },
       [](const at::Tensor& s) {
         return s.take(at::tensor(std::vector<std::int64_t>{4, 1, 1, 5, 0}, at::kLong));
       }},
  };
  return cases;
}

void check_index_family(const IndexCase& c, ScalarType st, at::ScalarType at_st,
                        double value_tol, double grad_tol) {
  const std::vector<double> sv{0.4, -0.7, 1.1, 0.5, -0.2, 0.9};
  Series s = make_series(sv, {6}).to(st);
  s.tracks_slope_(true);
  const Series out = c.pluss_op(s);
  out.sum().reverse();

  at::Tensor ts = at::tensor(sv, at::TensorOptions().dtype(at::kDouble)).to(at_st).set_requires_grad(true);
  const at::Tensor tout = c.torch_op(ts);
  tout.sum().backward();

  require_close(out.to(ScalarType::Float64).host<double>(), torch_values(tout), value_tol);
  require_close(s.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(ts.grad()), grad_tol);
}

TEST_CASE("CPU indexed-write / gather family reverse matches libtorch in float64 and float32",
          "[autodiff][parity][cpu][torch][indexed-write][dtype]") {
  for (const IndexCase& c : index_family_cases()) {
    DYNAMIC_SECTION(c.name + " float64") {
      check_index_family(c, ScalarType::Float64, at::kDouble, 2e-12, 2e-11);
    }
    DYNAMIC_SECTION(c.name + " float32") {
      check_index_family(c, ScalarType::Float32, at::kFloat, 3e-6, 3e-5);
    }
  }
}

TEST_CASE("CPU _addmm_activation matches libtorch value and decomposed reverse",
          "[autodiff][parity][cpu][torch]") {
  // beta*bias + alpha*(mat1 @ mat2), then relu (use_gelu=false) or exact gelu.
  // libtorch has no autograd formula for the fused op, so the forward value is
  // checked against at::_addmm_activation and the gradient against the
  // equivalent relu(addmm(...)) / gelu(addmm(...)) graph it must be identical to.
  const std::vector<double> bias_v{0.2, -0.5, 0.1, 0.4, -0.3, 0.6, -0.15, 0.25};  // [2,4]
  const std::vector<double> m1_v{0.3, -0.7, 1.1, 0.4, -0.2, 0.9};   // [2,3]
  const std::vector<double> m2_v{0.5, -0.3, 0.8, -0.6, 0.2, 1.0,    // [3,4]
                                 -0.4, 0.7, 0.9, -0.1, 0.35, -0.8};
  const double beta = 0.75, alpha = 1.25;

  for (const bool use_gelu : {false, true}) {
    DYNAMIC_SECTION((use_gelu ? "gelu" : "relu")) {
      Series bias = make_series(bias_v, {2, 4}).to(ScalarType::Float64);
      Series m1 = make_series(m1_v, {2, 3}).to(ScalarType::Float64);
      Series m2 = make_series(m2_v, {3, 4}).to(ScalarType::Float64);
      bias.tracks_slope_(true);
      m1.tracks_slope_(true);
      m2.tracks_slope_(true);
      const Series out = bias._addmm_activation(m1, m2, beta, alpha, use_gelu);
      out.sum().reverse();

      at::Tensor tb = at::tensor(bias_v, at::TensorOptions().dtype(at::kDouble)).reshape({2, 4});
      at::Tensor t1 = at::tensor(m1_v, at::TensorOptions().dtype(at::kDouble)).reshape({2, 3});
      at::Tensor t2 = at::tensor(m2_v, at::TensorOptions().dtype(at::kDouble)).reshape({3, 4});

      const at::Tensor fused = at::_addmm_activation(tb, t1, t2, beta, alpha, use_gelu);

      at::Tensor gb = tb.detach().clone().set_requires_grad(true);
      at::Tensor g1 = t1.detach().clone().set_requires_grad(true);
      at::Tensor g2 = t2.detach().clone().set_requires_grad(true);
      const at::Tensor linear = at::addmm(gb, g1, g2, beta, alpha);
      const at::Tensor decomposed =
          use_gelu ? at::gelu(linear, "none") : at::relu(linear);
      decomposed.sum().backward();

      require_close(out.host<double>(), torch_values(fused), 2e-12);
      require_close(out.host<double>(), torch_values(decomposed), 2e-12);
      require_close(bias.sensitivity().host<double>(), torch_values(gb.grad()), 2e-11);
      require_close(m1.sensitivity().host<double>(), torch_values(g1.grad()), 2e-11);
      require_close(m2.sensitivity().host<double>(), torch_values(g2.grad()), 2e-11);
    }
  }
}

TEST_CASE("CPU argless nan_to_num reverse matches libtorch across non-finite lanes",
          "[autodiff][parity][cpu][torch]") {
  // NaN, +Inf, -Inf, and finite lanes. Argless form: NaN -> 0, +Inf -> the
  // largest finite double, -Inf -> the most negative finite double. The reverse
  // must carry 1 on the finite lanes and 0 on every non-finite lane.
  const std::vector<double> values{
      std::nan(""), std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(), -0.7, 0.0, 2.3};

  Series input = make_series(values, {static_cast<std::int64_t>(values.size())})
                     .to(ScalarType::Float64);
  input.tracks_slope_(true);
  const Series result = input.nan_to_num();
  result.sum().reverse();

  at::Tensor torch_input = at::tensor(
      values, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
  at::Tensor torch_result = torch_input.nan_to_num();
  torch_result.sum().backward();

  require_close(result.host<double>(), torch_values(torch_result), 0.0);
  require_close(input.sensitivity().host<double>(),
                torch_values(torch_input.grad()), 2e-12);
}

const std::vector<ComplexUnaryCase>& complex_unary_reverse_cases() {
  static const std::vector<ComplexUnaryCase> cases{
      {"sqrt", [](const Series& x) { return x.sqrt(); }, [](const at::Tensor& x) { return x.sqrt(); }},
      {"exp", [](const Series& x) { return x.exp(); }, [](const at::Tensor& x) { return x.exp(); }},
      {"log", [](const Series& x) { return x.log(); }, [](const at::Tensor& x) { return x.log(); }},
      {"sin", [](const Series& x) { return x.sin(); }, [](const at::Tensor& x) { return x.sin(); }},
      {"cos", [](const Series& x) { return x.cos(); }, [](const at::Tensor& x) { return x.cos(); }},
      {"tan", [](const Series& x) { return x.tan(); }, [](const at::Tensor& x) { return x.tan(); }},
      {"asin", [](const Series& x) { return x.asin(); }, [](const at::Tensor& x) { return x.asin(); }},
      {"acos", [](const Series& x) { return x.acos(); }, [](const at::Tensor& x) { return x.acos(); }},
      {"atan", [](const Series& x) { return x.atan(); }, [](const at::Tensor& x) { return x.atan(); }},
      {"sinh", [](const Series& x) { return x.sinh(); }, [](const at::Tensor& x) { return x.sinh(); }},
      {"cosh", [](const Series& x) { return x.cosh(); }, [](const at::Tensor& x) { return x.cosh(); }},
      {"tanh", [](const Series& x) { return x.tanh(); }, [](const at::Tensor& x) { return x.tanh(); }},
      {"log1p", [](const Series& x) { return x.log1p(); }, [](const at::Tensor& x) { return x.log1p(); }},
      {"expm1", [](const Series& x) { return x.expm1(); }, [](const at::Tensor& x) { return x.expm1(); }},
      {"rsqrt", [](const Series& x) { return x.rsqrt(); }, [](const at::Tensor& x) { return x.rsqrt(); }},
      {"reciprocal", [](const Series& x) { return x.reciprocal(); }, [](const at::Tensor& x) { return x.reciprocal(); }},
      {"asinh", [](const Series& x) { return x.asinh(); }, [](const at::Tensor& x) { return x.asinh(); }},
      {"acosh", [](const Series& x) { return x.acosh(); }, [](const at::Tensor& x) { return x.acosh(); }},
      {"atanh", [](const Series& x) { return x.atanh(); }, [](const at::Tensor& x) { return x.atanh(); }},
      {"sigmoid", [](const Series& x) { return x.sigmoid(); }, [](const at::Tensor& x) { return x.sigmoid(); }},
      {"square", [](const Series& x) { return x.square(); }, [](const at::Tensor& x) { return x.square(); }},
  };
  return cases;
}

void check_complex_unary_reverse(const ComplexUnaryCase& test, ScalarType st,
                                 at::ScalarType at_st, double margin) {
  const std::vector<std::complex<double>> input_values{
      {0.4, -0.7}, {-0.2, 0.3}, {1.1, 0.5}};
  const std::vector<std::complex<double>> seed_values{
      {0.5, 0.2}, {-0.3, 0.7}, {1.2, -0.4}};
  const std::vector<c10::complex<double>> torch_input_values{
      {0.4, -0.7}, {-0.2, 0.3}, {1.1, 0.5}};
  const std::vector<c10::complex<double>> torch_seed_values{
      {0.5, 0.2}, {-0.3, 0.7}, {1.2, -0.4}};

  Series input = make_series(input_values, {3}).to(st);
  input.tracks_slope_(true);
  const Series seed = make_series(seed_values, {3}).to(st);
  const Series result = test.pluss_op(input);
  result.reverse(seed);

  at::Tensor torch_input =
      at::tensor(torch_input_values, at::TensorOptions().dtype(at::kComplexDouble))
          .to(at_st).set_requires_grad(true);
  const at::Tensor torch_seed =
      at::tensor(torch_seed_values, at::TensorOptions().dtype(at::kComplexDouble)).to(at_st);
  at::Tensor torch_result = test.torch_op(torch_input);
  torch_result.backward(torch_seed);

  require_complex_close(result.to(ScalarType::Complex128).host<std::complex<double>>(),
                        torch_complex_values(torch_result), margin);
  require_complex_close(input.sensitivity().to(ScalarType::Complex128).host<std::complex<double>>(),
                        torch_complex_values(torch_input.grad()), margin);
}

TEST_CASE("generated complex unary reverse rules conjugate like libtorch",
          "[autodiff][parity][cpu][torch][generated][complex]") {
  for (const ComplexUnaryCase& test : complex_unary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_complex_unary_reverse(test, ScalarType::Complex128, at::kComplexDouble, 1e-11);
    }
  }
}

TEST_CASE("generated complex unary reverse rules conjugate like libtorch in complex64",
          "[autodiff][parity][cpu][torch][generated][complex][dtype]") {
  for (const ComplexUnaryCase& test : complex_unary_reverse_cases()) {
    DYNAMIC_SECTION(test.name) {
      check_complex_unary_reverse(test, ScalarType::Complex64, at::kComplexFloat, 3e-5);
    }
  }
}

TEST_CASE("generated complex binary reverse rules conjugate like libtorch",
          "[autodiff][parity][cpu][torch][generated][complex]") {
  const std::vector<std::complex<double>> lhs_values{
      {0.4, -0.7}, {0.8, 0.3}, {1.1, 0.5}};
  const std::vector<std::complex<double>> rhs_values{
      {1.2, 0.2}, {0.7, -0.4}, {1.5, 0.1}};
  const std::vector<std::complex<double>> seed_values{
      {0.5, 0.2}, {-0.3, 0.7}, {1.2, -0.4}};
  const std::vector<c10::complex<double>> torch_lhs_values{
      {0.4, -0.7}, {0.8, 0.3}, {1.1, 0.5}};
  const std::vector<c10::complex<double>> torch_rhs_values{
      {1.2, 0.2}, {0.7, -0.4}, {1.5, 0.1}};
  const std::vector<c10::complex<double>> torch_seed_values{
      {0.5, 0.2}, {-0.3, 0.7}, {1.2, -0.4}};
  const std::vector<ComplexBinaryCase> cases{
      {"add", [](const Series& a, const Series& b) { return a.add(b); }, [](const at::Tensor& a, const at::Tensor& b) { return a.add(b); }},
      {"sub", [](const Series& a, const Series& b) { return a.sub(b); }, [](const at::Tensor& a, const at::Tensor& b) { return a.sub(b); }},
      {"mul", [](const Series& a, const Series& b) { return a.mul(b); }, [](const at::Tensor& a, const at::Tensor& b) { return a.mul(b); }},
      {"div", [](const Series& a, const Series& b) { return a.div(b); }, [](const at::Tensor& a, const at::Tensor& b) { return a.div(b); }},
      {"pow", [](const Series& a, const Series& b) { return a.pow(b); }, [](const at::Tensor& a, const at::Tensor& b) { return a.pow(b); }},
  };

  for (const ComplexBinaryCase& test : cases) {
    DYNAMIC_SECTION(test.name) {
      Series lhs = make_series(lhs_values, {3}).to(ScalarType::Complex128);
      Series rhs = make_series(rhs_values, {3}).to(ScalarType::Complex128);
      lhs.tracks_slope_(true);
      rhs.tracks_slope_(true);
      const Series seed = make_series(seed_values, {3}).to(ScalarType::Complex128);
      const Series result = test.pluss_op(lhs, rhs);
      result.reverse(seed);

      at::Tensor torch_lhs = at::tensor(
          torch_lhs_values,
          at::TensorOptions().dtype(at::kComplexDouble)).set_requires_grad(true);
      at::Tensor torch_rhs = at::tensor(
          torch_rhs_values,
          at::TensorOptions().dtype(at::kComplexDouble)).set_requires_grad(true);
      const at::Tensor torch_seed = at::tensor(
          torch_seed_values,
          at::TensorOptions().dtype(at::kComplexDouble));
      at::Tensor torch_result = test.torch_op(torch_lhs, torch_rhs);
      torch_result.backward(torch_seed);

      require_complex_close(result.host<std::complex<double>>(), torch_complex_values(torch_result));
      require_complex_close(lhs.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_lhs.grad()));
      require_complex_close(rhs.sensitivity().host<std::complex<double>>(),
                            torch_complex_values(torch_rhs.grad()));
    }
  }
}

TEST_CASE("CPU scatter_reduce reverse matches libtorch for every reduction",
          "[autodiff][parity][cpu][torch]") {
  const std::vector<std::int64_t> index_values{0, 0, 2};
  const std::vector<ScatterCase> cases{
      {"sum", {2.0, 7.0, 4.0}, {3.0, 5.0, 6.0}},
      {"mean", {2.0, 7.0, 4.0}, {3.0, 5.0, 6.0}},
      {"prod", {2.0, 7.0, 4.0}, {0.0, 5.0, 0.0}},
      {"amax", {5.0, 7.0, 4.0}, {5.0, 5.0, 4.0}},
      {"amin", {5.0, 7.0, 4.0}, {5.0, 5.0, 4.0}},
  };

  for (const bool include_self : {true, false}) {
    for (const ScatterCase& test : cases) {
      DYNAMIC_SECTION(test.reduce << ", include_self=" << include_self) {
        Series receiver = make_series(test.receiver, {3}).to(ScalarType::Float64);
        Series source = make_series(test.source, {3}).to(ScalarType::Float64);
        const Series index = make_series(index_values, {3});
        receiver.tracks_slope_(true);
        source.tracks_slope_(true);
        const Series result = receiver.scatter_reduce(
            0, index, source, test.reduce, include_self);
        result.sum().reverse();

        auto options = at::TensorOptions().dtype(at::kDouble);
        at::Tensor torch_receiver = at::tensor(test.receiver, options).set_requires_grad(true);
        at::Tensor torch_source = at::tensor(test.source, options).set_requires_grad(true);
        const at::Tensor torch_index = at::tensor(
            index_values, at::TensorOptions().dtype(at::kLong));
        at::Tensor torch_result = torch_receiver.scatter_reduce(
            0, torch_index, torch_source, test.reduce, include_self);
        torch_result.sum().backward();

        require_close(result.host<double>(), torch_values(torch_result));
        require_close(receiver.sensitivity().host<double>(), torch_values(torch_receiver.grad()));
        require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
      }
    }
  }
}

TEST_CASE("CPU dense training chain matches libtorch through broadcasting and matmul",
          "[autodiff][parity][cpu][torch][ai]") {
  const std::vector<double> x_values{0.2, -0.3, 0.7, 1.1, -0.4, 0.5};
  const std::vector<double> w_values{0.5, -0.2, 0.8, 0.4, -0.6, 0.9};
  const std::vector<double> b_values{0.1, -0.25};

  Series x = make_series(x_values, {2, 3}).to(ScalarType::Float64);
  Series w = make_series(w_values, {3, 2}).to(ScalarType::Float64);
  Series b = make_series(b_values, {2}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  w.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series result = x.matmul(w).add(b).tanh().square().mean();
  result.reverse();

  auto options = at::TensorOptions().dtype(at::kDouble);
  at::Tensor torch_x = at::tensor(x_values, options).set_requires_grad(true).reshape({2, 3});
  at::Tensor torch_w = at::tensor(w_values, options).set_requires_grad(true).reshape({3, 2});
  at::Tensor torch_b = at::tensor(b_values, options).set_requires_grad(true);
  torch_x.retain_grad();
  torch_w.retain_grad();
  at::Tensor torch_result = torch_x.matmul(torch_w).add(torch_b).tanh().square().mean();
  torch_result.backward();

  require_close(result.host<double>(), torch_values(torch_result));
  require_close(x.sensitivity().host<double>(), torch_values(torch_x.grad()));
  require_close(w.sensitivity().host<double>(), torch_values(torch_w.grad()));
  require_close(b.sensitivity().host<double>(), torch_values(torch_b.grad()));
}

TEST_CASE("CPU embedding-style indexing and views match libtorch reverse",
          "[autodiff][parity][cpu][torch][ai]") {
  const std::vector<double> table_values{
      0.1, 0.2, 0.3,
      0.4, 0.5, 0.6,
      0.7, 0.8, 0.9,
      1.0, 1.1, 1.2};
  const std::vector<std::int64_t> index_values{3, 1, 3};

  Series table = make_series(table_values, {4, 3}).to(ScalarType::Float64);
  table.tracks_slope_(true);
  const Series index = make_series(index_values, {3});
  const Series result = table.index_select(0, index).transpose(0, 1).reshape({9}).exp().sum();
  result.reverse();

  auto options = at::TensorOptions().dtype(at::kDouble);
  at::Tensor torch_table = at::tensor(table_values, options).set_requires_grad(true).reshape({4, 3});
  torch_table.retain_grad();
  const at::Tensor torch_index = at::tensor(
      index_values, at::TensorOptions().dtype(at::kLong));
  at::Tensor torch_result = torch_table.index_select(0, torch_index)
                                   .transpose(0, 1)
                                   .reshape({9})
                                   .exp()
                                   .sum();
  torch_result.backward();

  require_close(result.host<double>(), torch_values(torch_result));
  require_close(table.sensitivity().host<double>(), torch_values(torch_table.grad()));
}

TEST_CASE("CPU softmax and gather training chain matches libtorch reverse",
          "[autodiff][parity][cpu][torch][ai]") {
  const std::vector<double> logits_values{0.2, -0.1, 0.7, 1.0, 0.4, -0.3};
  const std::vector<double> weight_values{0.5, -0.2, 0.8, -0.4};
  const std::vector<std::int64_t> gather_values{2, 0, 1, 2};

  Series logits = make_series(logits_values, {2, 3}).to(ScalarType::Float64);
  Series weights = make_series(weight_values, {2, 2}).to(ScalarType::Float64);
  logits.tracks_slope_(true);
  weights.tracks_slope_(true);
  const Series gather_index = make_series(gather_values, {2, 2});
  const Series chosen = logits.softmax(1).gather(1, gather_index);
  const Series result = chosen.mul(weights).sum();
  result.reverse();

  auto options = at::TensorOptions().dtype(at::kDouble);
  at::Tensor torch_logits = at::tensor(logits_values, options).set_requires_grad(true).reshape({2, 3});
  at::Tensor torch_weights = at::tensor(weight_values, options).set_requires_grad(true).reshape({2, 2});
  torch_logits.retain_grad();
  torch_weights.retain_grad();
  const at::Tensor torch_gather = at::tensor(
      gather_values, at::TensorOptions().dtype(at::kLong)).reshape({2, 2});
  at::Tensor torch_result = torch_logits.softmax(1).gather(1, torch_gather).mul(torch_weights).sum();
  torch_result.backward();

  require_close(result.host<double>(), torch_values(torch_result));
  require_close(logits.sensitivity().host<double>(), torch_values(torch_logits.grad()));
  require_close(weights.sensitivity().host<double>(), torch_values(torch_weights.grad()));
}

// A small classification-head chain -- matmul + broadcast-add + tanh + softmax
// + log + reduction -- run at a chosen dtype and, optionally, with a
// non-contiguous `x` (transposed [F,N] leaf). Compares value and both parameter
// gradients to libtorch computed the same way at the same dtype.
void check_head_chain(ScalarType st, at::ScalarType at_st, bool strided_x,
                      double value_tol, double grad_tol) {
  const std::int64_t N = 3, F = 4, K = 2;
  const std::vector<double> x_v{0.20, -0.30, 0.70, 1.10,
                                -0.40, 0.50, 0.05, -0.90,
                                0.60, -0.10, 0.30, 0.80};        // [N,F]
  const std::vector<double> w_v{0.50, -0.20, 0.80, 0.40,
                                -0.60, 0.90, 0.10, -0.30};        // [F,K]
  const std::vector<double> b_v{0.10, -0.25};                     // [K]
  const std::vector<double> t_v{0.7, 0.3, 0.2, 0.8, 0.55, 0.45};  // [N,K] soft targets

  Series w = make_series(w_v, {F, K}).to(st);
  Series b = make_series(b_v, {K}).to(st);
  Series target = make_series(t_v, {N, K}).to(st);
  Series x_leaf = pluss::series::undefined();
  if (!strided_x) {
    x_leaf = make_series(x_v, {N, F}).to(st);
  } else {
    std::vector<double> xt(x_v.size());
    for (std::int64_t i = 0; i < N; ++i)
      for (std::int64_t j = 0; j < F; ++j) xt[j * N + i] = x_v[i * F + j];
    x_leaf = make_series(xt, {F, N}).to(st);
  }
  x_leaf.tracks_slope_(true);
  w.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series x_used = strided_x ? x_leaf.transpose(0, 1) : x_leaf;
  const Series logits = x_used.matmul(w).add(b).tanh();
  const Series logp = logits.softmax(1).log();
  const Series loss = logp.mul(target).neg().sum();
  loss.reverse();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor tx_leaf, tx_used;
  if (!strided_x) {
    tx_leaf = at::tensor(x_v, opt).to(at_st).reshape({N, F});
    tx_leaf.set_requires_grad(true);
    tx_used = tx_leaf;
  } else {
    std::vector<double> xt(x_v.size());
    for (std::int64_t i = 0; i < N; ++i)
      for (std::int64_t j = 0; j < F; ++j) xt[j * N + i] = x_v[i * F + j];
    tx_leaf = at::tensor(xt, opt).to(at_st).reshape({F, N});
    tx_leaf.set_requires_grad(true);
    tx_used = tx_leaf.transpose(0, 1);
  }
  at::Tensor tw = at::tensor(w_v, opt).to(at_st).reshape({F, K});
  at::Tensor tb = at::tensor(b_v, opt).to(at_st).reshape({K});
  at::Tensor tt = at::tensor(t_v, opt).to(at_st).reshape({N, K});
  tw.set_requires_grad(true);
  tb.set_requires_grad(true);
  const at::Tensor tlogits = tx_used.matmul(tw).add(tb).tanh();
  const at::Tensor tloss = tlogits.softmax(1).log().mul(tt).neg().sum();
  tloss.backward();

  require_close(loss.to(ScalarType::Float64).host<double>(), torch_values(tloss), value_tol);
  require_close(x_leaf.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(tx_leaf.grad()), grad_tol);
  require_close(w.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(tw.grad()), grad_tol);
  require_close(b.sensitivity().to(ScalarType::Float64).host<double>(),
                torch_values(tb.grad()), grad_tol);
}

TEST_CASE("CPU matmul reverse reaches a transposed (non-contiguous) input leaf",
          "[autodiff][parity][cpu][torch][repro]") {
  // xt: [F,N] leaf; used as xt.transpose(0,1) @ w  ->  [N,K]. The gradient must
  // flow back through the transpose onto xt.
  const std::int64_t N = 3, F = 4, K = 2;
  const std::vector<double> xt_v{0.2, -0.3, 0.7, 0.5, -0.4, 0.9, 0.1, -0.6,
                                 0.3, 0.8, -0.2, 0.4};  // [F,N]
  const std::vector<double> w_v{0.5, -0.2, 0.8, 0.4, -0.6, 0.9, 0.1, -0.3};  // [F,K]

  Series xt = make_series(xt_v, {F, N}).to(ScalarType::Float64);
  Series w = make_series(w_v, {F, K}).to(ScalarType::Float64);
  xt.tracks_slope_(true);
  w.tracks_slope_(true);
  const Series out = xt.transpose(0, 1).matmul(w).sum();
  out.reverse();

  auto opt = at::TensorOptions().dtype(at::kDouble);
  at::Tensor txt = at::tensor(xt_v, opt).reshape({F, N}).set_requires_grad(true);
  at::Tensor tw = at::tensor(w_v, opt).reshape({F, K}).set_requires_grad(true);
  txt.retain_grad();
  const at::Tensor tout = txt.transpose(0, 1).matmul(tw).sum();
  tout.backward();

  require_close(out.host<double>(), torch_values(tout), 2e-12);
  require_close(xt.sensitivity().host<double>(), torch_values(txt.grad()), 2e-11);
  require_close(w.sensitivity().host<double>(), torch_values(tw.grad()), 2e-11);
}

TEST_CASE("CPU stft matches libtorch value and reverse",
          "[autodiff][parity][cpu][torch][spectral]") {
  const std::int64_t T = 48, n_fft = 16, hop = 4;
  struct Cfg { std::string name; bool center; bool normalized; bool onesided; bool window; };
  const std::vector<Cfg> cfgs{
      {"basic", true, false, true, false},
      {"normalized", true, true, true, false},
      {"no_center", false, false, true, false},
      {"two_sided", true, false, false, false},
      {"windowed", true, false, true, true},
  };
  for (const Cfg& c : cfgs) {
    DYNAMIC_SECTION(c.name) {
      const auto sv = spectral_signal(T, 0x51ED);
      const std::vector<double> wv{0.08, 0.31, 0.6, 0.86, 1.0, 0.86, 0.6, 0.31, 0.08, 0.0};
      const std::int64_t win_len = c.window ? static_cast<std::int64_t>(wv.size()) : n_fft;

      Series x = make_series(sv, {T}).to(ScalarType::Float64);
      x.tracks_slope_(true);
      Series w = c.window ? make_series(wv, {win_len}).to(ScalarType::Float64)
                          : pluss::series::undefined();
      const Series spec =
          x.stft(n_fft, hop, win_len, w, c.center, "reflect", c.normalized, c.onesided, true);
      spec.abs().sum().reverse();

      at::Tensor tx = at::tensor(sv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
      std::optional<at::Tensor> tw =
          c.window ? std::optional<at::Tensor>(at::tensor(wv, at::TensorOptions().dtype(at::kDouble)))
                   : std::nullopt;
      const at::Tensor tspec = at::stft(tx, n_fft, hop, win_len, tw, c.center, "reflect",
                                        c.normalized, c.onesided, true);
      tspec.abs().sum().backward();

      require_close(spec.abs().to(ScalarType::Float64).host<double>(),
                    torch_values(tspec.abs()), 2e-10);
      require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-9);
    }
  }
}

TEST_CASE("CPU istft round-trips and matches libtorch reverse",
          "[autodiff][parity][cpu][torch][spectral]") {
  const std::int64_t T = 64, n_fft = 16, hop = 4;
  for (const bool normalized : {false, true}) {
    DYNAMIC_SECTION((normalized ? "normalized" : "plain")) {
      const auto sv = spectral_signal(T, 0x9E37);
      Series x = make_series(sv, {T}).to(ScalarType::Float64);
      x.tracks_slope_(true);
      const Series recon =
          x.stft(n_fft, hop, n_fft, pluss::series::undefined(), true, "reflect", normalized, true, true)
              .istft(n_fft, hop, n_fft, pluss::series::undefined(), true, normalized, true, T);
      recon.mul(recon).sum().reverse();

      at::Tensor tx = at::tensor(sv, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
      const at::Tensor tspec =
          at::stft(tx, n_fft, hop, n_fft, std::nullopt, true, "reflect", normalized, true, true);
      const at::Tensor trecon =
          at::istft(tspec, n_fft, hop, n_fft, std::nullopt, true, normalized, true, T, false);
      trecon.mul(trecon).sum().backward();

      require_close(recon.host<double>(), torch_values(trecon), 2e-9);
      require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-8);
    }
  }
}

TEST_CASE("CPU classification-head chain matches libtorch (matmul+softmax+reduce)",
          "[autodiff][parity][cpu][torch][ai]") {
  SECTION("float64") { check_head_chain(ScalarType::Float64, at::kDouble, false, 2e-12, 2e-11); }
  SECTION("float32") { check_head_chain(ScalarType::Float32, at::kFloat, false, 2e-5, 2e-4); }
  SECTION("float64 non-contiguous x") { check_head_chain(ScalarType::Float64, at::kDouble, true, 2e-12, 2e-11); }
  SECTION("float32 non-contiguous x") { check_head_chain(ScalarType::Float32, at::kFloat, true, 2e-5, 2e-4); }
}

TEST_CASE("CPU view expansion and activation chain matches libtorch reverse",
          "[autodiff][parity][cpu][torch][ai]") {
  const std::vector<double> values{-1.2, -0.5, 0.0, 0.4, 0.8, 1.3};

  Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
  input.tracks_slope_(true);
  const Series result = input.permute({1, 0})
                            .unsqueeze(0)
                            .expand({2, 3, 2})
                            .select(0, 1)
                            .reshape({6})
                            .softplus(1.2, 20.0)
                            .gelu(false)
                            .mean();
  result.reverse();

  at::Tensor torch_input = at::tensor(
      values, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true).reshape({2, 3});
  torch_input.retain_grad();
  const at::Tensor torch_view = torch_input.permute({1, 0})
                                    .unsqueeze(0)
                                    .expand({2, 3, 2})
                                    .select(0, 1)
                                    .reshape({6});
  at::Tensor torch_result = at::gelu(at::softplus(torch_view, 1.2, 20.0)).mean();
  torch_result.backward();

  require_close(result.host<double>(), torch_values(torch_result), 2e-12);
  require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()), 2e-11);
}

TEST_CASE("CPU reductions and overlapping views match libtorch reverse",
          "[autodiff][parity][cpu][torch][ai]") {
  const std::vector<double> values{1.0, 2.0, 2.0, -1.0, 3.0, 0.5};

  Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
  input.tracks_slope_(true);
  const Series result = input.var(1, false, true).sum()
                            .add(input.amax(1, false).sum())
                            .add(input.unfold(1, 2, 1).square().mean());
  result.reverse();

  at::Tensor torch_input = at::tensor(
      values, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true).reshape({2, 3});
  torch_input.retain_grad();
  at::Tensor torch_result = torch_input.var(1, false, true).sum()
                                .add(torch_input.amax(1, false).sum())
                                .add(torch_input.unfold(1, 2, 1).square().mean());
  torch_result.backward();

  require_close(result.host<double>(), torch_values(torch_result), 2e-12);
  require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()), 2e-11);
}

TEST_CASE("CPU prod reverse matches libtorch including zero lanes",
          "[autodiff][parity][cpu][torch][ai]") {
  // Lane 0: no zeros. Lane 1: exactly one zero. Lane 2: two zeros.
  const std::vector<double> values{1.5, -2.0, 3.0, 4.0, 0.0, -1.5, 0.0, 2.0, 0.0};

  struct Case { std::string name; std::function<Series(const Series&)> p;
                std::function<at::Tensor(const at::Tensor&)> t; };
  const std::vector<Case> cases{
      {"dim", [](const Series& x) { return x.prod(1, false); },
              [](const at::Tensor& x) { return x.prod(1); }},
      {"dim_keepdim", [](const Series& x) { return x.prod(1, true); },
                      [](const at::Tensor& x) { return x.prod(1, true); }},
      {"all", [](const Series& x) { return x.prod(); },
              [](const at::Tensor& x) { return x.prod(); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      Series input = make_series(values, {3, 3}).to(ScalarType::Float64);
      input.tracks_slope_(true);
      const Series result = c.p(input).sum();
      result.reverse();

      at::Tensor torch_input = at::tensor(
          values, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true).reshape({3, 3});
      torch_input.retain_grad();
      at::Tensor torch_result = c.t(torch_input).sum();
      torch_result.backward();

      require_close(result.host<double>(), torch_values(torch_result), 2e-12);
      require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()), 2e-11);
    }
  }
}

TEST_CASE("CPU minimum and maximum split tie gradients like libtorch",
          "[autodiff][parity][cpu][torch]") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<double> lhs_values{1.0, 2.0, 4.0, 3.0, nan, 5.0};
  const std::vector<double> rhs_values{2.0, 2.0, 1.0, 3.0, 5.0, nan};

  for (const bool use_maximum : {false, true}) {
    DYNAMIC_SECTION((use_maximum ? "maximum" : "minimum")) {
      Series lhs = make_series(lhs_values, {2, 3}).to(ScalarType::Float64);
      Series rhs = make_series(rhs_values, {2, 3}).to(ScalarType::Float64);
      lhs.tracks_slope_(true);
      rhs.tracks_slope_(true);
      const Series result = use_maximum ? lhs.maximum(rhs) : lhs.minimum(rhs);
      result.sum().reverse();

      auto options = at::TensorOptions().dtype(at::kDouble);
      at::Tensor torch_lhs = at::tensor(lhs_values, options).set_requires_grad(true).reshape({2, 3});
      at::Tensor torch_rhs = at::tensor(rhs_values, options).set_requires_grad(true).reshape({2, 3});
      torch_lhs.retain_grad();
      torch_rhs.retain_grad();
      at::Tensor torch_result = use_maximum
          ? at::maximum(torch_lhs, torch_rhs)
          : at::minimum(torch_lhs, torch_rhs);
      torch_result.sum().backward();

      require_close(result.host<double>(), torch_values(torch_result));
      require_close(lhs.sensitivity().host<double>(), torch_values(torch_lhs.grad()));
      require_close(rhs.sensitivity().host<double>(), torch_values(torch_rhs.grad()));
    }
  }
}

TEST_CASE("CPU clamp propagates NaN and keeps inclusive boundary gradients",
          "[autodiff][parity][cpu][torch]") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<double> values{-1.0, -0.5, 0.0, 0.5, 1.0, nan};

  Series input = make_series(values, {6}).to(ScalarType::Float64);
  input.tracks_slope_(true);
  const Series result = input.clamp(-0.5, 0.5);
  result.sum().reverse();

  at::Tensor torch_input = at::tensor(
      values, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
  at::Tensor torch_result = torch_input.clamp(-0.5, 0.5);
  torch_result.sum().backward();

  require_close(result.host<double>(), torch_values(torch_result));
  require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
}

TEST_CASE("CPU fmin and fmax match libtorch tie and NaN gradient routing",
          "[autodiff][parity][cpu][torch]") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<double> lhs_values{1.0, 2.0, nan, nan};
  const std::vector<double> rhs_values{2.0, 2.0, 3.0, nan};

  for (const bool use_maximum : {false, true}) {
    DYNAMIC_SECTION((use_maximum ? "fmax" : "fmin")) {
      Series lhs = make_series(lhs_values, {4}).to(ScalarType::Float64);
      Series rhs = make_series(rhs_values, {4}).to(ScalarType::Float64);
      lhs.tracks_slope_(true);
      rhs.tracks_slope_(true);
      const Series result = use_maximum ? lhs.fmax(rhs) : lhs.fmin(rhs);
      result.sum().reverse();

      auto options = at::TensorOptions().dtype(at::kDouble);
      at::Tensor torch_lhs = at::tensor(lhs_values, options).set_requires_grad(true);
      at::Tensor torch_rhs = at::tensor(rhs_values, options).set_requires_grad(true);
      at::Tensor torch_result = use_maximum
          ? at::fmax(torch_lhs, torch_rhs)
          : at::fmin(torch_lhs, torch_rhs);
      torch_result.sum().backward();

      require_close(result.host<double>(), torch_values(torch_result));
      require_close(lhs.sensitivity().host<double>(), torch_values(torch_lhs.grad()));
      require_close(rhs.sensitivity().host<double>(), torch_values(torch_rhs.grad()));
    }
  }
}

TEST_CASE("CPU index_reduce extrema ties match libtorch with and without the receiver",
          "[autodiff][parity][cpu][torch][indexed-write]") {
  auto options = at::TensorOptions().dtype(at::kDouble);
  const std::vector<double> base_values{3.0};
  const std::vector<double> source_values{3.0, 3.0};
  const std::vector<std::int64_t> index_values{0, 0};

  for (const std::string reduce : {"amax", "amin"}) {
    for (const bool include_self : {false, true}) {
      DYNAMIC_SECTION(reduce << (include_self ? " include_self" : " exclude_self")) {
        Series base = make_series(base_values, {1}).to(ScalarType::Float64);
        Series source = make_series(source_values, {2}).to(ScalarType::Float64);
        const Series index = make_series(index_values, {2});
        base.tracks_slope_(true);
        source.tracks_slope_(true);
        const Series output = base.index_reduce(0, index, source, reduce, include_self);
        output.sum().reverse();

        at::Tensor torch_base = at::tensor(base_values, options).set_requires_grad(true);
        at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
        const at::Tensor torch_index = at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
        const at::Tensor torch_output =
            torch_base.index_reduce(0, torch_index, torch_source, reduce, include_self);
        torch_output.sum().backward();

        require_close(output.host<double>(), torch_values(torch_output));
        require_close(base.sensitivity().host<double>(), torch_values(torch_base.grad()));
        require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
      }
    }
  }
}

TEST_CASE("CPU masked scatter routes receiver and source gradients like libtorch",
          "[autodiff][parity][cpu][torch][indexed-write]") {
  const std::vector<double> receiver_values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  const std::vector<double> source_values{10.0, 20.0, 30.0};
  const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8, 1.0, 1.2};
  const std::vector<std::uint8_t> mask_values{1, 0, 0, 1, 1, 0};

  Series receiver = make_series(receiver_values, {2, 3}).to(ScalarType::Float64);
  Series source = make_series(source_values, {3}).to(ScalarType::Float64);
  const Series mask = make_series(mask_values, {2, 3}).to(ScalarType::Bool);
  receiver.tracks_slope_(true);
  source.tracks_slope_(true);
  const Series output = receiver.masked_scatter(mask, source);
  output.reverse(make_series(seed_values, {2, 3}).to(ScalarType::Float64));

  auto options = at::TensorOptions().dtype(at::kDouble);
  at::Tensor torch_receiver =
      at::tensor(receiver_values, options).reshape({2, 3}).set_requires_grad(true);
  at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
  const at::Tensor torch_mask = at::tensor(mask_values, at::TensorOptions().dtype(at::kByte))
                                    .to(at::kBool)
                                    .reshape({2, 3});
  const at::Tensor torch_output = torch_receiver.masked_scatter(torch_mask, torch_source);
  torch_output.backward(at::tensor(seed_values, options).reshape({2, 3}));

  require_close(output.host<double>(), torch_values(torch_output));
  require_close(receiver.sensitivity().host<double>(), torch_values(torch_receiver.grad()));
  require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
}

TEST_CASE("CPU masked select scatters compact cotangents like libtorch",
          "[autodiff][parity][cpu][torch][operation-node][masked-select]") {
  const auto options = at::TensorOptions().dtype(at::kDouble);
  const std::vector<double> values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

  for (const std::vector<std::int64_t>& mask_values : {
           std::vector<std::int64_t>{1, 0, 1, 0, 0, 1},
           std::vector<std::int64_t>{0, 0, 0, 0, 0, 0}}) {
    Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series mask = make_series(mask_values, {2, 3}).to(ScalarType::Bool);
    const Series output = input.masked_select(mask);
    output.sum().reverse();

    at::Tensor torch_input =
        at::tensor(values, options).reshape({2, 3}).set_requires_grad(true);
    const at::Tensor torch_mask =
        at::tensor(mask_values, at::TensorOptions().dtype(at::kLong))
            .reshape({2, 3})
            .to(at::kBool);
    const at::Tensor torch_output = torch_input.masked_select(torch_mask);
    torch_output.sum().backward();

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }
}

TEST_CASE("CPU non-leaf in-place indexed writes preserve reverse graphs like libtorch",
          "[autodiff][parity][cpu][torch][indexed-write][inplace]") {
  const std::vector<double> leaf_values{1.0, 2.0, 3.0, 4.0};
  const std::vector<double> source_values{0.5, 1.5, -0.5, 2.0};
  const std::vector<std::int64_t> index_values{1, 0, 1, 0};
  const std::vector<double> seed_values{0.3, 0.7, 1.1, 1.5};
  auto options = at::TensorOptions().dtype(at::kDouble);

  SECTION("scatter_add_") {
    Series leaf = make_series(leaf_values, {2, 2}).to(ScalarType::Float64);
    Series source = make_series(source_values, {2, 2}).to(ScalarType::Float64);
    const Series index = make_series(index_values, {2, 2});
    leaf.tracks_slope_(true);
    source.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.scatter_add_(1, index, source);
    value.reverse(make_series(seed_values, {2, 2}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(leaf_values, options).reshape({2, 2}).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(source_values, options).reshape({2, 2}).set_requires_grad(true);
    const at::Tensor torch_index = at::tensor(index_values, at::TensorOptions().dtype(at::kLong)).reshape({2, 2});
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.scatter_add_(1, torch_index, torch_source);
    torch_value.backward(at::tensor(seed_values, options).reshape({2, 2}));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }

  SECTION("scatter_") {
    Series leaf = make_series(leaf_values, {2, 2}).to(ScalarType::Float64);
    Series source = make_series(source_values, {2, 2}).to(ScalarType::Float64);
    const Series index = make_series(index_values, {2, 2});
    leaf.tracks_slope_(true);
    source.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.scatter_(1, index, source);
    value.reverse(make_series(seed_values, {2, 2}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(leaf_values, options).reshape({2, 2}).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(source_values, options).reshape({2, 2}).set_requires_grad(true);
    const at::Tensor torch_index = at::tensor(index_values, at::TensorOptions().dtype(at::kLong)).reshape({2, 2});
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.scatter_(1, torch_index, torch_source);
    torch_value.backward(at::tensor(seed_values, options).reshape({2, 2}));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }

  SECTION("masked_scatter_") {
    const std::vector<std::uint8_t> mask_values{1, 0, 0, 1};
    Series leaf = make_series(leaf_values, {2, 2}).to(ScalarType::Float64);
    Series source = make_series(std::vector<double>{0.5, 1.5}, {2}).to(ScalarType::Float64);
    const Series mask = make_series(mask_values, {2, 2}).to(ScalarType::Bool);
    leaf.tracks_slope_(true);
    source.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.masked_scatter_(mask, source);
    value.reverse(make_series(seed_values, {2, 2}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(leaf_values, options).reshape({2, 2}).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(std::vector<double>{0.5, 1.5}, options).set_requires_grad(true);
    const at::Tensor torch_mask = at::tensor(mask_values, at::TensorOptions().dtype(at::kByte))
                                      .to(at::kBool)
                                      .reshape({2, 2});
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.masked_scatter_(torch_mask, torch_source);
    torch_value.backward(at::tensor(seed_values, options).reshape({2, 2}));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }
}

TEST_CASE("CPU leaf in-place indexed writes fail while recording like libtorch",
          "[autodiff][parity][cpu][torch][indexed-write][inplace]") {
  Series leaf = make_series(std::vector<double>{1.0, 2.0}, {2}).to(ScalarType::Float64);
  leaf.tracks_slope_(true);
  const Series index = make_series(std::vector<std::int64_t>{0}, {1});
  const Series source = make_series(std::vector<double>{3.0}, {1}).to(ScalarType::Float64);
  REQUIRE_THROWS_AS(leaf.scatter_add_(0, index, source), pluss::Error);

  at::Tensor torch_leaf = at::tensor(
      std::vector<double>{1.0, 2.0},
      at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
  const at::Tensor torch_index = at::tensor(
      std::vector<std::int64_t>{0},
      at::TensorOptions().dtype(at::kLong));
  const at::Tensor torch_source = at::tensor(
      std::vector<double>{3.0},
      at::TensorOptions().dtype(at::kDouble));
  REQUIRE_THROWS(torch_leaf.scatter_add_(0, torch_index, torch_source));
}

TEST_CASE("CPU version counter catches a saved tensor mutated after save like libtorch",
          "[autodiff][parity][cpu][torch][inplace][version]") {
  auto options = at::TensorOptions().dtype(at::kDouble);
  const std::vector<double> x_values{0.4, -0.7, 1.1, 0.5};
  const std::vector<double> seed{1.0, 1.0, 1.0, 1.0};

  SECTION("mutating a saved intermediate makes reverse throw") {
    Series x = make_series(x_values, {4}).to(ScalarType::Float64);
    x.tracks_slope_(true);
    Series w = x.mul(2.0);        // non-leaf
    Series y = w.mul(w);          // y's backward needs w (grad_w = 2*w*grad_y)
    w.add_(1.0);                  // legal in-place on a non-leaf; bumps w's version
    REQUIRE_THROWS_AS(y.sum().reverse(), pluss::Error);

    at::Tensor tx = at::tensor(x_values, options).set_requires_grad(true);
    at::Tensor tw = tx.mul(2.0);
    at::Tensor ty = tw.mul(tw);
    tw.add_(1.0);
    REQUIRE_THROWS(ty.sum().backward(at::tensor(seed, options)));
  }

  // A binary node version-guards a parent only when a backward rule that runs
  // actually reads its data (like libtorch's SavedVariable). So mutating `y`
  // after `z = y * c` with constant `c` -- whose grad wrt y is `grad * c`, never
  // touching y's data -- is allowed, and reverse still matches libtorch.
  SECTION("mutating a parent whose data no backward reads still reverses correctly") {
    Series x = make_series(x_values, {4}).to(ScalarType::Float64);
    x.tracks_slope_(true);
    Series y = x.add(1.0);
    Series z = y.mul(3.0);
    y.add_(10.0);
    z.reverse(make_series(seed, {4}).to(ScalarType::Float64));

    at::Tensor tx = at::tensor(x_values, options).set_requires_grad(true);
    at::Tensor ty = tx.add(1.0);
    at::Tensor tz = ty.mul(3.0);
    ty.add_(10.0);
    tz.backward(at::tensor(seed, options));

    require_close(x.sensitivity().host<double>(), torch_values(tx.grad()), 2e-12);
  }

  SECTION("mutating a genuinely-saved operand still throws (mul, both tracked)") {
    Series a = make_series(x_values, {4}).to(ScalarType::Float64);
    Series b = make_series(std::vector<double>{1.3, 0.6, -0.4, 2.1}, {4}).to(ScalarType::Float64);
    a.tracks_slope_(true);
    b.tracks_slope_(true);
    Series a1 = a.mul(1.0), b1 = b.mul(1.0);  // non-leaf handles to mutate
    Series z = a1.mul(b1);                    // grad wrt a1 reads b1, grad wrt b1 reads a1
    b1.add_(5.0);
    REQUIRE_THROWS_AS(z.sum().reverse(), pluss::Error);
  }
}

TEST_CASE("CPU axis-index writes preserve both reverse paths like libtorch",
          "[autodiff][parity][cpu][torch][indexed-write][axis]") {
  const std::vector<double> receiver_values{1.0, 2.0, 3.0, 4.0};
  const std::vector<double> source_values{0.5, 1.5};
  const std::vector<std::int64_t> index_values{2, 0};
  const std::vector<double> seed_values{0.3, 0.7, 1.1, 1.5};
  auto options = at::TensorOptions().dtype(at::kDouble);

  SECTION("index_add out-of-place") {
    Series receiver = make_series(receiver_values, {4}).to(ScalarType::Float64);
    Series source = make_series(source_values, {2}).to(ScalarType::Float64);
    const Series index = make_series(index_values, {2});
    receiver.tracks_slope_(true);
    source.tracks_slope_(true);
    const Series output = receiver.index_add(0, index, source, 1.7);
    output.reverse(make_series(seed_values, {4}).to(ScalarType::Float64));

    at::Tensor torch_receiver = at::tensor(receiver_values, options).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
    const at::Tensor torch_index = at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
    const at::Tensor torch_output = torch_receiver.index_add(0, torch_index, torch_source, 1.7);
    torch_output.backward(at::tensor(seed_values, options));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(receiver.sensitivity().host<double>(), torch_values(torch_receiver.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }

  SECTION("index_copy non-leaf in-place") {
    Series leaf = make_series(receiver_values, {4}).to(ScalarType::Float64);
    Series source = make_series(source_values, {2}).to(ScalarType::Float64);
    const Series index = make_series(index_values, {2});
    leaf.tracks_slope_(true);
    source.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.index_copy_(0, index, source);
    value.reverse(make_series(seed_values, {4}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(receiver_values, options).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
    const at::Tensor torch_index = at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.index_copy_(0, torch_index, torch_source);
    torch_value.backward(at::tensor(seed_values, options));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }

  SECTION("index_fill non-leaf in-place") {
    Series leaf = make_series(receiver_values, {4}).to(ScalarType::Float64);
    const Series index = make_series(index_values, {2});
    leaf.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.index_fill_(0, index, -3.0);
    value.reverse(make_series(seed_values, {4}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(receiver_values, options).set_requires_grad(true);
    const at::Tensor torch_index = at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.index_fill_(0, torch_index, -3.0);
    torch_value.backward(at::tensor(seed_values, options));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
  }

  SECTION("index_reduce mean non-leaf in-place") {
    const std::vector<std::int64_t> reduce_index_values{0, 0};
    Series leaf = make_series(receiver_values, {4}).to(ScalarType::Float64);
    Series source = make_series(source_values, {2}).to(ScalarType::Float64);
    const Series index = make_series(reduce_index_values, {2});
    leaf.tracks_slope_(true);
    source.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.index_reduce_(0, index, source, "mean", true);
    value.reverse(make_series(seed_values, {4}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(receiver_values, options).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
    const at::Tensor torch_index =
        at::tensor(reduce_index_values, at::TensorOptions().dtype(at::kLong));
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.index_reduce_(0, torch_index, torch_source, "mean", true);
    torch_value.backward(at::tensor(seed_values, options));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }
}

TEST_CASE("CPU index_put replacement and accumulation reverse match libtorch",
          "[autodiff][parity][cpu][torch][indexed-write][index-put]") {
  const std::vector<double> receiver_values{1.0, 2.0, 3.0, 4.0};
  const std::vector<double> source_values{10.0, 20.0};
  const std::vector<std::int64_t> index_values{2, 0};
  const std::vector<double> seed_values{0.3, 0.7, 1.1, 1.5};
  auto options = at::TensorOptions().dtype(at::kDouble);

  for (const bool accumulate : {false, true}) {
    DYNAMIC_SECTION("out-of-place accumulate=" << accumulate) {
      Series receiver = make_series(receiver_values, {4}).to(ScalarType::Float64);
      Series source = make_series(source_values, {2}).to(ScalarType::Float64);
      const Series index = make_series(index_values, {2});
      receiver.tracks_slope_(true);
      source.tracks_slope_(true);
      const Series output = receiver.index_put({index}, source, accumulate);
      output.reverse(make_series(seed_values, {4}).to(ScalarType::Float64));

      at::Tensor torch_receiver = at::tensor(receiver_values, options).set_requires_grad(true);
      at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
      const at::Tensor torch_index =
          at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
      const at::Tensor torch_output =
          torch_receiver.index_put({torch_index}, torch_source, accumulate);
      torch_output.backward(at::tensor(seed_values, options));

      require_close(output.host<double>(), torch_values(torch_output));
      require_close(receiver.sensitivity().host<double>(), torch_values(torch_receiver.grad()));
      require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
    }
  }

  SECTION("non-leaf in-place replacement") {
    Series leaf = make_series(receiver_values, {4}).to(ScalarType::Float64);
    Series source = make_series(source_values, {2}).to(ScalarType::Float64);
    const Series index = make_series(index_values, {2});
    leaf.tracks_slope_(true);
    source.tracks_slope_(true);
    Series value = leaf.mul(2.0);
    value.index_put_({index}, source, false);
    value.reverse(make_series(seed_values, {4}).to(ScalarType::Float64));

    at::Tensor torch_leaf = at::tensor(receiver_values, options).set_requires_grad(true);
    at::Tensor torch_source = at::tensor(source_values, options).set_requires_grad(true);
    const at::Tensor torch_index =
        at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
    at::Tensor torch_value = torch_leaf.mul(2.0);
    torch_value.index_put_({torch_index}, torch_source, false);
    torch_value.backward(at::tensor(seed_values, options));

    require_close(value.host<double>(), torch_values(torch_value));
    require_close(leaf.sensitivity().host<double>(), torch_values(torch_leaf.grad()));
    require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
  }

  SECTION("tracked leaf in-place write rejects") {
    Series leaf = make_series(receiver_values, {4}).to(ScalarType::Float64);
    leaf.tracks_slope_(true);
    const Series index = make_series(index_values, {2});
    const Series source = make_series(source_values, {2}).to(ScalarType::Float64);
    REQUIRE_THROWS_AS(leaf.index_put_({index}, source, false), pluss::Error);

    at::Tensor torch_leaf = at::tensor(receiver_values, options).set_requires_grad(true);
    const at::Tensor torch_index =
        at::tensor(index_values, at::TensorOptions().dtype(at::kLong));
    const at::Tensor torch_source = at::tensor(source_values, options);
    REQUIRE_THROWS(torch_leaf.index_put_({torch_index}, torch_source, false));
  }
}

TEST_CASE("CPU as_strided reverse follows libtorch overlap geometry",
          "[autodiff][parity][cpu][torch][operation-node][as-strided]") {
  auto options = at::TensorOptions().dtype(at::kDouble);

  SECTION("overlapping sliding window") {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8, 1.0, 1.2};
    Series input = make_series(values, {4}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.as_strided({3, 2}, {1, 1}, 0);
    output.reverse(make_series(seed_values, {3, 2}).to(ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
    const at::Tensor torch_output = torch_input.as_strided({3, 2}, {1, 1}, 0);
    torch_output.backward(at::tensor(seed_values, options).reshape({3, 2}));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("zero-stride expansion") {
    const std::vector<double> values{1.0, 2.0};
    const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8, 1.0, 1.2};
    Series input = make_series(values, {2}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.as_strided({3, 2}, {0, 1}, 0);
    output.reverse(make_series(seed_values, {3, 2}).to(ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
    const at::Tensor torch_output = torch_input.as_strided({3, 2}, {0, 1}, 0);
    torch_output.backward(at::tensor(seed_values, options).reshape({3, 2}));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("view of overlapping view") {
    const std::vector<double> values{1.0, 2.0};
    const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8};
    Series input = make_series(values, {2}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series expanded = input.as_strided({2, 2}, {0, 1}, 0);
    const Series output = expanded.as_strided({2, 2}, {1, 0}, 0);
    output.reverse(make_series(seed_values, {2, 2}).to(ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
    const at::Tensor torch_expanded = torch_input.as_strided({2, 2}, {0, 1}, 0);
    const at::Tensor torch_output = torch_expanded.as_strided({2, 2}, {1, 0}, 0);
    torch_output.backward(at::tensor(seed_values, options).reshape({2, 2}));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }
}

TEST_CASE("CPU as_strided_scatter preserves receiver and source reverse paths",
          "[autodiff][parity][cpu][torch][operation-node][as-strided-scatter]") {
  const std::vector<double> receiver_values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  const std::vector<double> source_values{10.0, 20.0, 30.0, 40.0};
  const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8, 1.0, 1.2};
  auto options = at::TensorOptions().dtype(at::kDouble);

  for (const std::int64_t offset : {0, 1}) {
    DYNAMIC_SECTION("storage_offset=" << offset) {
      Series receiver = make_series(receiver_values, {6}).to(ScalarType::Float64);
      Series source = make_series(source_values, {2, 2}).to(ScalarType::Float64);
      receiver.tracks_slope_(true);
      source.tracks_slope_(true);
      const Series output = receiver.as_strided_scatter(source, {2, 2}, {3, 1}, offset);
      output.reverse(make_series(seed_values, {6}).to(ScalarType::Float64));

      at::Tensor torch_receiver = at::tensor(receiver_values, options).set_requires_grad(true);
      at::Tensor torch_source =
          at::tensor(source_values, options).reshape({2, 2}).set_requires_grad(true);
      const at::Tensor torch_output =
          torch_receiver.as_strided_scatter(torch_source, {2, 2}, {3, 1}, offset);
      torch_output.backward(at::tensor(seed_values, options));

      require_close(output.host<double>(), torch_values(torch_output));
      require_close(receiver.sensitivity().host<double>(), torch_values(torch_receiver.grad()));
      require_close(source.sensitivity().host<double>(), torch_values(torch_source.grad()));
    }
  }

  SECTION("tracked non-contiguous receiver rejects") {
    Series leaf = make_series(receiver_values, {2, 3}).to(ScalarType::Float64);
    leaf.tracks_slope_(true);
    const Series receiver = leaf.transpose(0, 1);
    const Series source = make_series(source_values, {2, 2}).to(ScalarType::Float64);
    REQUIRE_THROWS_AS(receiver.as_strided_scatter(source, {2, 2}, {2, 1}, 0), pluss::Error);

    at::Tensor torch_leaf =
        at::tensor(receiver_values, options).reshape({2, 3}).set_requires_grad(true);
    const at::Tensor torch_receiver = torch_leaf.transpose(0, 1);
    const at::Tensor torch_source = at::tensor(source_values, options).reshape({2, 2});
    REQUIRE_THROWS(torch_receiver.as_strided_scatter(torch_source, {2, 2}, {2, 1}, 0));
  }
}

TEST_CASE("CPU value selections and finite replacement preserve operation-level reverse",
          "[autodiff][parity][cpu][torch][operation-node]") {
  auto options = at::TensorOptions().dtype(at::kDouble);

  SECTION("topk sorted false") {
    const std::vector<double> values{0.2, 3.0, -1.0, 1.7, 2.2, 0.8};
    const std::vector<double> seed_values{0.4, -0.7, 1.1, 0.2};
    Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.topk(2, 1, true, false);
    output.reverse(make_series(seed_values, {2, 2}).to(ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).reshape({2, 3}).set_requires_grad(true);
    const at::Tensor torch_output = std::get<0>(torch_input.topk(2, 1, true, false));
    torch_output.backward(at::tensor(seed_values, options).reshape({2, 2}));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("kthvalue keepdim false") {
    const std::vector<double> values{0.2, 3.0, -1.0, 1.7, 2.2, 0.8};
    const std::vector<double> seed_values{0.4, -0.7};
    Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.kthvalue(2, 1, false);
    output.reverse(make_series(seed_values, {2}).to(ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).reshape({2, 3}).set_requires_grad(true);
    const at::Tensor torch_output = std::get<0>(torch_input.kthvalue(2, 1, false));
    torch_output.backward(at::tensor(seed_values, options));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("nan_to_num") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::vector<double> values{1.0, nan, -2.0, inf, -inf};
    const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8, 1.0};
    Series input = make_series(values, {5}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.nan_to_num(9.0, 7.0, -7.0);
    output.reverse(make_series(seed_values, {5}).to(ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
    const at::Tensor torch_output = torch_input.nan_to_num(9.0, 7.0, -7.0);
    torch_output.backward(at::tensor(seed_values, options));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }
}

TEST_CASE("CPU cumprod reverse matches libtorch across zero topology",
          "[autodiff][parity][cpu][torch][operation-node][cumprod]") {
  const std::vector<std::vector<double>> cases{
      {2.0, 3.0, 4.0, 5.0, 0.5, -2.0},
      {2.0, 0.0, 4.0, 5.0, 0.5, -2.0},
      {2.0, 0.0, 4.0, 0.0, 0.5, -2.0},
  };
  const std::vector<double> seed_values{0.2, 0.4, 0.6, 0.8, 1.0, 1.2};
  auto options = at::TensorOptions().dtype(at::kDouble);

  for (std::size_t case_index = 0; case_index < cases.size(); ++case_index) {
    DYNAMIC_SECTION("real zero case " << case_index) {
      Series input = make_series(cases[case_index], {2, 3}).to(ScalarType::Float64);
      input.tracks_slope_(true);
      const Series output = input.cumprod(1, false);
      output.reverse(make_series(seed_values, {2, 3}).to(ScalarType::Float64));

      at::Tensor torch_input =
          at::tensor(cases[case_index], options).reshape({2, 3}).set_requires_grad(true);
      const at::Tensor torch_output = torch_input.cumprod(1);
      torch_output.backward(at::tensor(seed_values, options).reshape({2, 3}));

      require_close(output.host<double>(), torch_values(torch_output));
      require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
    }
  }

  SECTION("complex lane") {
    const std::vector<std::complex<double>> values{
        {1.0, 0.5}, {0.0, 0.0}, {2.0, -0.5}, {-1.0, 0.2}};
    const std::vector<std::complex<double>> seed_values_complex{
        {0.2, -0.3}, {0.4, 0.1}, {-0.5, 0.7}, {0.8, -0.2}};
    const std::vector<c10::complex<double>> torch_values_complex{
        {1.0, 0.5}, {0.0, 0.0}, {2.0, -0.5}, {-1.0, 0.2}};
    const std::vector<c10::complex<double>> torch_seed_values_complex{
        {0.2, -0.3}, {0.4, 0.1}, {-0.5, 0.7}, {0.8, -0.2}};

    Series input = complex_series(values, {4});
    input.tracks_slope_(true);
    const Series output = input.cumprod(0, false);
    output.reverse(complex_series(seed_values_complex, {4}));

    at::Tensor torch_input = torch_complex_series(torch_values_complex, {4}, true);
    const at::Tensor torch_output = torch_input.cumprod(0);
    torch_output.backward(torch_complex_series(torch_seed_values_complex, {4}));

    require_complex_close(output.host<std::complex<double>>(),
                          torch_complex_values(torch_output));
    require_complex_close(input.sensitivity().host<std::complex<double>>(),
                          torch_complex_values(torch_input.grad()));
  }
}

TEST_CASE("CPU median and quantile reverse match libtorch selection semantics",
          "[autodiff][parity][cpu][torch][operation-node][order-statistic]") {
  auto options = at::TensorOptions().dtype(at::kDouble);

  SECTION("even median differs from interpolated quantile") {
    const std::vector<double> values{1.0, 4.0, 2.0, 3.0};
    for (const bool use_quantile : {false, true}) {
      DYNAMIC_SECTION((use_quantile ? "quantile" : "median")) {
        Series input = make_series(values, {4}).to(ScalarType::Float64);
        input.tracks_slope_(true);
        const Series output = use_quantile ? input.quantile(0.5) : input.median();
        output.reverse(Series::full({}, 2.0, ScalarType::Float64));

        at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
        const at::Tensor torch_output =
            use_quantile ? torch_input.quantile(0.5) : torch_input.median();
        torch_output.backward(at::tensor(2.0, options));

        require_close(output.host<double>(), torch_values(torch_output));
        require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
      }
    }
  }

  SECTION("global median splits repeated winner") {
    const std::vector<double> values{1.0, 2.0, 2.0, 4.0};
    Series input = make_series(values, {4}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.median();
    output.reverse(Series::full({}, 1.0, ScalarType::Float64));

    at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
    const at::Tensor torch_output = torch_input.median();
    torch_output.backward();

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("NaN median and nanmedian") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<double> values{1.0, nan, 3.0, nan, 2.0};
    for (const bool ignore_nan : {false, true}) {
      DYNAMIC_SECTION((ignore_nan ? "nanmedian" : "median")) {
        Series input = make_series(values, {5}).to(ScalarType::Float64);
        input.tracks_slope_(true);
        const Series output = ignore_nan ? input.nanmedian() : input.median();
        output.reverse(Series::full({}, 1.0, ScalarType::Float64));

        at::Tensor torch_input = at::tensor(values, options).set_requires_grad(true);
        const at::Tensor torch_output =
            ignore_nan ? torch_input.nanmedian() : torch_input.median();
        torch_output.backward();

        require_close(output.host<double>(), torch_values(torch_output));
        require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
      }
    }
  }

  SECTION("axis nanmedian routes to selected positions") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<double> values{3.0, 1.0, 2.0, nan, 5.0, 4.0};
    const std::vector<double> seed_values{0.4, 0.8};
    Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.nanmedian(1);
    output.reverse(make_series(seed_values, {2}).to(ScalarType::Float64));

    at::Tensor torch_input =
        at::tensor(values, options).reshape({2, 3}).set_requires_grad(true);
    const at::Tensor torch_output = std::get<0>(torch_input.nanmedian(1));
    torch_output.backward(at::tensor(seed_values, options));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("axis mode routes to the kernel-selected duplicate") {
    const std::vector<double> values{3.0, 1.0, 1.0, 2.0, 5.0, 2.0, 2.0, 4.0};
    const std::vector<double> seed_values{0.4, 0.8};
    Series input = make_series(values, {2, 4}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.mode(1);
    output.reverse(make_series(seed_values, {2}).to(ScalarType::Float64));

    at::Tensor torch_input =
        at::tensor(values, options).reshape({2, 4}).set_requires_grad(true);
    const at::Tensor torch_output = std::get<0>(torch_input.mode(1));
    torch_output.backward(at::tensor(seed_values, options));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }
}

TEST_CASE("CPU sequence transforms retain reverse graphs through empty results",
          "[autodiff][parity][cpu][torch][operation-node][sequence]") {
  const auto options = at::TensorOptions().dtype(at::kDouble);

  SECTION("ediff1d is the flattened first difference") {
    const std::vector<double> values{1.0, 4.0, 2.0, 8.0};
    const std::vector<double> seed_values{0.2, -0.5, 0.7};
    Series input = make_series(values, {2, 2}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.ediff1d();
    output.reverse(make_series(seed_values, {3}).to(ScalarType::Float64));

    at::Tensor torch_input =
        at::tensor(values, options).reshape({2, 2}).set_requires_grad(true);
    const at::Tensor torch_output = torch_input.flatten().diff();
    torch_output.backward(at::tensor(seed_values, options));

    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("empty diff remains connected") {
    Series input = make_series({3.0}, {1}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    input.diff(1, 0).sum().reverse();

    at::Tensor torch_input = at::tensor({3.0}, options).set_requires_grad(true);
    torch_input.diff().sum().backward();
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("zero repeat remains connected") {
    Series input = make_series({1.0, 2.0}, {2}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    input.repeat({0}).sum().reverse();

    at::Tensor torch_input = at::tensor({1.0, 2.0}, options).set_requires_grad(true);
    torch_input.repeat({0}).sum().backward();
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("zero repeat_interleave remains connected") {
    Series input = make_series({1.0, 2.0}, {2}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    input.repeat_interleave(0, 0).sum().reverse();

    at::Tensor torch_input = at::tensor({1.0, 2.0}, options).set_requires_grad(true);
    torch_input.repeat_interleave(0, 0).sum().backward();
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("unfold uses PyTorch dimension order and accumulates overlap") {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    Series input = make_series(values, {3, 2}).to(ScalarType::Float64);
    input.tracks_slope_(true);
    const Series output = input.unfold(0, 2, 1);
    output.sum().reverse();

    at::Tensor torch_input =
        at::tensor(values, options).reshape({3, 2}).set_requires_grad(true);
    const at::Tensor torch_output = torch_input.unfold(0, 2, 1);
    torch_output.sum().backward();

    REQUIRE(std::vector<std::int64_t>(output.sizes().begin(), output.sizes().end()) ==
            torch_output.sizes().vec());
    require_close(output.host<double>(), torch_values(torch_output));
    require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
  }

  SECTION("oversized unfold fails like PyTorch") {
    Series input = make_series({1.0, 2.0}, {2}).to(ScalarType::Float64);
    at::Tensor torch_input = at::tensor({1.0, 2.0}, options).set_requires_grad(true);
    REQUIRE_THROWS_AS(input.unfold(0, 3, 1), pluss::Error);
    REQUIRE_THROWS(torch_input.unfold(0, 3, 1));
  }
}

TEST_CASE("CPU NaN population variance retains its operation graph",
          "[autodiff][parity][cpu][torch][operation-node][nanvar]") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<double> values{1.0, nan, 5.0, 2.0, 4.0, nan};
  const std::vector<double> seed_values{0.3, -0.7};

  Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
  input.tracks_slope_(true);
  const Series output = input.nanvar(1);
  output.reverse(make_series(seed_values, {2}).to(ScalarType::Float64));

  at::Tensor torch_input =
      at::tensor(values, at::TensorOptions().dtype(at::kDouble))
          .reshape({2, 3})
          .set_requires_grad(true);
  const at::Tensor observed = ~torch_input.isnan();
  const at::Tensor clean = torch_input.masked_fill(~observed, 0.0);
  const at::Tensor count = observed.sum(1, true);
  const at::Tensor mean = clean.sum(1, true).div(count);
  const at::Tensor centered = torch_input.sub(mean).masked_fill(~observed, 0.0);
  const at::Tensor torch_output = centered.square().sum(1).div(count.squeeze(1));
  torch_output.backward(at::tensor(seed_values, at::TensorOptions().dtype(at::kDouble)));

  require_close(output.host<double>(), torch_values(torch_output));
  require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
}

TEST_CASE("CPU cumulative extrema route gradients through selected indices",
          "[autodiff][parity][cpu][torch][operation-node][cum-extrema]") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<double> values{2.0, 2.0, 1.0, 3.0, nan, 3.0};
  const std::vector<double> seed_values{0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
  const auto options = at::TensorOptions().dtype(at::kDouble);

  for (const bool maximum : {false, true}) {
    DYNAMIC_SECTION((maximum ? "cummax" : "cummin")) {
      Series input = make_series(values, {2, 3}).to(ScalarType::Float64);
      input.tracks_slope_(true);
      const Series output = maximum ? input.cummax(1) : input.cummin(1);
      output.reverse(make_series(seed_values, {2, 3}).to(ScalarType::Float64));

      at::Tensor torch_input =
          at::tensor(values, options).reshape({2, 3}).set_requires_grad(true);
      const at::Tensor torch_output = maximum
          ? std::get<0>(torch_input.cummax(1))
          : std::get<0>(torch_input.cummin(1));
      torch_output.backward(at::tensor(seed_values, options).reshape({2, 3}));

      require_close(output.host<double>(), torch_values(torch_output));
      require_close(input.sensitivity().host<double>(), torch_values(torch_input.grad()));
    }
  }
}

// ---------------------------------------------------------------------------
// Track C: forward-mode (DualTrace/jvp) per-dim reductions and structural
// ops, checked against libtorch's own dual-number forward-mode AD
// (`_make_dual`/`_unpack_dual`), not a hand re-derivation. All of these are
// linear in x (JVP of a linear op = the same op applied to the tangent), so
// this is really checking the *implementation* -- dim handling, shapes --
// against libtorch's, not the underlying math.
// ---------------------------------------------------------------------------
TEST_CASE("CPU forward-mode per-dim reductions and structural ops match libtorch jvp",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  const std::vector<double> xv{0.6, -1.1, 0.25, 1.8, -0.4, 0.9, -0.7, 1.3, 0.15, -1.5, 0.55, 2.1};
  const std::vector<double> tv{0.3, -0.2, 0.5, -0.4, 0.1, 0.6, -0.5, 0.2, -0.3, 0.4, -0.1, 0.7};
  const std::vector<std::int64_t> shape{2, 3, 2};

  struct Case {
    std::string name;
    std::function<DualTrace(const DualTrace&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
  };
  const std::vector<Case> cases{
      {"sum_dim1_keepdim",
       [](const DualTrace& x) { return pluss::subsp::math::diff::sum(x, 1, true); },
       [](const at::Tensor& x) { return x.sum(1, true); }},
      {"sum_dim1_no_keepdim",
       [](const DualTrace& x) { return pluss::subsp::math::diff::sum(x, 1, false); },
       [](const at::Tensor& x) { return x.sum(1, false); }},
      {"mean_dim0",
       [](const DualTrace& x) { return pluss::subsp::math::diff::mean(x, 0, false); },
       [](const at::Tensor& x) { return x.mean(0, false); }},
      {"sum_multi_dims",
       [](const DualTrace& x) { return pluss::subsp::math::diff::sum(x, std::vector<std::int64_t>{0, 2}, false); },
       [](const at::Tensor& x) { return x.sum(std::vector<std::int64_t>{0, 2}, false); }},
      {"mean_multi_dims_keepdim",
       [](const DualTrace& x) { return pluss::subsp::math::diff::mean(x, std::vector<std::int64_t>{0, 2}, true); },
       [](const at::Tensor& x) { return x.mean(std::vector<std::int64_t>{0, 2}, true); }},
      {"reshape",
       [](const DualTrace& x) { return pluss::subsp::math::diff::reshape(x, std::vector<std::int64_t>{4, 3}); },
       [](const at::Tensor& x) { return x.reshape({4, 3}); }},
      {"transpose",
       [](const DualTrace& x) { return pluss::subsp::math::diff::transpose(x, 0, 2); },
       [](const at::Tensor& x) { return x.transpose(0, 2); }},
      {"squeeze_dim",
       [](const DualTrace& x) { return pluss::subsp::math::diff::squeeze(pluss::subsp::math::diff::unsqueeze(x, 1), 1); },
       [](const at::Tensor& x) { return x.unsqueeze(1).squeeze(1); }},
      {"unsqueeze",
       [](const DualTrace& x) { return pluss::subsp::math::diff::unsqueeze(x, 0); },
       [](const at::Tensor& x) { return x.unsqueeze(0); }},
      {"flatten",
       [](const DualTrace& x) { return pluss::subsp::math::diff::flatten(x, 0, 1); },
       [](const at::Tensor& x) { return x.flatten(0, 1); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      const Series x = make_series(xv, shape).to(ScalarType::Float64);
      const Series t = make_series(tv, shape).to(ScalarType::Float64);
      const auto [value, tangent] = jvp(c.pop, x, t);

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
      at::Tensor tt = at::tensor(tv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
      const auto [torch_value, torch_tangent] = torch_jvp(c.top, tx, tt);

      require_close(value.host<double>(), torch_values(torch_value), 2e-12);
      require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
    }
  }
}

// Track C batch 2: amax/amin (a selection -- deliberately includes a tie in
// row 2 to exercise the "split evenly across tied max positions" convention,
// same as the reverse-mode rule) and var/std (a smooth quadratic), all
// checked against libtorch's own forward-mode AD like the batch-1 ops above.
TEST_CASE("CPU forward-mode amax/amin/var/std match libtorch jvp",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  // row 0: unique max/min. row 1: tied max (1.8 appears twice).
  const std::vector<double> xv{0.5, -1.2, 2.1, 0.3, -0.4, 1.8, 1.8, 0.9};
  const std::vector<double> tv{0.2, -0.3, 0.6, 0.1, -0.5, 0.4, -0.2, 0.3};
  const std::vector<std::int64_t> shape{2, 4};

  struct Case {
    std::string name;
    std::function<DualTrace(const DualTrace&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
  };
  const std::vector<Case> cases{
      {"amax_dim1", [](const DualTrace& x) { return pluss::subsp::math::diff::amax(x, 1, false); },
       [](const at::Tensor& x) { return at::amax(x, 1, false); }},
      {"amax_dim1_keepdim", [](const DualTrace& x) { return pluss::subsp::math::diff::amax(x, 1, true); },
       [](const at::Tensor& x) { return at::amax(x, 1, true); }},
      {"amin_dim1", [](const DualTrace& x) { return pluss::subsp::math::diff::amin(x, 1, false); },
       [](const at::Tensor& x) { return at::amin(x, 1, false); }},
      {"var_dim1_biased", [](const DualTrace& x) { return pluss::subsp::math::diff::var(x, 1, false, false); },
       [](const at::Tensor& x) { return at::var(x, 1, /*unbiased=*/false, /*keepdim=*/false); }},
      {"var_dim1_unbiased_keepdim", [](const DualTrace& x) { return pluss::subsp::math::diff::var(x, 1, true, true); },
       [](const at::Tensor& x) { return at::var(x, 1, /*unbiased=*/true, /*keepdim=*/true); }},
      {"std_dim1_unbiased", [](const DualTrace& x) { return pluss::subsp::math::diff::std(x, 1, true, false); },
       [](const at::Tensor& x) { return at::std(x, 1, /*unbiased=*/true, /*keepdim=*/false); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      const Series x = make_series(xv, shape).to(ScalarType::Float64);
      const Series t = make_series(tv, shape).to(ScalarType::Float64);
      const auto [value, tangent] = jvp(c.pop, x, t);

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
      at::Tensor tt = at::tensor(tv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
      const auto [torch_value, torch_tangent] = torch_jvp(c.top, tx, tt);

      require_close(value.host<double>(), torch_values(torch_value), 2e-10);
      require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-9);
    }
  }
}

// Track C batch 3: cumsum (linear) and cumprod (genuine singularity at zero
// elements -- row 1 below has a zero mid-row specifically to exercise the
// zero-safe branch, mirroring PyTorch's own `cumprod_jvp`), checked against
// libtorch's forward-mode AD like every other Track C batch.
TEST_CASE("CPU forward-mode cumsum/cumprod match libtorch jvp",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  // row 0: no zeros. row 1: a zero at index 2 (exercises cumprod_jvp's
  // "first zero" special-case branch).
  const std::vector<double> xv{0.6, -1.1, 0.3, 1.8, -0.4, 0.9, -0.7, 0.0, 1.3, -1.5};
  const std::vector<double> tv{0.2, -0.3, 0.5, -0.4, 0.1, -0.5, 0.4, 0.6, -0.2, 0.3};
  const std::vector<std::int64_t> shape{2, 5};

  struct Case {
    std::string name;
    std::function<DualTrace(const DualTrace&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
  };
  const std::vector<Case> cases{
      {"cumsum_dim1", [](const DualTrace& x) { return pluss::subsp::math::diff::cumsum(x, 1); },
       [](const at::Tensor& x) { return x.cumsum(1); }},
      {"cumprod_dim1", [](const DualTrace& x) { return pluss::subsp::math::diff::cumprod(x, 1); },
       [](const at::Tensor& x) { return x.cumprod(1); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      const Series x = make_series(xv, shape).to(ScalarType::Float64);
      const Series t = make_series(tv, shape).to(ScalarType::Float64);
      const auto [value, tangent] = jvp(c.pop, x, t);

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
      at::Tensor tt = at::tensor(tv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
      const auto [torch_value, torch_tangent] = torch_jvp(c.top, tx, tt);

      require_close(value.host<double>(), torch_values(torch_value), 2e-10);
      require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-9);
    }
  }
}

// Track C batch 4: index_select/gather (unary in x, index is a plain
// non-differentiable Series) checked with the single-input jvp/torch_jvp;
// scatter/scatter_add (self AND src both differentiable) and pile (torch's
// `stack`, N differentiable inputs) checked with the multi-input forms.
TEST_CASE("CPU forward-mode index_select/gather match libtorch jvp",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  const std::vector<double> xv{0.6, -1.1, 0.3, 1.8, -0.4, 0.9, -0.7, 0.2, 1.3, -1.5, 0.55, 2.1};
  const std::vector<double> tv{0.2, -0.3, 0.5, -0.4, 0.1, 0.6, -0.5, 0.2, -0.3, 0.4, -0.1, 0.7};
  const std::vector<std::int64_t> shape{4, 3};
  const std::vector<std::int64_t> idx0{2, 0, 3};
  const std::vector<std::int64_t> gather_idx{2, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 1};

  const Series x = make_series(xv, shape).to(ScalarType::Float64);
  const Series t = make_series(tv, shape).to(ScalarType::Float64);
  at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
  at::Tensor tt = at::tensor(tv, at::TensorOptions().dtype(at::kDouble)).reshape(shape);

  SECTION("index_select dim0") {
    const Series index = make_series(idx0, {3});
    at::Tensor tidx = at::tensor(idx0, at::TensorOptions().dtype(at::kLong));
    const auto [value, tangent] = jvp(
        [&](const DualTrace& z) { return pluss::subsp::math::diff::index_select(z, 0, index); }, x, t);
    const auto [torch_value, torch_tangent] =
        torch_jvp([&](const at::Tensor& z) { return z.index_select(0, tidx); }, tx, tt);
    require_close(value.host<double>(), torch_values(torch_value), 2e-12);
    require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
  }

  SECTION("gather dim1") {
    const Series index = make_series(gather_idx, shape);
    at::Tensor tidx = at::tensor(gather_idx, at::TensorOptions().dtype(at::kLong)).reshape(shape);
    const auto [value, tangent] = jvp(
        [&](const DualTrace& z) { return pluss::subsp::math::diff::gather(z, 1, index); }, x, t);
    const auto [torch_value, torch_tangent] =
        torch_jvp([&](const at::Tensor& z) { return z.gather(1, tidx); }, tx, tt);
    require_close(value.host<double>(), torch_values(torch_value), 2e-12);
    require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
  }
}

TEST_CASE("CPU forward-mode scatter/scatter_add/pile match libtorch jvp",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  const std::vector<std::int64_t> shape{2, 3};
  const std::vector<double> self_v{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  const std::vector<double> src_v{0.4, -0.2, 0.7, -0.5, 0.3, 0.9};
  const std::vector<double> self_t{0.1, -0.2, 0.3, -0.1, 0.2, -0.3};
  const std::vector<double> src_t{-0.4, 0.2, -0.1, 0.3, -0.2, 0.1};
  const std::vector<std::int64_t> scatter_idx{1, 0, 2, 0, 2, 1};

  const Series self = make_series(self_v, shape).to(ScalarType::Float64);
  const Series src = make_series(src_v, shape).to(ScalarType::Float64);
  const Series self_tan = make_series(self_t, shape).to(ScalarType::Float64);
  const Series src_tan = make_series(src_t, shape).to(ScalarType::Float64);
  const Series index = make_series(scatter_idx, shape);

  at::Tensor t_self = at::tensor(self_v, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
  at::Tensor t_src = at::tensor(src_v, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
  at::Tensor t_self_tan = at::tensor(self_t, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
  at::Tensor t_src_tan = at::tensor(src_t, at::TensorOptions().dtype(at::kDouble)).reshape(shape);
  at::Tensor t_idx = at::tensor(scatter_idx, at::TensorOptions().dtype(at::kLong)).reshape(shape);

  SECTION("scatter dim1") {
    const auto [value, tangent] = jvp(
        [&](const std::vector<DualTrace>& xs) {
          return pluss::subsp::math::diff::scatter(xs[0], 1, index, xs[1]);
        },
        std::vector<Series>{self, src},
        std::vector<Series>{self_tan, src_tan});
    const auto [torch_value, torch_tangent] = torch_jvp_n(
        [&](const std::vector<at::Tensor>& xs) { return xs[0].scatter(1, t_idx, xs[1]); },
        {t_self, t_src},
        {t_self_tan, t_src_tan});
    require_close(value.host<double>(), torch_values(torch_value), 2e-12);
    require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
  }

  SECTION("scatter_add dim1") {
    const auto [value, tangent] = jvp(
        [&](const std::vector<DualTrace>& xs) {
          return pluss::subsp::math::diff::scatter_add(xs[0], 1, index, xs[1]);
        },
        std::vector<Series>{self, src},
        std::vector<Series>{self_tan, src_tan});
    const auto [torch_value, torch_tangent] = torch_jvp_n(
        [&](const std::vector<at::Tensor>& xs) { return xs[0].scatter_add(1, t_idx, xs[1]); },
        {t_self, t_src},
        {t_self_tan, t_src_tan});
    require_close(value.host<double>(), torch_values(torch_value), 2e-12);
    require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
  }

  SECTION("pile (torch stack) dim0, 3 inputs") {
    const Series c = self.mul(2.0);
    const Series c_tan = self_tan.mul(2.0);
    at::Tensor t_c = t_self.mul(2.0);
    at::Tensor t_c_tan = t_self_tan.mul(2.0);

    const auto [value, tangent] = jvp(
        [&](const std::vector<DualTrace>& xs) { return pluss::subsp::math::diff::pile(xs, 0); },
        std::vector<Series>{self, src, c},
        std::vector<Series>{self_tan, src_tan, c_tan});
    const auto [torch_value, torch_tangent] = torch_jvp_n(
        [&](const std::vector<at::Tensor>& xs) { return at::stack(xs, 0); },
        {t_self, t_src, t_c},
        {t_self_tan, t_src_tan, t_c_tan});
    require_close(value.host<double>(), torch_values(torch_value), 2e-12);
    require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
  }
}

// Track C batch 5: the Track B fused activation family now has forward-mode
// duals too, checked against libtorch's own forward-AD like every other
// batch. Sample spans both sides of every piecewise op's kink (0 for elu/
// celu/selu, +-3 for hardsigmoid/hardswish, the threshold value for
// threshold) to exercise both branches.
TEST_CASE("CPU forward-mode elu/celu/selu/hardsigmoid/hardswish/mish/threshold match libtorch jvp",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  const std::vector<double> xv{-4.5, -3.4, -1.7, -0.4, -0.05, 0.2, 0.9, 2.6, 3.4, 4.2};
  const std::vector<double> tv{0.3, -0.4, 0.5, -0.2, 0.6, -0.5, 0.4, -0.3, 0.2, -0.1};

  struct Case {
    std::string name;
    std::function<DualTrace(const DualTrace&)> pop;
    std::function<at::Tensor(const at::Tensor&)> top;
  };
  const std::vector<Case> cases{
      {"elu_default", [](const DualTrace& x) { return pluss::subsp::math::diff::elu(x); },
       [](const at::Tensor& x) { return at::elu(x); }},
      {"elu_alpha", [](const DualTrace& x) { return pluss::subsp::math::diff::elu(x, 0.7); },
       [](const at::Tensor& x) { return at::elu(x, 0.7); }},
      {"elu_full", [](const DualTrace& x) { return pluss::subsp::math::diff::elu(x, 1.3, 0.8, 1.1); },
       [](const at::Tensor& x) { return at::elu(x, 1.3, 0.8, 1.1); }},
      {"celu", [](const DualTrace& x) { return pluss::subsp::math::diff::celu(x, 0.6); },
       [](const at::Tensor& x) { return at::celu(x, 0.6); }},
      {"selu", [](const DualTrace& x) { return pluss::subsp::math::diff::selu(x); },
       [](const at::Tensor& x) { return at::selu(x); }},
      {"hardsigmoid", [](const DualTrace& x) { return pluss::subsp::math::diff::hardsigmoid(x); },
       [](const at::Tensor& x) { return at::hardsigmoid(x); }},
      {"hardswish", [](const DualTrace& x) { return pluss::subsp::math::diff::hardswish(x); },
       [](const at::Tensor& x) { return at::hardswish(x); }},
      {"mish", [](const DualTrace& x) { return pluss::subsp::math::diff::mish(x); },
       [](const at::Tensor& x) { return at::mish(x); }},
      {"threshold", [](const DualTrace& x) { return pluss::subsp::math::diff::threshold(x, 0.0, -1.0); },
       [](const at::Tensor& x) { return at::threshold(x, 0.0, -1.0); }},
  };

  for (const Case& c : cases) {
    DYNAMIC_SECTION(c.name) {
      const Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
      const Series t = make_series(tv, {static_cast<std::int64_t>(tv.size())}).to(ScalarType::Float64);
      const auto [value, tangent] = jvp(c.pop, x, t);

      at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble));
      at::Tensor tt = at::tensor(tv, at::TensorOptions().dtype(at::kDouble));
      const auto [torch_value, torch_tangent] = torch_jvp(c.top, tx, tt);

      require_close(value.host<double>(), torch_values(torch_value), 2e-10);
      require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-9);
    }
  }
}

// rrelu/dropout forward-mode: eval mode (rrelu training=false; dropout p=0)
// is deterministic and checked exactly against libtorch. Training mode's
// random draw differs between the two RNG streams (same reason the Track B
// reverse-mode rrelu test can't compare training-mode values directly), so
// it's checked for internal self-consistency instead: the realized
// per-element slope/scale is recoverable from pluss's own primal (primal_i
// / x_i on the branch where it was applied), and the returned tangent must
// equal exactly that times the input tangent.
TEST_CASE("CPU forward-mode rrelu/dropout: eval mode matches libtorch, training mode is self-consistent",
          "[autodiff][parity][cpu][torch][forward-mode][jvp]") {
  const std::vector<double> xv{-2.1, -0.9, -0.4, -0.05, 0.2, 0.9, 2.3};
  const std::vector<double> tv{0.3, -0.4, 0.5, -0.2, 0.6, -0.5, 0.4};
  const Series x = make_series(xv, {static_cast<std::int64_t>(xv.size())}).to(ScalarType::Float64);
  const Series t = make_series(tv, {static_cast<std::int64_t>(tv.size())}).to(ScalarType::Float64);

  SECTION("rrelu eval mode matches libtorch exactly") {
    const auto [value, tangent] = jvp(
        [](const DualTrace& z) { return pluss::subsp::math::diff::rrelu(z, 0.2, 0.8, false); }, x, t);
    at::Tensor tx = at::tensor(xv, at::TensorOptions().dtype(at::kDouble));
    at::Tensor tt = at::tensor(tv, at::TensorOptions().dtype(at::kDouble));
    const auto [torch_value, torch_tangent] =
        torch_jvp([](const at::Tensor& z) { return at::rrelu(z, 0.2, 0.8, false); }, tx, tt);
    require_close(value.host<double>(), torch_values(torch_value), 2e-12);
    require_close(tangent.host<double>(), torch_values(torch_tangent), 2e-12);
  }

  SECTION("rrelu training mode reproduces its own realized slope") {
    const auto [value, tangent] = jvp(
        [](const DualTrace& z) { return pluss::subsp::math::diff::rrelu(z, 0.2, 0.8, true); }, x, t);
    const auto xs = x.host<double>();
    const auto ys = value.host<double>();
    const auto tv_ = t.host<double>();
    const auto got_t = tangent.host<double>();
    for (std::size_t i = 0; i < xs.size(); ++i) {
      const double slope = xs[i] > 0.0 ? 1.0 : ys[i] / xs[i];
      REQUIRE(got_t[i] == Approx(slope * tv_[i]).margin(1e-9));
    }
  }

  SECTION("dropout p=0 is the identity, matching libtorch") {
    const auto [value, tangent] = jvp(
        [](const DualTrace& z) { return pluss::subsp::math::diff::dropout(z, 0.0, true); }, x, t);
    require_close(value.host<double>(), x.host<double>(), 2e-12);
    require_close(tangent.host<double>(), t.host<double>(), 2e-12);
  }

  SECTION("dropout training mode reproduces its own realized scale") {
    const auto [value, tangent] = jvp(
        [](const DualTrace& z) { return pluss::subsp::math::diff::dropout(z, 0.4, true); }, x, t);
    const auto xs = x.host<double>();
    const auto ys = value.host<double>();
    const auto tv_ = t.host<double>();
    const auto got_t = tangent.host<double>();
    for (std::size_t i = 0; i < xs.size(); ++i) {
      const double scale = ys[i] / xs[i];
      REQUIRE(got_t[i] == Approx(scale * tv_[i]).margin(1e-9));
    }
  }
}

// Track C batch 6: linalg forward-mode, checked against libtorch's own
// dual-number forward-AD like every other Track C batch -- the same
// well-conditioned matrices the reverse-mode linalg oracle test (above)
// already uses, so a formula bug shows up as a real mismatch, not a
// near-singular numerical artifact.
TEST_CASE("CPU forward-mode linalg matches libtorch jvp", "[autodiff][parity][cpu][torch][forward-mode][jvp][linalg]") {
  namespace la = pluss::subsp::linearalg;
  const std::int64_t N = 3;
  const std::vector<double> m{1.2, 0.3, -0.4, 0.1, 1.5, 0.2, -0.3, 0.25, 1.1};
  const std::vector<double> dm{0.05, -0.1, 0.2, 0.15, -0.05, 0.1, -0.2, 0.1, 0.05};
  std::vector<double> spd(static_cast<std::size_t>(N * N), 0.0);
  for (std::int64_t i = 0; i < N; ++i) {
    for (std::int64_t j = 0; j < N; ++j) {
      double v = (i == j) ? static_cast<double>(N) : 0.0;
      for (std::int64_t k = 0; k < N; ++k) {
        v += m[static_cast<std::size_t>(i * N + k)] * m[static_cast<std::size_t>(j * N + k)];
      }
      spd[static_cast<std::size_t>(i * N + j)] = v;
    }
  }
  auto opt = at::TensorOptions().dtype(at::kDouble);

  SECTION("det") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::det(z); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] = torch_jvp([](const at::Tensor& z) { return at::linalg_det(z); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("logdet") {
    const Series x = make_series(spd, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::logdet(z); }, x, t);
    at::Tensor tx = at::tensor(spd, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] = torch_jvp([](const at::Tensor& z) { return at::logdet(z); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("slogdet") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::slogdet(z); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] = torch_jvp(
        [](const at::Tensor& z) {
          auto [sign, logabs] = at::linalg_slogdet(z);
          return at::stack({sign, logabs}, -1);
        },
        tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("inverse") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::inverse(z); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] = torch_jvp([](const at::Tensor& z) { return at::linalg_inv(z); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("pinverse") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::pinverse(z); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] = torch_jvp([](const at::Tensor& z) { return at::linalg_pinv(z); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-8);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-7);
  }

  SECTION("cholesky") {
    const Series x = make_series(spd, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::cholesky(z, false); }, x, t);
    at::Tensor tx = at::tensor(spd, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] =
        torch_jvp([](const at::Tensor& z) { return at::linalg_cholesky(z, false); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("matrix_exp") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::matrix_exp(z); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] =
        torch_jvp([](const at::Tensor& z) { return at::linalg_matrix_exp(z); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-8);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-7);
  }

  SECTION("matrix_power3") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::matrix_power(z, 3); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] =
        torch_jvp([](const at::Tensor& z) { return at::linalg_matrix_power(z, 3); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("matrix_power_neg2") {
    const Series x = make_series(m, {N, N}).to(ScalarType::Float64);
    const Series t = make_series(dm, {N, N}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::matrix_power(z, -2); }, x, t);
    at::Tensor tx = at::tensor(m, opt).reshape({N, N});
    at::Tensor tt = at::tensor(dm, opt).reshape({N, N});
    const auto [tv, tt_out] =
        torch_jvp([](const at::Tensor& z) { return at::linalg_matrix_power(z, -2); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("triangular_solve") {
    const std::vector<double> factor_v{2.0, 0.0, 0.0, 0.5, 1.5, 0.0, -0.3, 0.4, 1.2};
    const std::vector<double> rhs_v{1.0, 0.5, -0.2, 0.3, 1.1, 0.4};
    const std::vector<double> dfactor_v{0.1, 0.0, 0.0, 0.05, -0.1, 0.0, 0.02, 0.03, -0.05};
    const std::vector<double> drhs_v{0.2, -0.1, 0.05, 0.1, -0.2, 0.15};
    const Series factor = make_series(factor_v, {3, 3}).to(ScalarType::Float64);
    const Series rhs = make_series(rhs_v, {3, 2}).to(ScalarType::Float64);
    const Series dfactor = make_series(dfactor_v, {3, 3}).to(ScalarType::Float64);
    const Series drhs = make_series(drhs_v, {3, 2}).to(ScalarType::Float64);
    const auto [value, tangent] = jvp(
        [](const std::vector<DualTrace>& xs) { return pluss::subsp::math::diff::triangular_solve(xs[0], xs[1], false, false); },
        std::vector<Series>{factor, rhs},
        std::vector<Series>{dfactor, drhs});

    at::Tensor tfactor = at::tensor(factor_v, opt).reshape({3, 3});
    at::Tensor trhs = at::tensor(rhs_v, opt).reshape({3, 2});
    at::Tensor tdfactor = at::tensor(dfactor_v, opt).reshape({3, 3});
    at::Tensor tdrhs = at::tensor(drhs_v, opt).reshape({3, 2});
    const auto [tv, tt_out] = torch_jvp_n(
        [](const std::vector<at::Tensor>& xs) { return at::linalg_solve_triangular(xs[0], xs[1], false); },
        {tfactor, trhs},
        {tdfactor, tdrhs});
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }

  SECTION("qr Q and R (m >= n)") {
    const std::vector<double> qv{1.2, -0.3, 0.5, 1.7, -0.8, 0.9};
    const std::vector<double> dqv{0.1, -0.05, 0.15, -0.1, 0.05, 0.2};
    const Series x = make_series(qv, {3, 2}).to(ScalarType::Float64);
    const Series t = make_series(dqv, {3, 2}).to(ScalarType::Float64);
    at::Tensor tx = at::tensor(qv, opt).reshape({3, 2});
    at::Tensor tt = at::tensor(dqv, opt).reshape({3, 2});

    for (const bool q_path : {true, false}) {
      DYNAMIC_SECTION((q_path ? "Q" : "R")) {
        const auto [value, tangent] = jvp(
            [q_path](const DualTrace& z) { return q_path ? pluss::subsp::math::diff::qr_q(z, true) : pluss::subsp::math::diff::qr_r(z, true); }, x, t);
        const auto [tv, tt_out] = torch_jvp(
            [q_path](const at::Tensor& z) {
              auto [q, r] = at::linalg_qr(z, "reduced");
              return q_path ? q : r;
            },
            tx, tt);
        require_close(value.host<double>(), torch_values(tv), 2e-9);
        require_close(tangent.host<double>(), torch_values(tt_out), 2e-7);
      }
    }
  }

  SECTION("svd_s") {
    const std::vector<double> sv{2.0, -0.4, 0.7, 1.4, -0.6, 0.2};
    const std::vector<double> dsv{0.1, -0.05, 0.08, -0.03, 0.06, -0.1};
    const Series x = make_series(sv, {3, 2}).to(ScalarType::Float64);
    const Series t = make_series(dsv, {3, 2}).to(ScalarType::Float64);
    const auto [value, tangent] =
        jvp([](const DualTrace& z) { return pluss::subsp::math::diff::svd_s(z, false); }, x, t);
    at::Tensor tx = at::tensor(sv, opt).reshape({3, 2});
    at::Tensor tt = at::tensor(dsv, opt).reshape({3, 2});
    const auto [tv, tt_out] = torch_jvp(
        [](const at::Tensor& z) { return std::get<1>(at::linalg_svd(z, false)); }, tx, tt);
    require_close(value.host<double>(), torch_values(tv), 2e-9);
    require_close(tangent.host<double>(), torch_values(tt_out), 2e-8);
  }
}

// Strict Track A: `pile` (torch's `stack`) and `unite` (torch's `cat`) had
// no dedicated reverse-mode oracle case anywhere in this file -- only
// exercised transitively inside other composites (batched-matmul chains,
// the conv/attention/RNN model-code audit). Direct per-op, per-dtype
// coverage, 3 differentiable inputs each so a wrong per-input gradient
// split (e.g. swapped/misaligned slices) would show up per-tensor.
TEST_CASE("CPU pile/unite reverse matches libtorch in float64 and float32",
          "[autodiff][parity][cpu][torch][structural][dtype]") {
  const std::vector<double> av{0.6, -1.1, 0.3, 1.8, -0.4, 0.9};   // [2,3]
  const std::vector<double> bv{-0.7, 1.3, 0.2, -0.5, 0.8, -0.2};  // [2,3]
  const std::vector<double> cv{0.4, -0.9, 1.1, -0.3, 0.6, -1.4};  // [2,3]

  for (const auto& dt : {std::pair{ScalarType::Float64, at::kDouble}, {ScalarType::Float32, at::kFloat}}) {
    DYNAMIC_SECTION((dt.first == ScalarType::Float64 ? "pile float64" : "pile float32")) {
      Series a = make_series(av, {2, 3}).to(dt.first);
      Series b = make_series(bv, {2, 3}).to(dt.first);
      Series c = make_series(cv, {2, 3}).to(dt.first);
      a.tracks_slope_(true);
      b.tracks_slope_(true);
      c.tracks_slope_(true);
      const Series out = a.pile(std::vector<Series>{b, c}, 1);  // -> [2,3,3]
      out.mul(out).sum().reverse();

      at::Tensor ta = at::tensor(av, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({2, 3}).set_requires_grad(true);
      at::Tensor tb = at::tensor(bv, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({2, 3}).set_requires_grad(true);
      at::Tensor tc = at::tensor(cv, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({2, 3}).set_requires_grad(true);
      at::Tensor tout = at::stack({ta, tb, tc}, 1);
      tout.mul(tout).sum().backward();

      const double tol = dt.first == ScalarType::Float64 ? 2e-9 : 3e-4;
      require_close(out.to(ScalarType::Float64).host<double>(), torch_values(tout), tol);
      require_close(a.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(ta.grad()), tol);
      require_close(b.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(tb.grad()), tol);
      require_close(c.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(tc.grad()), tol);
    }

    DYNAMIC_SECTION((dt.first == ScalarType::Float64 ? "unite float64" : "unite float32")) {
      // Different sizes along the concatenated dim (2,3 / 2,2 / 2,4) so a
      // wrong slice-width in the backward's split would be caught.
      const std::vector<double> ub{-0.4, 0.7, 0.2, -0.6};                    // [2,2]
      const std::vector<double> uc{0.5, -0.2, 0.3, 0.8, -0.5, 0.1, -0.3, 0.6};  // [2,4]
      Series a = make_series(av, {2, 3}).to(dt.first);
      Series b = make_series(ub, {2, 2}).to(dt.first);
      Series c = make_series(uc, {2, 4}).to(dt.first);
      a.tracks_slope_(true);
      b.tracks_slope_(true);
      c.tracks_slope_(true);
      const Series out = a.unite(std::vector<Series>{b, c}, 1);  // -> [2,9]
      out.mul(out).sum().reverse();

      at::Tensor ta = at::tensor(av, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({2, 3}).set_requires_grad(true);
      at::Tensor tb = at::tensor(ub, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({2, 2}).set_requires_grad(true);
      at::Tensor tc = at::tensor(uc, at::TensorOptions().dtype(at::kDouble)).to(dt.second).reshape({2, 4}).set_requires_grad(true);
      at::Tensor tout = at::cat({ta, tb, tc}, 1);
      tout.mul(tout).sum().backward();

      const double tol = dt.first == ScalarType::Float64 ? 2e-9 : 3e-4;
      require_close(out.to(ScalarType::Float64).host<double>(), torch_values(tout), tol);
      require_close(a.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(ta.grad()), tol);
      require_close(b.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(tb.grad()), tol);
      require_close(c.sensitivity().to(ScalarType::Float64).host<double>(), torch_values(tc.grad()), tol);
    }
  }
}

// `Series::hardshrink_backward` was flagged NO_NODE by the full-surface
// backward manifest (had no node at all -- a raw buffer loop, not built
// from tracked ops) and rewired composite via where()/gt() like
// hardshrink_impl itself. Verify both the value AND that a reverse-mode
// node now actually forms, against libtorch's own `at::hardshrink_backward`.
//
// Note the pluss call convention is `receiver.hardshrink_backward(arg,
// lambd)` with **receiver playing torch's `grad_out` role and `arg` playing
// torch's `self`/input role** (`hardshrink_backward_impl(self, other, s)`
// at the Invoke.cpp dispatch site maps positionally to
// `hardshrink_backward_impl(grad_out=self, input=other, lambda=s)`) --
// the wrapper parameter is named `grad_output` but is used as the original
// hardshrink input; unusual, but internally consistent and confirmed
// correct here against the oracle, so left as-is rather than touched as
// part of this fix.
TEST_CASE("CPU hardshrink_backward now forms a reverse-mode node and matches libtorch",
          "[autodiff][parity][cpu][torch][structural]") {
  const std::vector<double> grad_out_v{1.0, 2.0, 3.0, 4.0, 5.0};
  const std::vector<double> input_v{-2.0, -0.3, 0.1, 0.8, 3.0};
  const double lambd = 0.5;

  Series grad_out = make_series(grad_out_v, {5});
  Series input = make_series(input_v, {5});
  grad_out.tracks_slope_(true);
  const Series out = grad_out.hardshrink_backward(input, lambd);
  out.mul(out).sum().reverse();

  at::Tensor t_grad_out =
      at::tensor(grad_out_v, at::TensorOptions().dtype(at::kDouble)).set_requires_grad(true);
  at::Tensor t_input = at::tensor(input_v, at::TensorOptions().dtype(at::kDouble));
  at::Tensor t_out = at::hardshrink_backward(t_grad_out, t_input, lambd);
  t_out.mul(t_out).sum().backward();

  require_close(out.host<double>(), torch_values(t_out), 2e-12);
  require_close(grad_out.sensitivity().host<double>(), torch_values(t_grad_out.grad()), 2e-12);
}

#endif
