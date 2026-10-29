#include "PrimitiveRegistry.h"

#include "pluss/codem/series_factory.h"
#include "pluss/error.h"

#include "pluss/subsp/linearalg/linearalg.h"

#include <array>
#include <bit>
#include <numeric>
#include <string>
#include <vector>

namespace pluss::subsp::math::detail {
namespace {

bool is_complex(const pluss::Series &value) noexcept {
  return value.scalar_type() == pluss::ScalarType::Complex64 ||
         value.scalar_type() == pluss::ScalarType::Complex128;
}

pluss::Series add(pluss::RangeView<pluss::Series> x) { return x[0].add(x[1]); }
pluss::Series sub(pluss::RangeView<pluss::Series> x) { return x[0].sub(x[1]); }
pluss::Series mul(pluss::RangeView<pluss::Series> x) { return x[0].mul(x[1]); }
pluss::Series div(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[1].ne(0.0).all().item<bool>(),
              "math::Function: division domain excludes zero denominators");
  return x[0].div(x[1]);
}
pluss::Series pow(pluss::RangeView<pluss::Series> x) { return x[0].pow(x[1]); }
pluss::Series neg(pluss::RangeView<pluss::Series> x) { return x[0].neg(); }
pluss::Series exp(pluss::RangeView<pluss::Series> x) { return x[0].exp(); }
pluss::Series log(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(is_complex(x[0]) ? x[0].ne(0.0).all().item<bool>()
                               : x[0].gt(0.0).all().item<bool>(),
              "math::Function: log domain excludes zero and requires "
              "positive real values");
  return x[0].log();
}
pluss::Series log1p(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(is_complex(x[0]) ? x[0].add(1.0).ne(0.0).all().item<bool>()
                               : x[0].gt(-1.0).all().item<bool>(),
              "math::Function: log1p domain excludes -1 and requires real "
              "values greater than -1");
  return x[0].log1p();
}
pluss::Series expm1(pluss::RangeView<pluss::Series> x) { return x[0].expm1(); }
pluss::Series sqrt(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(is_complex(x[0]) || x[0].ge(0.0).all().item<bool>(),
              "math::Function: sqrt domain requires nonnegative values");
  return x[0].sqrt();
}
pluss::Series sin(pluss::RangeView<pluss::Series> x) { return x[0].sin(); }
pluss::Series cos(pluss::RangeView<pluss::Series> x) { return x[0].cos(); }
pluss::Series tan(pluss::RangeView<pluss::Series> x) { return x[0].tan(); }
pluss::Series sinh(pluss::RangeView<pluss::Series> x) { return x[0].sinh(); }
pluss::Series cosh(pluss::RangeView<pluss::Series> x) { return x[0].cosh(); }
pluss::Series tanh(pluss::RangeView<pluss::Series> x) { return x[0].tanh(); }
pluss::Series abs(pluss::RangeView<pluss::Series> x) { return x[0].abs(); }
pluss::Series conj(pluss::RangeView<pluss::Series> x) { return x[0].conj(); }
pluss::Series real(pluss::RangeView<pluss::Series> x) { return x[0].real(); }
pluss::Series imag(pluss::RangeView<pluss::Series> x) { return x[0].imag(); }
pluss::Series angle(pluss::RangeView<pluss::Series> x) { return x[0].angle(); }

pluss::Series safe_div_strict(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[1].ne(0.0).all().item<bool>(),
              "operators: complex derivative is undefined at zero");
  return x[0].div(x[1]);
}
pluss::Series safe_div_zero(pluss::RangeView<pluss::Series> x) {
  const pluss::Series zero = pluss::series::zeros_like(x[0].add(x[1]));
  return x[1].eq(0.0).where(zero, x[0].div(x[1]));
}
pluss::Series safe_div_piecewise(pluss::RangeView<pluss::Series> x) {
  return x[0].div(x[1]);
}
pluss::Series zeros_like(pluss::RangeView<pluss::Series> x) {
  return pluss::series::zeros_like(x[0]);
}
pluss::Series ones_like(pluss::RangeView<pluss::Series> x) {
  return pluss::series::ones_like(x[0]);
}
pluss::Series stack(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(!x.empty(), "math::Function: stack requires at least one input");
  pluss::ScalarType dtype = x.front().scalar_type();
  for (std::size_t i = 1; i < x.size(); ++i) {
    dtype = pluss::codem::promote_types(dtype, x[i].scalar_type());
  }
  std::vector<pluss::Series> promoted;
  promoted.reserve(x.size());
  for (const pluss::Series &value : x) {
    promoted.push_back(value.scalar_type() == dtype ? value : value.to(dtype));
  }
  return promoted.front().pile(
      pluss::RangeView<pluss::Series>(promoted).drop_front(),
      promoted.front().dim());
}
pluss::Series abs_slope_strict(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[0].ne(0.0).all().item<bool>(),
              "operators::differential: abs is not differentiable at zero");
  return x[0].div(x[0].abs());
}
pluss::Series abs_slope_zero(pluss::RangeView<pluss::Series> x) {
  const pluss::Series zero = pluss::series::zeros_like(x[0]);
  return x[0].eq(0.0).where(zero, x[0].div(x[0].abs()));
}
pluss::Series abs_slope_piecewise(pluss::RangeView<pluss::Series> x) {
  return x[0].div(x[0].abs());
}


