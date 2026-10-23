#pragma once

#include "CodeM/compat/Invoke.h"
#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace pluss::subsp::linearalg {

struct SvdDecomposition {
  Series u = pluss::series::undefined();
  Series s = pluss::series::undefined();
  Series vh = pluss::series::undefined();
};

struct EigenDecomposition {
  Series values = pluss::series::undefined();
  Series vectors = pluss::series::undefined();
};

struct GenEigResult {
  Series values = pluss::series::undefined();
  Series vectors = pluss::series::undefined();
  Series residual = pluss::series::undefined();
  Series converged = pluss::series::undefined();
  std::int64_t iterations{};
};

namespace detail {

inline bool same_gen_eig_layout(const Series &lhs, const Series &rhs) {
  if (!lhs.defined() || lhs.dim() != rhs.dim() ||
      lhs.scalar_type() != rhs.scalar_type() ||
      lhs.platform() != rhs.platform())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline bool finite_gen_eig_value(const Series &value) {
  return value.dtype().is_complex()
             ? value.real().isfinite().all().item<bool>() &&
                   value.imag().isfinite().all().item<bool>()
             : value.isfinite().all().item<bool>();
}

template <class Apply>
Series checked_gen_eig_apply(Apply &apply, const Series &value,
                             const char *role) {
  Series image = apply(value);
  if (!same_gen_eig_layout(image, value))
    throw std::invalid_argument(std::string("linearalg::gen_eigh: ") + role +
                                " must preserve shape, dtype, and platform");
  if (!finite_gen_eig_value(image))
    throw std::invalid_argument(std::string("linearalg::gen_eigh: ") + role +
                                " produced a non-finite value");
  return image;
}

} // namespace detail

template <class ApplyA, class ApplyB, class SolveA>
GenEigResult gen_eigh(const Series &initial, ApplyA &&apply_a,
                      ApplyB &&apply_b, SolveA &&solve_a,
                      const GenEigOptions &options) {
  if (!initial.defined() ||
      (!initial.dtype().is_floating() && !initial.dtype().is_complex()) ||
      initial.dim() < 2 || initial.numel() == 0 || initial.size(-2) <= 0 ||
      initial.size(-1) <= 0 ||
      initial.size(-2) > initial.size(-1))
    throw std::invalid_argument(
        "linearalg::gen_eigh: initial must be floating or complex "
        "[..., mode, state] with 0 < mode <= state");
  if (!detail::finite_gen_eig_value(initial))
    throw std::invalid_argument(
        "linearalg::gen_eigh: initial must be finite");
  if (options.max_iterations <= 0 ||
      !(options.relative_tolerance >= 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance >= 0.0) ||
      !std::isfinite(options.absolute_tolerance))
    throw std::invalid_argument(
        "linearalg::gen_eigh: iteration limit must be positive and "
        "tolerances must be finite and non-negative");

  auto &&a = apply_a;
  auto &&b = apply_b;
  auto &&inverse_a = solve_a;
  const auto orthonormalize = [&](const Series &candidate) {
    const Series image = detail::checked_gen_eig_apply(a, candidate, "apply_a");
    const Series gram = candidate.conj().matmul(image.transpose(-2, -1));
    const Series symmetric = gram.add(gram.adjoint()).mul(0.5);
    const auto eig = symmetric.linearalg.eigh();
    const double scale = eig.values.abs().amax().item<double>();
    const bool single_precision =
        initial.scalar_type() == ScalarType::Float32 ||
        initial.scalar_type() == ScalarType::Complex64;
    const double epsilon = single_precision
                               ? std::numeric_limits<float>::epsilon()
                               : std::numeric_limits<double>::epsilon();
    if (!(scale > 0.0) ||
        !eig.values.gt(scale * epsilon * 100.0)
             .all()
             .item<bool>())
      throw std::invalid_argument(
          "linearalg::gen_eigh: the trial subspace is rank deficient");
    return eig.vectors.transpose(-2, -1)
        .mul(eig.values.rsqrt().unsqueeze(-1))
        .matmul(candidate);
  };

  Series vectors = orthonormalize(initial);
  Series values = initial.abs().select(-1, 0).mul(0.0);
  Series residual = pluss::series::full_like(values, INFINITY);
  Series converged = pluss::series::zeros_like(values).to(ScalarType::Bool);
  std::int64_t iterations = 0;

  for (std::int64_t iteration = 0; iteration < options.max_iterations;
       ++iteration) {
    const Series rhs = detail::checked_gen_eig_apply(b, vectors, "apply_b");
    const Series solved =
        detail::checked_gen_eig_apply(inverse_a, rhs, "solve_a");
    vectors = orthonormalize(solved);

    Series b_vectors = detail::checked_gen_eig_apply(b, vectors, "apply_b");
    const Series projected =
        vectors.conj().matmul(b_vectors.transpose(-2, -1));
    const Series symmetric =
        projected.add(projected.adjoint()).mul(0.5);
    const auto eig = symmetric.linearalg.eigh();
    const Series mu = eig.values.flip(-1);
    const Series rotation = eig.vectors.flip(-1).transpose(-2, -1);
    vectors = rotation.matmul(vectors);
    b_vectors = rotation.matmul(b_vectors);
    if (!mu.gt(0.0).all().item<bool>())
      throw std::invalid_argument(
          "linearalg::gen_eigh: requested modes are not in the positive "
          "generalized eigenspace");
    values = mu.reciprocal();
    const Series a_vectors = detail::checked_gen_eig_apply(a, vectors, "apply_a");
    const Series defect =
        a_vectors.sub(b_vectors.mul(values.unsqueeze(-1)));
    residual = defect.abs().square().sum(-1, false).sqrt();
    const Series scale =
        a_vectors.abs().square().sum(-1, false).sqrt().maximum(
            b_vectors.abs().square().sum(-1, false).sqrt().mul(values.abs()));
    converged = residual.le(
        scale.mul(options.relative_tolerance).add(options.absolute_tolerance));
    iterations = iteration + 1;
    if (converged.all().item<bool>())
      break;
  }
  const Series b_vectors =
      detail::checked_gen_eig_apply(b, vectors, "apply_b");
  const Series b_norm2 =
      vectors.conj().mul(b_vectors).sum(-1, false).real();
  if (!b_norm2.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "linearalg::gen_eigh: a returned vector has non-positive B norm");
  const Series b_norm = b_norm2.sqrt();
  return {values, vectors.div(b_norm.unsqueeze(-1)), residual.div(b_norm),
          converged, iterations};
}

inline Series call(std::string_view op, const Series &x,
                   pluss::codem::_compat::ArgView args = {}) {
  return pluss::codem::_compat::invoke(op, x, args);
}

template <class... Args>
inline Series call(std::string_view op, const Series &x, Args &&...args) {
  const std::array<pluss::codem::_compat::Arg, sizeof...(Args)> packed{
      pluss::codem::_compat::Arg(std::forward<Args>(args))...};
  return call(op, x,
              pluss::codem::_compat::ArgView{packed.data(), packed.size()});
}

} // namespace pluss::subsp::linearalg

