#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::linearalg {
namespace {

using pluss::codem::Config;
using pluss::codem::Platform;
using pluss::codem::ScalarType;
using pluss::codem::Series;

bool is_complex(ScalarType dtype) noexcept {
  return dtype == ScalarType::Complex64 || dtype == ScalarType::Complex128;
}

Series adjoint(const Series &value) {
  Series result = value.transpose(-2, -1);
  return is_complex(value.scalar_type()) ? result.conj() : result;
}

std::vector<std::int64_t> batch_shape(const Series &value) {
  const auto sizes = value.sizes();
  return {sizes.begin(), sizes.end() - 2};
}

std::vector<std::int64_t>
broadcast_shape(const std::vector<std::int64_t> &left,
                const std::vector<std::int64_t> &right,
                std::string_view operation) {
  const std::size_t rank = std::max(left.size(), right.size());
  std::vector<std::int64_t> result(rank, 1);
  for (std::size_t offset = 0; offset < rank; ++offset) {
    const std::int64_t a =
        offset < left.size() ? left[left.size() - 1 - offset] : 1;
    const std::int64_t b =
        offset < right.size() ? right[right.size() - 1 - offset] : 1;
    PLUSS_CHECK(a == b || a == 1 || b == 1,
                std::string("subsp::linearalg::") + std::string(operation) +
                    ": matrix batch dimensions are not broadcastable");
    result[rank - 1 - offset] = std::max(a, b);
  }
  return result;
}

void validate_matrix(const Series &value, std::string_view operation,
                     std::string_view name) {
  const std::string prefix =
      std::string("subsp::linearalg::") + std::string(operation) + ": ";
  PLUSS_CHECK(value.defined(), prefix + std::string(name) + " must be defined");
  PLUSS_CHECK(value.dim() >= 2,
              prefix + std::string(name) + " must have matrix axes");
  PLUSS_CHECK(value.size(-2) > 0 && value.size(-1) > 0,
              prefix + std::string(name) + " must be non-empty");
  PLUSS_CHECK(value.dtype().is_floating() || value.dtype().is_complex(),
              prefix + std::string(name) +
                  " must have a floating or complex dtype");
}

void validate_square(const Series &value, std::string_view operation,
                     std::string_view name) {
  validate_matrix(value, operation, name);
  PLUSS_CHECK(value.size(-2) == value.size(-1),
              std::string("subsp::linearalg::") + std::string(operation) +
                  ": " + std::string(name) + " must be square");
}

void validate_options(const MatrixEqOptions &options,
                      std::string_view operation) {
  const std::string prefix =
      std::string("subsp::linearalg::") + std::string(operation) + ": ";
  PLUSS_CHECK(options.max_iterations > 0,
              prefix + "max_iterations must be positive");
  PLUSS_CHECK(options.check_interval > 0,
              prefix + "check_interval must be positive");
  PLUSS_CHECK(std::isfinite(options.tolerance) && options.tolerance > 0.0,
              prefix + "tolerance must be finite and positive");
}

ScalarType common_dtype(const std::vector<Series> &values) {
  ScalarType dtype = values.front().scalar_type();
  for (std::size_t i = 1; i < values.size(); ++i)
    dtype = pluss::codem::promote_types(dtype, values[i].scalar_type());
  return dtype;
}

Series expand_matrix(Series value, const std::vector<std::int64_t> &batch) {
  while (static_cast<std::size_t>(value.dim() - 2) < batch.size())
    value = value.unsqueeze(0);
  std::vector<std::int64_t> shape = batch;
  shape.push_back(value.size(-2));
  shape.push_back(value.size(-1));
  return value.expand(shape, true);
}

Series identity_like(const Series &matrix, std::int64_t size,
                     const std::vector<std::int64_t> &batch = {}) {
  const ScalarType eye_dtype =
      matrix.scalar_type() == ScalarType::Complex64
          ? ScalarType::Float32
          : (matrix.scalar_type() == ScalarType::Complex128
                 ? ScalarType::Float64
                 : matrix.scalar_type());
  Series result = pluss::series::eye(
      size, Config{}.with_dtype(eye_dtype).with_platform(matrix.platform()));
  if (result.scalar_type() != matrix.scalar_type())
    result = result.to(matrix.scalar_type());
  return batch.empty() ? result : expand_matrix(std::move(result), batch);
}

double work_tolerance(const Series &value, double requested) {
  const bool narrow = value.scalar_type() == ScalarType::Float32 ||
                      value.scalar_type() == ScalarType::Complex64;
  return std::max(requested, narrow ? 2e-5 : 5e-12);
}

double max_abs(const Series &value) {
  return value.abs().amax().item<double>();
}

double relative_residual(const Series &residual, double scale) {
  const double value = max_abs(residual);
  return value / std::max(1.0, scale);
}

Series left_solve(const Series &coefficient, const Series &rhs) {
  Series result = coefficient.linearalg.lu_solve(rhs);
  if (result.scalar_type() != coefficient.scalar_type())
    result = result.to(coefficient.scalar_type());
  if (result.platform() != coefficient.platform())
    result = result.to(coefficient.platform());
  return result;
}

Series right_solve(const Series &coefficient, const Series &rhs) {
  return adjoint(left_solve(adjoint(coefficient), adjoint(rhs)));
}

Series hermitian_part(const Series &value) {
  return value.add(adjoint(value)).mul(0.5);
}

void require_hermitian(const Series &value, std::string_view operation,
                       std::string_view name, double tolerance) {
  PLUSS_CHECK(value.allclose(adjoint(value), tolerance, tolerance),
              std::string("subsp::linearalg::") + std::string(operation) +
                  ": " + std::string(name) + " must be Hermitian");
}

std::vector<Series> prepare_square_triplet(const Series &a, const Series &g,
                                           const Series &q,
                                           std::string_view operation) {
  validate_square(a, operation, "A");
  validate_square(g, operation, "G");
  validate_square(q, operation, "Q");
  PLUSS_CHECK(a.size(-1) == g.size(-1) && a.size(-1) == q.size(-1),
              std::string("subsp::linearalg::") + std::string(operation) +
                  ": A, G, and Q must have the same matrix size");
  PLUSS_CHECK(a.platform() == g.platform() && a.platform() == q.platform(),
              std::string("subsp::linearalg::") + std::string(operation) +
                  ": A, G, and Q must share one platform");
  auto batch = broadcast_shape(batch_shape(a), batch_shape(g), operation);
  batch = broadcast_shape(batch, batch_shape(q), operation);
  const ScalarType dtype = common_dtype({a, g, q});
  return {expand_matrix(a.to(dtype), batch), expand_matrix(g.to(dtype), batch),
          expand_matrix(q.to(dtype), batch)};
}

} // namespace