// ---- tensor primitives ---------------------------------------------------
// A value is [batch..., output axes...]; `rank` (attribute 0 or 0/1) is the number of trailing output
// axes, so output axis k of an operand v lives at v.dim() - rank + k.
std::int64_t axis_of(const pluss::Series &value, std::int64_t rank,
                     std::int64_t output_axis) {
  PLUSS_CHECK(rank >= 0 && output_axis >= 0 && output_axis < rank &&
                  value.dim() >= rank,
              "math::Function: tensor axis is outside the operand's output "
              "shape");
  return value.dim() - rank + output_axis;
}

pluss::Series transpose_with(pluss::RangeView<pluss::Series> x,
                             std::span<const std::int64_t> a) {
  return x[0].transpose(axis_of(x[0], a[0], a[1]), axis_of(x[0], a[0], a[2]));
}
pluss::Series select_with(pluss::RangeView<pluss::Series> x,
                          std::span<const std::int64_t> a) {
  return x[0].select(axis_of(x[0], a[0], a[1]), a[2]);
}
// Closed forms for 1x1, 2x2 and 3x3 matrices (the sizes tensor calculus uses): elementwise over the batch,
// instead of a general factorization per matrix.
pluss::Series entry(const pluss::Series &m, std::int64_t i, std::int64_t j) {
  return m.select(-2, i).select(-1, j);
}
pluss::Series matrix_of(const std::vector<std::vector<pluss::Series>> &rows) {
  std::vector<pluss::Series> built;
  for (const auto &row : rows) {
    std::vector<pluss::Series> rest(row.begin() + 1, row.end());
    built.push_back(row.front().pile(pluss::RangeView<pluss::Series>(rest), row.front().dim()));
  }
  std::vector<pluss::Series> rest(built.begin() + 1, built.end());
  return built.front().pile(pluss::RangeView<pluss::Series>(rest), built.front().dim() - 1);
}
pluss::Series small_det(const pluss::Series &m) {
  const std::int64_t n = m.size(-1);
  if (n == 1) return entry(m, 0, 0);
  if (n == 2) return entry(m, 0, 0).mul(entry(m, 1, 1)).sub(entry(m, 0, 1).mul(entry(m, 1, 0)));
  const auto e = [&](std::int64_t i, std::int64_t j) { return entry(m, i, j); };
  return e(0, 0).mul(e(1, 1).mul(e(2, 2)).sub(e(1, 2).mul(e(2, 1))))
      .sub(e(0, 1).mul(e(1, 0).mul(e(2, 2)).sub(e(1, 2).mul(e(2, 0)))))
      .add(e(0, 2).mul(e(1, 0).mul(e(2, 1)).sub(e(1, 1).mul(e(2, 0)))));
}
pluss::Series small_inverse(const pluss::Series &m, const pluss::Series &det) {
  const std::int64_t n = m.size(-1);
  const auto e = [&](std::int64_t i, std::int64_t j) { return entry(m, i, j); };
  if (n == 1) return matrix_of({{det.reciprocal()}});
  if (n == 2) {
    return matrix_of({{e(1, 1).div(det), e(0, 1).mul(-1.0).div(det)}, {e(1, 0).mul(-1.0).div(det), e(0, 0).div(det)}});
  }
  const auto cof = [&](std::int64_t i, std::int64_t j) {  // cofactor C_ij
    const std::int64_t r0 = (i + 1) % 3, r1 = (i + 2) % 3, c0 = (j + 1) % 3, c1 = (j + 2) % 3;
    return e(r0, c0).mul(e(r1, c1)).sub(e(r0, c1).mul(e(r1, c0)));
  };
  // inverse = adj / det with adj_ij = C_ji (the cyclic indices above fold the alternating signs in)
  return matrix_of({{cof(0, 0).div(det), cof(1, 0).div(det), cof(2, 0).div(det)},
                    {cof(0, 1).div(det), cof(1, 1).div(det), cof(2, 1).div(det)},
                    {cof(0, 2).div(det), cof(1, 2).div(det), cof(2, 2).div(det)}});
}
pluss::Series inverse_with(pluss::RangeView<pluss::Series> x,
                           std::span<const std::int64_t>) {
  PLUSS_CHECK(x[0].dim() >= 2 && x[0].size(-1) == x[0].size(-2),
              "math::Function: inverse requires a square matrix output");
  const bool small = x[0].size(-1) <= 3 && !x[0].is_complex();
  const pluss::Series det = small ? small_det(x[0]) : pluss::subsp::linearalg::det(x[0]);
  PLUSS_CHECK(det.ne(0.0).all().item<bool>(),
              "math::Function: inverse domain excludes singular matrices");
  return small ? small_inverse(x[0], det) : pluss::subsp::linearalg::inverse(x[0]);
}
pluss::Series det_with(pluss::RangeView<pluss::Series> x,
                       std::span<const std::int64_t>) {
  PLUSS_CHECK(x[0].dim() >= 2 && x[0].size(-1) == x[0].size(-2),
              "math::Function: det requires a square matrix output");
  if (x[0].size(-1) <= 3 && !x[0].is_complex()) return small_det(x[0]);
  return pluss::subsp::linearalg::det(x[0]);
}
pluss::Series trace_with(pluss::RangeView<pluss::Series> x,
                         std::span<const std::int64_t>) {
  PLUSS_CHECK(x[0].dim() >= 2 && x[0].size(-1) == x[0].size(-2),
              "math::Function: trace requires a square matrix output");
  const pluss::Series eye =
      pluss::series::eye(x[0].size(-1), x[0].scalar_type()).to(x[0].platform());
  return x[0].mul(eye).sum(-1, false).sum(-1, false);
}