namespace pluss::codem {

inline pluss::subsp::linearalg::SvdDecomposition
Series::SeriesLinearAlgProxy::svd(bool full_matrices) const {
  return pluss::subsp::linearalg::svd(self(), full_matrices);
}

inline pluss::subsp::linearalg::SvdDecomposition
Series::SeriesLinearAlgAccessor::svd(bool full_matrices) const {
  return (*this)().svd(full_matrices);
}

inline pluss::subsp::linearalg::EigenDecomposition
Series::SeriesLinearAlgProxy::eigh(bool upper) const {
  return pluss::subsp::linearalg::eigh(self(), upper);
}

inline pluss::subsp::linearalg::EigenDecomposition
Series::SeriesLinearAlgAccessor::eigh(bool upper) const {
  return (*this)().eigh(upper);
}

template <class ApplyA, class ApplyB, class SolveA>
inline pluss::subsp::linearalg::GenEigResult
Series::SeriesLinearAlgProxy::gen_eigh(
    ApplyA &&apply_a, ApplyB &&apply_b, SolveA &&solve_a,
    const pluss::subsp::linearalg::GenEigOptions &options) const {
  return pluss::subsp::linearalg::gen_eigh(
      self(), std::forward<ApplyA>(apply_a), std::forward<ApplyB>(apply_b),
      std::forward<SolveA>(solve_a), options);
}

template <class ApplyA, class ApplyB, class SolveA>
inline pluss::subsp::linearalg::GenEigResult
Series::SeriesLinearAlgAccessor::gen_eigh(
    ApplyA &&apply_a, ApplyB &&apply_b, SolveA &&solve_a,
    const pluss::subsp::linearalg::GenEigOptions &options) const {
  return (*this)().gen_eigh(
      std::forward<ApplyA>(apply_a), std::forward<ApplyB>(apply_b),
      std::forward<SolveA>(solve_a), options);
}

} // namespace pluss::codem