Series sylv(const Series &a, const Series &b, const Series &c) {
  constexpr std::string_view operation = "sylv";
  validate_square(a, operation, "A");
  validate_square(b, operation, "B");
  validate_matrix(c, operation, "C");
  PLUSS_CHECK(
      c.size(-2) == a.size(-1) && c.size(-1) == b.size(-1),
      "subsp::linearalg::sylv: C must have shape [..., A.rows, B.rows]");
  PLUSS_CHECK(a.platform() == b.platform() && a.platform() == c.platform(),
              "subsp::linearalg::sylv: A, B, and C must share one platform");

  auto batch = broadcast_shape(batch_shape(a), batch_shape(b), operation);
  batch = broadcast_shape(batch, batch_shape(c), operation);
  const ScalarType dtype = common_dtype({a, b, c});
  const Series aa = expand_matrix(a.to(dtype), batch);
  const Series bb = expand_matrix(b.to(dtype), batch);
  const Series cc = expand_matrix(c.to(dtype), batch);
  const std::int64_t m = aa.size(-1);
  const std::int64_t n = bb.size(-1);

  std::vector<std::int64_t> left_eye_shape(batch.size(), 1);
  left_eye_shape.insert(left_eye_shape.end(), {1, n, 1, n});
  std::vector<std::int64_t> right_eye_shape(batch.size(), 1);
  right_eye_shape.insert(right_eye_shape.end(), {m, 1, m, 1});
  const Series eye_n = identity_like(aa, n).reshape(left_eye_shape);
  const Series eye_m = identity_like(aa, m).reshape(right_eye_shape);
  const Series left = aa.unsqueeze(-1).unsqueeze(-3).mul(eye_n);
  const Series right =
      bb.transpose(-2, -1).unsqueeze(-2).unsqueeze(-4).mul(eye_m);

  std::vector<std::int64_t> system_shape = batch;
  system_shape.insert(system_shape.end(), {m * n, m * n});
  std::vector<std::int64_t> rhs_shape = batch;
  rhs_shape.insert(rhs_shape.end(), {m * n, 1});
  const Series system = left.add(right).reshape(system_shape);
  const Series solution = left_solve(system, cc.reshape(rhs_shape));
  return solution.reshape(cc.sizes());
}