// attributes: rank_a, rank_b, n, a_axes[n], b_axes[n]
pluss::Series contract_with(pluss::RangeView<pluss::Series> x,
                            std::span<const std::int64_t> a) {
  const std::int64_t rank_a = a[0], rank_b = a[1], n = a[2];
  const pluss::Series &left = x[0];
  const pluss::Series &right = x[1];
  PLUSS_CHECK(left.dim() >= rank_a && right.dim() >= rank_b,
              "math::Function: contraction operand has fewer axes than its "
              "output shape");
  const std::int64_t left_batch = left.dim() - rank_a;
  const std::int64_t right_batch = right.dim() - rank_b;
  for (std::int64_t k = 0; k < n; ++k) {
    PLUSS_CHECK(left.size(left_batch + a[3 + k]) ==
                    right.size(right_batch + a[3 + n + k]),
                "math::Function: contracted axes have different lengths");
  }
  // No batch axes: a plain tensordot over the trailing operand axes.
  if (left_batch == 0 && right_batch == 0) {
    return left.tensordot(
        right, std::vector<std::int64_t>(a.begin() + 3, a.begin() + 3 + n),
        std::vector<std::int64_t>(a.begin() + 3 + n, a.begin() + 3 + 2 * n));
  }
  // Leading batch axes broadcast against each other (right-aligned), which
  // einsum's ellipsis expresses directly: "...<a-axes>,...<b-axes>->...<free>".
  PLUSS_CHECK(rank_a <= 26 && rank_b <= 26,
              "math::Function: contraction operand has too many axes");
  std::string left_labels, right_labels(static_cast<std::size_t>(rank_b), ' '),
      free_labels;
  for (std::int64_t i = 0; i < rank_a; ++i)
    left_labels.push_back(static_cast<char>('a' + i));
  for (std::int64_t k = 0; k < n; ++k)
    right_labels[static_cast<std::size_t>(a[3 + n + k])] =
        left_labels[static_cast<std::size_t>(a[3 + k])];
  std::vector<bool> left_used(static_cast<std::size_t>(rank_a), false);
  for (std::int64_t k = 0; k < n; ++k)
    left_used[static_cast<std::size_t>(a[3 + k])] = true;
  for (std::int64_t i = 0; i < rank_a; ++i)
    if (!left_used[static_cast<std::size_t>(i)]) free_labels.push_back(left_labels[static_cast<std::size_t>(i)]);
  for (std::int64_t i = 0; i < rank_b; ++i) {
    if (right_labels[static_cast<std::size_t>(i)] == ' ') {
      right_labels[static_cast<std::size_t>(i)] = static_cast<char>('A' + i);
      free_labels.push_back(right_labels[static_cast<std::size_t>(i)]);
    }
  }
  return pluss::Series::einsum(
      "..." + left_labels + ",..." + right_labels + "->..." + free_labels,
      {left, right});
}