Series dlyap(const Series &a, const Series &q, MatrixEqOptions options) {
  constexpr std::string_view operation = "dlyap";
  validate_options(options, operation);
  auto values = prepare_square_triplet(a, q, q, operation);
  Series aa = values[0];
  Series qq = values[1];
  require_hermitian(qq, operation, "Q", work_tolerance(qq, options.tolerance));

  Series x = qq;
  Series power = aa;
  const double tolerance = work_tolerance(x, options.tolerance);
  bool converged = false;
  for (std::int64_t iteration = 0; iteration < options.max_iterations;
       ++iteration) {
    x = x.add(power.matmul(x).matmul(adjoint(power)));
    power = power.matmul(power);
    if ((iteration + 1) % options.check_interval != 0 &&
        iteration + 1 != options.max_iterations)
      continue;
    const Series residual = aa.matmul(x).matmul(adjoint(aa)).sub(x).add(qq);
    const double scale = max_abs(qq) + max_abs(x);
    const double error = relative_residual(residual, scale);
    PLUSS_CHECK(
        std::isfinite(error),
        "subsp::linearalg::dlyap: iteration produced non-finite values");
    if (error <= tolerance) {
      converged = true;
      break;
    }
  }
  PLUSS_CHECK(converged,
              "subsp::linearalg::dlyap: doubling iteration did not converge; "
              "A may not be Schur stable");
  return hermitian_part(x);
}

Series lyap(const Series &a, const Series &q, MatrixEqOptions options) {
  constexpr std::string_view operation = "lyap";
  validate_options(options, operation);
  auto values = prepare_square_triplet(a, q, q, operation);
  const Series aa = values[0];
  const Series qq = values[1];
  require_hermitian(qq, operation, "Q", work_tolerance(qq, options.tolerance));

  const auto batch = batch_shape(aa);
  const Series eye = identity_like(aa, aa.size(-1), batch);
  const Series minus = aa.sub(eye);
  const Series cayley = left_solve(minus, aa.add(eye));
  Series mapped_q = left_solve(minus, qq).mul(2.0);
  mapped_q = right_solve(adjoint(minus), mapped_q);
  Series x = dlyap(cayley, mapped_q, options);
  const Series residual = aa.matmul(x).add(x.matmul(adjoint(aa))).add(qq);
  const double error =
      relative_residual(residual, max_abs(qq) + 2.0 * max_abs(aa) * max_abs(x));
  PLUSS_CHECK(std::isfinite(error) &&
                  error <= 8.0 * work_tolerance(x, options.tolerance),
              "subsp::linearalg::lyap: continuous residual exceeds tolerance; "
              "A may not be Hurwitz stable");
  return hermitian_part(x);
}