// ---- path functionals -------------------------------------------------------
// Arguments of a path functional are Series whose axis `path_axis` indexes the
// sample times of one path; every other axis is an independent path (batch).
std::int64_t path_axis_of(const pluss::Series &value, std::int64_t axis) {
  const std::int64_t resolved = axis < 0 ? value.dim() + axis : axis;
  PLUSS_CHECK(resolved >= 0 && resolved < value.dim() && value.size(resolved) >= 2,
              "math::Function: a path functional needs a path axis with at "
              "least two samples");
  return resolved;
}

// attributes: theta (bit pattern), cumulative, path_axis. inputs: integrand F, noise W.
// sum_i [(1 - theta) F_i + theta F_{i+1}] (W_{i+1} - W_i): theta 0 is Ito (left endpoint),
// 1/2 Stratonovich, 1 Klimontovich (right endpoint).
pluss::Series path_integral_with(pluss::RangeView<pluss::Series> x,
                                 std::span<const std::int64_t> a) {
  const double theta = std::bit_cast<double>(a[0]);
  const bool cumulative = a[1] != 0;
  const pluss::Series &noise = x[1];
  const std::int64_t axis = path_axis_of(noise, a[2]);
  const std::int64_t n = noise.size(axis);
  PLUSS_CHECK(x[0].dim() <= noise.dim(),
              "math::Function: the integrand has more axes than the noise "
              "path");
  const pluss::Series integrand = x[0].add(noise.mul(0.0)); // broadcast constants along the path
  const pluss::Series increments =
      noise.slice(axis, 1, n, 1).sub(noise.slice(axis, 0, n - 1, 1));
  const pluss::Series left = integrand.slice(axis, 0, n - 1, 1);
  const pluss::Series right = integrand.slice(axis, 1, n, 1);
  const pluss::Series weighted =
      theta == 0.0 ? left : left.mul(1.0 - theta).add(right.mul(theta));
  const pluss::Series terms = weighted.mul(increments);
  if (!cumulative) {
    return terms.sum(axis, false);
  }
  const pluss::Series zero = pluss::series::zeros_like(terms.slice(axis, 0, 1, 1));
  return zero.unite({terms.cumsum(axis)}, axis);
}