Series care(const Series &a, const Series &g, const Series &q,
            MatrixEqOptions options) {
  constexpr std::string_view operation = "care";
  validate_options(options, operation);
  auto values = prepare_square_triplet(a, g, q, operation);
  const Series aa = values[0];
  const Series gg = values[1];
  const Series qq = values[2];
  const double tolerance = work_tolerance(aa, options.tolerance);
  require_hermitian(gg, operation, "G", tolerance);
  require_hermitian(qq, operation, "Q", tolerance);

  const auto batch = batch_shape(aa);
  const std::int64_t n = aa.size(-1);
  const Series top = aa.unite({gg.neg()}, -1, true);
  const Series bottom = qq.neg().unite({adjoint(aa).neg()}, -1, true);
  Series sign = top.unite({bottom}, -2, true);
  const Series eye = identity_like(sign, 2 * n, batch);
  bool converged = false;
  for (std::int64_t iteration = 0; iteration < options.max_iterations;
       ++iteration) {
    const Series inverse = left_solve(sign, eye);
    const double sign_norm = max_abs(sign);
    const double inverse_norm = max_abs(inverse);
    PLUSS_CHECK(
        std::isfinite(sign_norm) && std::isfinite(inverse_norm) &&
            sign_norm > 0.0 && inverse_norm > 0.0,
        "subsp::linearalg::care: matrix-sign iteration became singular");
    const double scale = std::sqrt(inverse_norm / sign_norm);
    sign = sign.mul(scale).add(inverse.div(scale)).mul(0.5);
    if ((iteration + 1) % options.check_interval != 0 &&
        iteration + 1 != options.max_iterations)
      continue;
    const double error = relative_residual(sign.matmul(sign).sub(eye), 1.0);
    PLUSS_CHECK(std::isfinite(error),
                "subsp::linearalg::care: matrix-sign iteration produced "
                "non-finite values");
    if (error <= tolerance) {
      converged = true;
      break;
    }
  }
  PLUSS_CHECK(converged,
              "subsp::linearalg::care: matrix-sign iteration did not "
              "converge; the Hamiltonian may have imaginary-axis spectrum");

  const Series projector = eye.sub(sign).mul(0.5);
  const Series p11 = projector.slice(-2, 0, n).slice(-1, 0, n);
  const Series p21 = projector.slice(-2, n, 2 * n).slice(-1, 0, n);
  Series x = hermitian_part(right_solve(p11, p21));
  const Series residual = adjoint(aa)
                              .matmul(x)
                              .add(x.matmul(aa))
                              .sub(x.matmul(gg).matmul(x))
                              .add(qq);
  const double error =
      relative_residual(residual, max_abs(qq) + 2.0 * max_abs(aa) * max_abs(x) +
                                      max_abs(gg) * max_abs(x) * max_abs(x));
  PLUSS_CHECK(std::isfinite(error) && error <= 16.0 * tolerance,
              "subsp::linearalg::care: stabilizing solution residual exceeds "
              "tolerance");
  return x;
}

Series dare(const Series &a, const Series &g, const Series &q,
            MatrixEqOptions options) {
  constexpr std::string_view operation = "dare";
  validate_options(options, operation);
  auto values = prepare_square_triplet(a, g, q, operation);
  Series ak = values[0];
  Series gk = values[1];
  Series hk = values[2];
  const Series original_a = ak;
  const Series original_g = gk;
  const Series original_q = hk;
  const double tolerance = work_tolerance(ak, options.tolerance);
  require_hermitian(gk, operation, "G", tolerance);
  require_hermitian(hk, operation, "Q", tolerance);
  const Series eye = identity_like(ak, ak.size(-1), batch_shape(ak));

  bool converged = false;
  for (std::int64_t iteration = 0; iteration < options.max_iterations;
       ++iteration) {
    const Series ak_h = adjoint(ak);
    const Series left = eye.add(gk.matmul(hk));
    const Series right = eye.add(hk.matmul(gk));
    const Series next_a = ak.matmul(left_solve(left, ak));
    const Series next_g = gk.add(ak.matmul(gk).matmul(left_solve(right, ak_h)));
    const Series next_h = hk.add(ak_h.matmul(left_solve(right, hk.matmul(ak))));
    ak = next_a;
    gk = hermitian_part(next_g);
    hk = hermitian_part(next_h);
    if ((iteration + 1) % options.check_interval != 0 &&
        iteration + 1 != options.max_iterations)
      continue;
    const Series rhs =
        original_q.add(adjoint(original_a)
                           .matmul(left_solve(eye.add(hk.matmul(original_g)),
                                              hk.matmul(original_a))));
    const double error =
        relative_residual(hk.sub(rhs), max_abs(original_q) + max_abs(hk));
    PLUSS_CHECK(std::isfinite(error),
                "subsp::linearalg::dare: doubling iteration produced "
                "non-finite values");
    if (error <= tolerance) {
      converged = true;
      break;
    }
  }
  PLUSS_CHECK(converged,
              "subsp::linearalg::dare: doubling iteration did not converge; "
              "the stabilizing solution may not exist");
  return hermitian_part(hk);
}

} // namespace pluss::subsp::linearalg