// attributes: cumulative, path_axis. inputs X, Y: sum_i (X_{i+1} - X_i)(Y_{i+1} - Y_i).
pluss::Series path_covariation_with(pluss::RangeView<pluss::Series> x,
                                    std::span<const std::int64_t> a) {
  const bool cumulative = a[0] != 0;
  const pluss::Series first = x[0].add(x[1].mul(0.0));
  const pluss::Series second = x[1].add(x[0].mul(0.0));
  const std::int64_t axis = path_axis_of(first, a[1]);
  const std::int64_t n = first.size(axis);
  const pluss::Series terms =
      first.slice(axis, 1, n, 1).sub(first.slice(axis, 0, n - 1, 1))
          .mul(second.slice(axis, 1, n, 1).sub(second.slice(axis, 0, n - 1, 1)));
  if (!cumulative) {
    return terms.sum(axis, false);
  }
  const pluss::Series zero = pluss::series::zeros_like(terms.slice(axis, 0, 1, 1));
  return zero.unite({terms.cumsum(axis)}, axis);
}

constexpr std::array<PrimitiveSpec, 39> registry{{
    {PrimitiveId::Add, "add", 2, add},
    {PrimitiveId::Sub, "sub", 2, sub},
    {PrimitiveId::Mul, "mul", 2, mul},
    {PrimitiveId::Div, "div", 2, div},
    {PrimitiveId::Pow, "pow", 2, pow},
    {PrimitiveId::Neg, "neg", 1, neg},
    {PrimitiveId::Exp, "exp", 1, exp},
    {PrimitiveId::Log, "log", 1, log},
    {PrimitiveId::Log1p, "log1p", 1, log1p},
    {PrimitiveId::Expm1, "expm1", 1, expm1},
    {PrimitiveId::Sqrt, "sqrt", 1, sqrt},
    {PrimitiveId::Sin, "sin", 1, sin},
    {PrimitiveId::Cos, "cos", 1, cos},
    {PrimitiveId::Tan, "tan", 1, tan},
    {PrimitiveId::Sinh, "sinh", 1, sinh},
    {PrimitiveId::Cosh, "cosh", 1, cosh},
    {PrimitiveId::Tanh, "tanh", 1, tanh},
    {PrimitiveId::Abs, "abs", 1, abs},
    {PrimitiveId::ZerosLike, "zeros_like", 1, zeros_like},
    {PrimitiveId::OnesLike, "ones_like", 1, ones_like},
    {PrimitiveId::Stack, "stack", 0, stack, true},
    {PrimitiveId::AbsSlopeStrict, "abs_slope_strict", 1, abs_slope_strict},
    {PrimitiveId::AbsSlopeZero, "abs_slope_zero", 1, abs_slope_zero},
    {PrimitiveId::AbsSlopePiecewise, "abs_slope_piecewise", 1,
     abs_slope_piecewise},
    {PrimitiveId::Transpose, "transpose", 1, nullptr, false, transpose_with},
    {PrimitiveId::Contract, "contract", 2, nullptr, false, contract_with},
    {PrimitiveId::Inverse, "inverse", 1, nullptr, false, inverse_with},
    {PrimitiveId::Det, "det", 1, nullptr, false, det_with},
    {PrimitiveId::Trace, "trace", 1, nullptr, false, trace_with},
    {PrimitiveId::Select, "select", 1, nullptr, false, select_with},
    {PrimitiveId::PathIntegral, "path_integral", 2, nullptr, false, path_integral_with},
    {PrimitiveId::PathCovariation, "path_covariation", 2, nullptr, false, path_covariation_with},
    {PrimitiveId::Conj, "conj", 1, conj},
    {PrimitiveId::Real, "real", 1, real},
    {PrimitiveId::Imag, "imag", 1, imag},
    {PrimitiveId::Angle, "angle", 1, angle},
    {PrimitiveId::SafeDivStrict, "safe_div_strict", 2, safe_div_strict},
    {PrimitiveId::SafeDivZero, "safe_div_zero", 2, safe_div_zero},
    {PrimitiveId::SafeDivPiecewise, "safe_div_piecewise", 2,
     safe_div_piecewise},
}};

} // namespace

const PrimitiveSpec &primitive(PrimitiveId id) {
  const std::size_t index = static_cast<std::size_t>(id) - 1;
  PLUSS_CHECK(index < registry.size() && registry[index].id == id,
              "math::Function: unknown primitive operation ID");
  return registry[index];
}

} // namespace pluss::subsp::math::detail
