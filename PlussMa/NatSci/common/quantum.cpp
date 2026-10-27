#include "pluss/subsp/natsci/common/quantum.h"

#include "CodeM/core/TypePromotion.h"
#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/math/ode.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace pluss::subsp::natsci::common::quantum {
namespace {

std::vector<std::int64_t> leading_shape(const Series &value,
                                        std::int64_t trailing) {
  return {value.sizes().begin(), value.sizes().end() - trailing};
}

std::int64_t checked_size(const std::vector<std::int64_t> &spaces,
                          const char *operation) {
  std::int64_t total = 1;
  for (const std::int64_t extent : spaces) {
    if (extent <= 0)
      throw std::invalid_argument(std::string(operation) +
                                  ": subsystem sizes must be positive");
    if (total > std::numeric_limits<std::int64_t>::max() / extent)
      throw std::overflow_error(std::string(operation) +
                                ": Hilbert-space size overflows int64");
    total *= extent;
  }
  return total;
}

void require_numeric(const Series &value, const char *operation) {
  if (!value.defined())
    throw std::invalid_argument(std::string(operation) +
                                ": expected a defined Series");
  if (!value.dtype().is_floating() && !value.dtype().is_complex())
    throw std::invalid_argument(
        std::string(operation) +
        ": values must have a floating or complex dtype");
  if (value.numel() == 0)
    throw std::invalid_argument(std::string(operation) +
                                ": values must not be empty");
  const bool finite = value.dtype().is_complex()
                          ? value.real().isfinite().all().item<bool>() &&
                                value.imag().isfinite().all().item<bool>()
                          : value.isfinite().all().item<bool>();
  if (!finite)
    throw std::invalid_argument(std::string(operation) +
                                ": values must be finite");
}

std::vector<std::int64_t> resolve_spaces(std::vector<std::int64_t> spaces,
                                         std::int64_t size,
                                         const char *operation) {
  if (spaces.empty())
    spaces.push_back(size);
  if (checked_size(spaces, operation) != size)
    throw std::invalid_argument(
        std::string(operation) +
        ": subsystem product must match the Hilbert axis");
  return spaces;
}

void require_same_space(const Ket &left, const Ket &right,
                        const char *operation) {
  if (left.spaces() != right.spaces())
    throw std::invalid_argument(
        std::string(operation) +
        ": states must describe the same Hilbert space");
  if (left.value().platform() != right.value().platform())
    throw std::invalid_argument(std::string(operation) +
                                ": states must share a platform");
}

void require_same_space(const Op &left, const Op &right,
                        const char *operation) {
  if (left.spaces() != right.spaces())
    throw std::invalid_argument(
        std::string(operation) +
        ": operators must describe the same Hilbert space");
  if (left.value().platform() != right.value().platform())
    throw std::invalid_argument(std::string(operation) +
                                ": operators must share a platform");
}

double default_tolerance(const Series &value) {
  switch (value.scalar_type()) {
  case ScalarType::Float64:
  case ScalarType::Complex128:
    return 1e-14;
  case ScalarType::Float32:
  case ScalarType::Complex64:
    return 1e-6;
  case ScalarType::Float16:
  case ScalarType::BFloat16:
    return 1e-3;
  default:
    return 1e-12;
  }
}

std::vector<std::int64_t> combined_shape(const Series &value,
                                         std::int64_t trailing,
                                         std::int64_t final_size) {
  std::vector<std::int64_t> shape(value.sizes().begin(),
                                  value.sizes().end() - trailing);
  shape.push_back(final_size);
  return shape;
}

Series identity(const Series &like, std::int64_t size) {
  ScalarType dtype = like.scalar_type();
  if (dtype == ScalarType::Complex64)
    dtype = ScalarType::Float32;
  else if (dtype == ScalarType::Complex128)
    dtype = ScalarType::Float64;
  Series result = Series::eye(size, size, dtype, like.platform());
  return result.scalar_type() == like.scalar_type()
             ? result
             : result.to(like.scalar_type());
}

Series batched_trace(const Series &value) {
  std::vector<std::int64_t> mask_shape(static_cast<std::size_t>(value.dim()),
                                       1);
  mask_shape[mask_shape.size() - 2] = value.size(-2);
  mask_shape.back() = value.size(-1);
  return value.mul(identity(value, value.size(-1)).reshape(mask_shape))
      .sum(-1)
      .sum(-1);
}

ScalarType real_dtype(const Series &value) {
  return value.scalar_type() == ScalarType::Float32 ||
                 value.scalar_type() == ScalarType::Complex64
             ? ScalarType::Float32
             : ScalarType::Float64;
}

Series minus_i(const Series &value) {
  const bool complex = value.dtype().is_complex();
  const Series real = complex ? value.imag() : value.mul(0.0);
  const Series imag = complex ? value.real().neg() : value.neg();
  const ScalarType dtype = real_dtype(value) == ScalarType::Float32
                               ? ScalarType::Complex64
                               : ScalarType::Complex128;
  return pluss::series::complex(real, imag, dtype);
}

ScalarType joint_dtype(const Series &left, const Series &right) {
  return pluss::codem::promote_types(left.scalar_type(), right.scalar_type());
}

Series cast_to(const Series &value, ScalarType dtype) {
  return value.scalar_type() == dtype ? value : value.to(dtype);
}

Series matmul(const Series &left, const Series &right) {
  const ScalarType dtype = joint_dtype(left, right);
  return cast_to(left, dtype).matmul(cast_to(right, dtype));
}

void require_tolerance(double tolerance, const char *operation) {
  if (!std::isfinite(tolerance) || tolerance < 0.0)
    throw std::invalid_argument(std::string(operation) +
                                ": tolerance must be finite and non-negative");
}

void require_hermitian(const Op &value, double tolerance,
                       const char *operation) {
  require_tolerance(tolerance, operation);
  if (!is_herm(value, tolerance, tolerance).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": expected a Hermitian operator");
}

pluss::subsp::linearalg::EigenDecomposition
eigen(const Op &value, double tolerance, const char *operation) {
  require_hermitian(value, tolerance, operation);
  return value.value().linearalg.eigh(false);
}

pluss::subsp::linearalg::EigenDecomposition
require_density(const Op &state, double tolerance, const char *operation) {
  auto decomposition = eigen(state, tolerance, operation);
  if (!decomposition.values.ge(-tolerance).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": density operator must be positive semidefinite");
  const Series tr = trace(state);
  const Series real_ok = tr.real().isclose(tr.real().mul(0.0).add(1.0),
                                           tolerance, tolerance, false);
  const Series imag_ok = tr.dtype().is_complex() ? tr.imag().abs().le(tolerance)
                                                 : tr.mul(0.0).eq(0.0);
  if (!real_ok.logical_and(imag_ok).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": density operator must have unit trace");
  return decomposition;
}

Series psd_sqrt(const Op &state, double tolerance, const char *operation) {
  const auto decomposition = require_density(state, tolerance, operation);
  const Series roots = decomposition.values.clamp_min(0.0).sqrt();
  return decomposition.vectors.mul(roots.unsqueeze(-2))
      .matmul(decomposition.vectors.adjoint());
}

void require_same_space(const Op &op_value, const Ket &state,
                        const char *operation) {
  if (op_value.spaces() != state.spaces())
    throw std::invalid_argument(std::string(operation) +
                                ": operator and state must describe the same "
                                "Hilbert space");
  if (op_value.value().platform() != state.value().platform())
    throw std::invalid_argument(std::string(operation) +
                                ": operator and state must share a platform");
}

std::vector<double> time_values(const Series &times, const char *operation) {
  require_numeric(times, operation);
  if (!times.dtype().is_floating() || times.dim() != 1)
    throw std::invalid_argument(std::string(operation) +
                                ": times must be a one-dimensional real "
                                "floating Series");
  std::vector<double> values =
      times.to(Platform::CPU, ScalarType::Float64).host<double>();
  for (std::size_t index = 1; index < values.size(); ++index) {
    if (!(values[index] > values[index - 1]))
      throw std::invalid_argument(std::string(operation) +
                                  ": times must be strictly increasing");
  }
  return values;
}

Series stack_states(const std::vector<Series> &states,
                    std::int64_t trailing_axes) {
  const std::int64_t axis = states.front().dim() - trailing_axes;
  Series first = states.front().unsqueeze(axis);
  if (states.size() == 1)
    return first;
  std::vector<Series> rest;
  rest.reserve(states.size() - 1);
  for (std::size_t index = 1; index < states.size(); ++index)
    rest.push_back(states[index].unsqueeze(axis));
  return first.unite(rest, axis);
}

void require_solve_options(const SolveOptions &options, const char *operation) {
  if (options.steps_per_span <= 0 || !std::isfinite(options.hbar) ||
      options.hbar <= 0.0 || !(options.relative_tolerance > 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance > 0.0) ||
      !std::isfinite(options.absolute_tolerance) ||
      !(options.initial_step >= 0.0) || !std::isfinite(options.initial_step) ||
      !(options.minimum_step >= 0.0) || !std::isfinite(options.minimum_step) ||
      !(options.maximum_step > 0.0) || std::isnan(options.maximum_step) ||
      options.max_steps <= 0 || options.minimum_step > options.maximum_step)
    throw std::invalid_argument(
        std::string(operation) +
        ": invalid integration, tolerance, or Planck-scale option");
}

Series evolve(const pluss::subsp::math::ode::LinearOdeFn &rhs, Series initial,
              const std::vector<double> &times, std::int64_t trailing_axes,
              const SolveOptions &options) {
  if (options.method == SolveMethod::Krylov)
    throw std::invalid_argument(
        "natsci::common::quantum: Krylov exponential action requires a "
        "time-independent sparse Hamiltonian");
  std::vector<Series> states;
  states.reserve(times.size());
  states.push_back(initial);
  for (std::size_t index = 1; index < times.size(); ++index) {
    if (options.method == SolveMethod::ImplicitMidpoint) {
      pluss::subsp::math::ode::LinearOdeOptions ode_options;
      ode_options.steps = options.steps_per_span;
      ode_options.relative_tolerance = options.relative_tolerance;
      ode_options.absolute_tolerance = options.absolute_tolerance;
      ode_options.initial_step = options.initial_step;
      ode_options.minimum_step = options.minimum_step;
      ode_options.maximum_step = options.maximum_step;
      ode_options.max_steps = options.max_steps;
      ode_options.solve_dims.clear();
      for (std::int64_t axis = initial.dim() - trailing_axes;
           axis < initial.dim(); ++axis)
        ode_options.solve_dims.push_back(axis);
      ode_options.linear = options.linear;
      initial = pluss::subsp::math::ode::fluxa_linear(
                    rhs, initial, times[index - 1], times[index], ode_options)
                    .y_final;
    } else {
      pluss::subsp::math::ode::OdeOptions ode_options;
      ode_options.method = options.method == SolveMethod::RK4
                               ? pluss::subsp::math::ode::OdeMethod::RK4
                               : pluss::subsp::math::ode::OdeMethod::RK45;
      ode_options.steps = options.steps_per_span;
      ode_options.relative_tolerance = options.relative_tolerance;
      ode_options.absolute_tolerance = options.absolute_tolerance;
      ode_options.initial_step = options.initial_step;
      ode_options.minimum_step = options.minimum_step;
      ode_options.maximum_step = options.maximum_step;
      ode_options.max_steps = options.max_steps;
      initial = pluss::subsp::math::ode::fluxa(
                    [&](double time, const Series &state, const Series &) {
                      return rhs(time, state);
                    },
                    initial, times[index - 1], times[index],
                    pluss::series::undefined(), ode_options)
                    .y_final;
    }
    states.push_back(initial);
  }
  return stack_states(states, trailing_axes);
}

void require_sparse_space(const SparseOp &value,
                          const std::vector<std::int64_t> &spaces,
                          Platform platform, const char *operation) {
  if (value.spaces() != spaces || value.map().values().platform() != platform)
    throw std::invalid_argument(
        std::string(operation) +
        ": sparse operators and state must share a Hilbert space and "
        "platform");
}

Series sparse_left(const SparseOp &value, const Series &matrix) {
  return value.map().apply_columns(matrix);
}

Series sparse_right(const Series &matrix, const SparseOp &value) {
  return value.map().adjoint().apply_columns(matrix.adjoint()).adjoint();
}

void require_sparse_hermitian(
    const pluss::subsp::math::linear_operator::CooMap &map,
    const char *operation) {
  const auto rows = map.row_index().to(Platform::CPU).host<std::int64_t>();
  const auto columns =
      map.column_index().to(Platform::CPU).host<std::int64_t>();
  const auto values = map.values()
                          .to(Platform::CPU, ScalarType::Complex128)
                          .host<std::complex<double>>();
  std::map<std::pair<std::int64_t, std::int64_t>, std::complex<double>>
      canonical;
  for (std::size_t index = 0; index < values.size(); ++index)
    canonical[{rows[index], columns[index]}] += values[index];
  const double tolerance =
      map.values().scalar_type() == ScalarType::Float32 ||
              map.values().scalar_type() == ScalarType::Complex64
          ? 1e-6
          : 1e-12;
  for (const auto &[position, value] : canonical) {
    const auto reverse = canonical.find({position.second, position.first});
    const std::complex<double> paired =
        reverse == canonical.end() ? std::complex<double>{} : reverse->second;
    const double scale = std::max({1.0, std::abs(value), std::abs(paired)});
    if (std::abs(value - std::conj(paired)) > tolerance * scale)
      throw std::domain_error(std::string(operation) +
                              ": Hermitian sparse contract is false");
  }
}

} // namespace

namespace detail {
struct Access final {
  static Ket ket(Series value, std::vector<std::int64_t> spaces) {
    return Ket(std::move(value), std::move(spaces));
  }
  static Op op(Series value, std::vector<std::int64_t> spaces) {
    return Op(std::move(value), std::move(spaces));
  }
  static Channel channel(std::vector<Op> kraus,
                         std::vector<std::int64_t> spaces) {
    return Channel(std::move(kraus), std::move(spaces));
  }
  static SparseOp sparse_op(pluss::subsp::math::linear_operator::CooMap map,
                            std::vector<std::int64_t> spaces, bool hermitian) {
    return SparseOp(std::move(map), std::move(spaces), hermitian);
  }
};
} // namespace detail

std::vector<std::int64_t> Ket::batch_shape() const {
  return leading_shape(value_, 1);
}

std::vector<std::int64_t> Op::batch_shape() const {
  return leading_shape(value_, 2);
}

Ket ket(Series amplitudes, std::vector<std::int64_t> spaces) {
  constexpr const char *operation = "natsci::common::quantum::ket";
  require_numeric(amplitudes, operation);
  if (amplitudes.dim() < 1)
    throw std::invalid_argument(
        "natsci::common::quantum::ket: amplitudes must have shape [..., N]");
  spaces = resolve_spaces(std::move(spaces), amplitudes.size(-1), operation);
  return detail::Access::ket(std::move(amplitudes), std::move(spaces));
}

Op op(Series values, std::vector<std::int64_t> spaces) {
  constexpr const char *operation = "natsci::common::quantum::op";
  require_numeric(values, operation);
  if (values.dim() < 2 || values.size(-2) != values.size(-1))
    throw std::invalid_argument(
        "natsci::common::quantum::op: values must have shape [..., N, N]");
  spaces = resolve_spaces(std::move(spaces), values.size(-1), operation);
  return detail::Access::op(std::move(values), std::move(spaces));
}

SparseOp sparse_op(std::int64_t size, Series row_index, Series column_index,
                   Series values, std::vector<std::int64_t> spaces,
                   bool hermitian) {
  constexpr const char *operation = "natsci::common::quantum::sparse_op";
  spaces = resolve_spaces(std::move(spaces), size, operation);
  auto map = pluss::subsp::math::linear_operator::coo(
      size, size, std::move(row_index), std::move(column_index),
      std::move(values));
  if (hermitian)
    require_sparse_hermitian(map, operation);
  return detail::Access::sparse_op(std::move(map), std::move(spaces),
                                   hermitian);
}

Series norm(const Ket &state) {
  return state.value().abs().square().sum(-1).sqrt();
}

Ket normalize(const Ket &state, double tolerance) {
  if (tolerance < 0.0 || !std::isfinite(tolerance))
    throw std::invalid_argument("natsci::common::quantum::normalize: tolerance "
                                "must be finite and non-negative");
  if (tolerance == 0.0)
    tolerance = default_tolerance(state.value());
  const Series magnitude = norm(state);
  if (!magnitude.gt(tolerance).all().item<bool>())
    throw std::domain_error("natsci::common::quantum::normalize: every state "
                            "must have non-zero norm");
  return detail::Access::ket(state.value().div(magnitude.unsqueeze(-1)),
                             state.spaces());
}

Series inner(const Ket &left, const Ket &right) {
  require_same_space(left, right, "natsci::common::quantum::inner");
  return left.value().conj().mul(right.value()).sum(-1);
}

Series prob(const Ket &state) { return state.value().abs().square(); }

Op dag(const Op &value) {
  return detail::Access::op(value.value().adjoint(), value.spaces());
}

Series trace(const Op &value) { return batched_trace(value.value()); }

Series is_herm(const Op &value, double rtol, double atol) {
  if (rtol < 0.0 || atol < 0.0 || !std::isfinite(rtol) || !std::isfinite(atol))
    throw std::invalid_argument("natsci::common::quantum::is_herm: tolerances "
                                "must be finite and non-negative");
  return value.value()
      .isclose(value.value().adjoint(), rtol, atol, false)
      .all(-1)
      .all(-1);
}

Op density(const Ket &state) {
  const Series values =
      state.value().unsqueeze(-1).mul(state.value().conj().unsqueeze(-2));
  return detail::Access::op(values, state.spaces());
}

Series expect(const Op &observable, const Ket &state) {
  if (observable.spaces() != state.spaces())
    throw std::invalid_argument(
        "natsci::common::quantum::expect: observable and state must describe "
        "the same Hilbert space");
  if (observable.value().platform() != state.value().platform())
    throw std::invalid_argument("natsci::common::quantum::expect: observable "
                                "and state must share a platform");
  const ScalarType dtype = joint_dtype(observable.value(), state.value());
  const Series amplitudes = cast_to(state.value(), dtype);
  const Series applied = cast_to(observable.value(), dtype)
                             .matmul(amplitudes.unsqueeze(-1))
                             .squeeze(-1);
  return amplitudes.conj().mul(applied).sum(-1);
}

Series expect(const Op &observable, const Op &state) {
  require_same_space(observable, state, "natsci::common::quantum::expect");
  return batched_trace(matmul(state.value(), observable.value()));
}

Ket apply(const SparseOp &value, const Ket &state) {
  require_sparse_space(value, state.spaces(), state.value().platform(),
                       "natsci::common::quantum::apply");
  return detail::Access::ket(value.map().apply(state.value()), state.spaces());
}

Series purity(const Op &state) {
  return batched_trace(state.value().matmul(state.value()));
}

Spectrum spectrum(const Op &value, double tolerance) {
  constexpr const char *operation = "natsci::common::quantum::spectrum";
  auto decomposition = eigen(value, tolerance, operation);
  return {std::move(decomposition.values),
          detail::Access::op(std::move(decomposition.vectors), value.spaces())};
}

Ket ground(const Op &value, double tolerance) {
  const Spectrum decomposition = spectrum(value, tolerance);
  return detail::Access::ket(decomposition.vectors.value().select(-1, 0),
                             value.spaces());
}

Series entropy(const Op &state, double base, double tolerance) {
  constexpr const char *operation = "natsci::common::quantum::entropy";
  if (!std::isfinite(base) || base <= 0.0 || base == 1.0)
    throw std::invalid_argument(
        "natsci::common::quantum::entropy: base must be finite, positive, "
        "and different from one");
  const auto decomposition = require_density(state, tolerance, operation);
  const Series probabilities = decomposition.values.clamp_min(0.0);
  const Series terms = probabilities.gt(tolerance).where(
      probabilities.mul(probabilities.clamp_min(tolerance).log()),
      probabilities.mul(0.0));
  return terms.sum(-1).neg().div(std::log(base));
}

Series root_fidelity(const Ket &left, const Ket &right) {
  return inner(left, right).abs().clamp(0.0, 1.0);
}

Series root_fidelity(const Ket &left, const Op &right, double tolerance) {
  require_same_space(right, left, "natsci::common::quantum::root_fidelity");
  require_density(right, tolerance, "natsci::common::quantum::root_fidelity");
  return expect(right, left).real().clamp(0.0, 1.0).sqrt();
}

Series root_fidelity(const Op &left, const Op &right, double tolerance) {
  require_same_space(left, right, "natsci::common::quantum::root_fidelity");
  const Series root =
      psd_sqrt(left, tolerance, "natsci::common::quantum::root_fidelity");
  require_density(right, tolerance, "natsci::common::quantum::root_fidelity");
  const ScalarType dtype = joint_dtype(root, right.value());
  const Series promoted_root = cast_to(root, dtype);
  const Series product =
      promoted_root.matmul(cast_to(right.value(), dtype)).matmul(promoted_root);
  const Series hermitian = product.add(product.adjoint()).mul(0.5);
  const auto values =
      pluss::subsp::linearalg::eigvalsh(hermitian, false).clamp_min(0.0);
  return values.sqrt().sum(-1).clamp(0.0, 1.0);
}

Series fidelity(const Ket &left, const Ket &right) {
  return root_fidelity(left, right).square();
}

Series fidelity(const Ket &left, const Op &right, double tolerance) {
  return root_fidelity(left, right, tolerance).square().clamp(0.0, 1.0);
}

Series fidelity(const Op &left, const Op &right, double tolerance) {
  return root_fidelity(left, right, tolerance).square().clamp(0.0, 1.0);
}

Ket tensor(const Ket &left, const Ket &right) {
  if (left.value().platform() != right.value().platform())
    throw std::invalid_argument(
        "natsci::common::quantum::tensor: states must share a platform");
  if (left.size() > std::numeric_limits<std::int64_t>::max() / right.size())
    throw std::overflow_error(
        "natsci::common::quantum::tensor: Hilbert-space size overflows int64");
  const std::int64_t size = left.size() * right.size();
  const Series product =
      left.value().unsqueeze(-1).mul(right.value().unsqueeze(-2));
  std::vector<std::int64_t> spaces = left.spaces();
  spaces.insert(spaces.end(), right.spaces().begin(), right.spaces().end());
  return detail::Access::ket(product.reshape(combined_shape(product, 2, size)),
                             std::move(spaces));
}

Op tensor(const Op &left, const Op &right) {
  if (left.value().platform() != right.value().platform())
    throw std::invalid_argument(
        "natsci::common::quantum::tensor: operators must share a platform");
  if (left.size() > std::numeric_limits<std::int64_t>::max() / right.size())
    throw std::overflow_error(
        "natsci::common::quantum::tensor: Hilbert-space size overflows int64");
  const std::int64_t size = left.size() * right.size();
  const Series product = left.value().unsqueeze(-1).unsqueeze(-3).mul(
      right.value().unsqueeze(-2).unsqueeze(-4));
  std::vector<std::int64_t> shape(product.sizes().begin(),
                                  product.sizes().end() - 4);
  shape.insert(shape.end(), {size, size});
  std::vector<std::int64_t> spaces = left.spaces();
  spaces.insert(spaces.end(), right.spaces().begin(), right.spaces().end());
  return detail::Access::op(product.reshape(shape), std::move(spaces));
}

Op partial_trace(const Op &state, std::vector<std::int64_t> keep) {
  constexpr const char *operation = "natsci::common::quantum::partial_trace";
  const std::int64_t count = static_cast<std::int64_t>(state.spaces().size());
  std::sort(keep.begin(), keep.end());
  if (std::adjacent_find(keep.begin(), keep.end()) != keep.end())
    throw std::invalid_argument(std::string(operation) +
                                ": kept subsystem indices must be unique");
  if (!keep.empty() && (keep.front() < 0 || keep.back() >= count))
    throw std::out_of_range(std::string(operation) +
                            ": kept subsystem index is out of range");

  std::vector<std::int64_t> traced;
  traced.reserve(static_cast<std::size_t>(count) - keep.size());
  for (std::int64_t index = 0; index < count; ++index) {
    if (!std::binary_search(keep.begin(), keep.end(), index))
      traced.push_back(index);
  }

  std::vector<std::int64_t> shape = state.batch_shape();
  shape.insert(shape.end(), state.spaces().begin(), state.spaces().end());
  shape.insert(shape.end(), state.spaces().begin(), state.spaces().end());
  Series reduced = state.value().reshape(shape);
  const std::int64_t batch_rank =
      static_cast<std::int64_t>(state.batch_shape().size());
  std::int64_t remaining = count;
  for (auto it = traced.rbegin(); it != traced.rend(); ++it) {
    const std::int64_t row_axis = batch_rank + *it;
    const std::int64_t column_axis = batch_rank + remaining + *it;
    const std::int64_t extent = state.spaces()[static_cast<std::size_t>(*it)];
    std::vector<std::int64_t> mask_shape(
        static_cast<std::size_t>(reduced.dim()), 1);
    mask_shape[static_cast<std::size_t>(row_axis)] = extent;
    mask_shape[static_cast<std::size_t>(column_axis)] = extent;
    reduced = reduced.mul(identity(reduced, extent).reshape(mask_shape))
                  .sum(column_axis)
                  .sum(row_axis);
    --remaining;
  }

  std::vector<std::int64_t> spaces;
  spaces.reserve(keep.size());
  std::int64_t size = 1;
  for (const std::int64_t index : keep) {
    spaces.push_back(state.spaces()[static_cast<std::size_t>(index)]);
    size *= spaces.back();
  }
  if (spaces.empty())
    spaces.push_back(1);
  std::vector<std::int64_t> result_shape(reduced.sizes().begin(),
                                         reduced.sizes().begin() + batch_rank);
  result_shape.insert(result_shape.end(), {size, size});
  return detail::Access::op(reduced.reshape(result_shape), std::move(spaces));
}

Op unitary(const Op &hamiltonian, double time, double hbar) {
  constexpr const char *operation = "natsci::common::quantum::unitary";
  if (!std::isfinite(time) || !std::isfinite(hbar) || hbar <= 0.0)
    throw std::invalid_argument(
        "natsci::common::quantum::unitary: time must be finite and hbar must "
        "be finite and positive");
  require_hermitian(hamiltonian, 1e-9, operation);
  Series generator = minus_i(hamiltonian.value().mul(time / hbar));
  return detail::Access::op(generator.linearalg.matrix_exp(),
                            hamiltonian.spaces());
}

Ket schrod(const Op &hamiltonian, const Ket &initial, const Series &times,
           double hbar) {
  constexpr const char *operation = "natsci::common::quantum::schrod";
  require_same_space(hamiltonian, initial, operation);
  require_hermitian(hamiltonian, 1e-9, operation);
  if (!std::isfinite(hbar) || hbar <= 0.0)
    throw std::invalid_argument(
        "natsci::common::quantum::schrod: hbar must be finite and positive");
  const std::vector<double> checked_times = time_values(times, operation);
  const Series t =
      times.sub(checked_times.front())
          .to(hamiltonian.value().platform(), real_dtype(hamiltonian.value()))
          .reshape({times.numel(), 1, 1});
  const Series generators =
      minus_i(hamiltonian.value().unsqueeze(-3).mul(t).div(hbar));
  const Series propagators = generators.linearalg.matrix_exp();
  const ScalarType dtype = joint_dtype(propagators, initial.value());
  const Series amplitudes = cast_to(initial.value(), dtype);
  const Series states = cast_to(propagators, dtype)
                            .matmul(amplitudes.unsqueeze(-2).unsqueeze(-1))
                            .squeeze(-1);
  return detail::Access::ket(states, initial.spaces());
}

Ket schrod(const OpFn &hamiltonian, const Ket &initial, const Series &times,
           const SolveOptions &options) {
  constexpr const char *operation = "natsci::common::quantum::schrod";
  if (!hamiltonian)
    throw std::invalid_argument(std::string(operation) +
                                ": Hamiltonian callback is required");
  require_solve_options(options, operation);
  const auto checked_times = time_values(times, operation);
  ScalarType dtype = initial.value().scalar_type();
  for (const double time : checked_times) {
    const Op value = hamiltonian(time);
    require_same_space(value, initial, operation);
    require_hermitian(value, 1e-9, operation);
    dtype = pluss::codem::promote_types(dtype, value.value().scalar_type());
  }
  const auto rhs = [&](double time, const Series &state) {
    const Op value = hamiltonian(time);
    require_same_space(value, initial, operation);
    return minus_i(matmul(value.value(), state.unsqueeze(-1)).squeeze(-1))
        .div(options.hbar);
  };
  const Series path =
      evolve(rhs, cast_to(initial.value(), dtype), checked_times, 1, options);
  return detail::Access::ket(path, initial.spaces());
}

Ket schrod(const SparseOp &hamiltonian, const Ket &initial, const Series &times,
           const SolveOptions &options) {
  constexpr const char *operation = "natsci::common::quantum::schrod";
  require_solve_options(options, operation);
  require_sparse_space(hamiltonian, initial.spaces(),
                       initial.value().platform(), operation);
  if (!hamiltonian.hermitian())
    throw std::domain_error(std::string(operation) +
                            ": sparse Hamiltonian must carry the Hermitian "
                            "contract");
  const auto checked_times = time_values(times, operation);
  const ScalarType dtype = pluss::codem::promote_types(
      initial.value().scalar_type(), hamiltonian.map().values().scalar_type());
  const auto rhs = [&](double, const Series &state) {
    return minus_i(hamiltonian.map().apply(state)).div(options.hbar);
  };
  if (options.method == SolveMethod::Krylov) {
    pluss::subsp::math::krylov::ExpOptions exp_options;
    exp_options.max_iterations = options.linear.max_iterations;
    exp_options.relative_tolerance = options.relative_tolerance;
    exp_options.absolute_tolerance = options.absolute_tolerance;
    const Series start = cast_to(initial.value(), dtype);
    std::vector<Series> states;
    states.reserve(checked_times.size());
    for (const double time : checked_times) {
      auto evolved = pluss::subsp::math::krylov::exp_action(
          [&](const Series &state) { return rhs(time, state); }, start, {-1},
          time - checked_times.front(), exp_options);
      if (!evolved.converged.all().item<bool>())
        throw std::runtime_error(
            "natsci::common::quantum::schrod: Krylov subspace did not meet "
            "the requested tolerance");
      states.push_back(std::move(evolved.value));
    }
    return detail::Access::ket(stack_states(states, 1), initial.spaces());
  }
  const Series path =
      evolve(rhs, cast_to(initial.value(), dtype), checked_times, 1, options);
  return detail::Access::ket(path, initial.spaces());
}

Channel channel(std::vector<Op> kraus, double tolerance) {
  constexpr const char *operation = "natsci::common::quantum::channel";
  require_tolerance(tolerance, operation);
  if (kraus.empty())
    throw std::invalid_argument(
        "natsci::common::quantum::channel: at least one Kraus operator is "
        "required");
  const auto spaces = kraus.front().spaces();
  const Platform platform = kraus.front().value().platform();
  Series closure =
      kraus.front().value().adjoint().matmul(kraus.front().value());
  for (std::size_t index = 1; index < kraus.size(); ++index) {
    if (kraus[index].spaces() != spaces ||
        kraus[index].value().platform() != platform)
      throw std::invalid_argument(
          "natsci::common::quantum::channel: Kraus operators must share a "
          "Hilbert space and platform");
    closure = closure.add(
        kraus[index].value().adjoint().matmul(kraus[index].value()));
  }
  const Series expected = identity(closure, closure.size(-1));
  if (!closure.isclose(expected, tolerance, tolerance, false)
           .all()
           .item<bool>())
    throw std::domain_error(
        "natsci::common::quantum::channel: Kraus operators are not trace "
        "preserving");
  return detail::Access::channel(std::move(kraus), spaces);
}

Op apply(const Channel &map, const Ket &state) {
  return apply(map, density(state));
}

Op apply(const Channel &map, const Op &state) {
  constexpr const char *operation = "natsci::common::quantum::apply";
  if (map.spaces() != state.spaces())
    throw std::invalid_argument(std::string(operation) +
                                ": channel and state must describe the same "
                                "Hilbert space");
  if (map.kraus().front().value().platform() != state.value().platform())
    throw std::invalid_argument(std::string(operation) +
                                ": channel and state must share a platform");
  const auto apply_one = [&](const Op &kraus) {
    const ScalarType dtype = joint_dtype(kraus.value(), state.value());
    const Series value = cast_to(kraus.value(), dtype);
    return value.matmul(cast_to(state.value(), dtype)).matmul(value.adjoint());
  };
  Series result = apply_one(map.kraus().front());
  for (std::size_t index = 1; index < map.kraus().size(); ++index) {
    result = result.add(apply_one(map.kraus()[index]));
  }
  return detail::Access::op(std::move(result), state.spaces());
}

Op lindblad(const Op &hamiltonian, const Op &initial, const Series &times,
            const std::vector<Op> &collapse, const SolveOptions &options) {
  constexpr const char *operation = "natsci::common::quantum::lindblad";
  require_same_space(hamiltonian, initial, operation);
  require_hermitian(hamiltonian, 1e-9, operation);
  require_density(initial, 1e-9, operation);
  require_solve_options(options, operation);
  for (const Op &value : collapse)
    require_same_space(hamiltonian, value, operation);
  const std::vector<double> t = time_values(times, operation);

  ScalarType dtype = joint_dtype(hamiltonian.value(), initial.value());
  for (const Op &value : collapse)
    dtype = pluss::codem::promote_types(dtype, value.value().scalar_type());
  const Series hamiltonian_value = cast_to(hamiltonian.value(), dtype);
  std::vector<Series> collapse_values;
  collapse_values.reserve(collapse.size());
  for (const Op &value : collapse)
    collapse_values.push_back(cast_to(value.value(), dtype));

  const auto rhs = [&](double, const Series &rho) {
    Series derivative = minus_i(hamiltonian_value.matmul(rho).sub(
                                    rho.matmul(hamiltonian_value)))
                            .div(options.hbar);
    for (const Series &c : collapse_values) {
      const Series cdag = c.adjoint();
      const Series gram = cdag.matmul(c);
      derivative = derivative.add(c.matmul(rho).matmul(cdag).sub(
          gram.matmul(rho).add(rho.matmul(gram)).mul(0.5)));
    }
    return derivative;
  };

  return detail::Access::op(
      evolve(rhs, cast_to(initial.value(), dtype), t, 2, options),
      initial.spaces());
}

Op lindblad(const OpFn &hamiltonian, const Op &initial, const Series &times,
            const CollapseFn &collapse, const SolveOptions &options) {
  constexpr const char *operation = "natsci::common::quantum::lindblad";
  if (!hamiltonian)
    throw std::invalid_argument(std::string(operation) +
                                ": Hamiltonian callback is required");
  require_density(initial, 1e-9, operation);
  require_solve_options(options, operation);
  const auto t = time_values(times, operation);
  ScalarType dtype = initial.value().scalar_type();
  for (const double time : t) {
    const Op h = hamiltonian(time);
    require_same_space(h, initial, operation);
    require_hermitian(h, 1e-9, operation);
    dtype = pluss::codem::promote_types(dtype, h.value().scalar_type());
    if (collapse) {
      for (const Op &value : collapse(time)) {
        require_same_space(h, value, operation);
        dtype = pluss::codem::promote_types(dtype, value.value().scalar_type());
      }
    }
  }

  const auto rhs = [&](double time, const Series &rho) {
    const Op h = hamiltonian(time);
    require_same_space(h, initial, operation);
    const Series h_value = cast_to(h.value(), dtype);
    Series derivative =
        minus_i(h_value.matmul(rho).sub(rho.matmul(h_value))).div(options.hbar);
    if (collapse) {
      for (const Op &value : collapse(time)) {
        require_same_space(h, value, operation);
        const Series c = cast_to(value.value(), dtype);
        const Series cdag = c.adjoint();
        const Series gram = cdag.matmul(c);
        derivative = derivative.add(c.matmul(rho).matmul(cdag).sub(
            gram.matmul(rho).add(rho.matmul(gram)).mul(0.5)));
      }
    }
    return derivative;
  };
  return detail::Access::op(
      evolve(rhs, cast_to(initial.value(), dtype), t, 2, options),
      initial.spaces());
}

Op lindblad(const SparseOp &hamiltonian, const Op &initial, const Series &times,
            const std::vector<SparseOp> &collapse,
            const SolveOptions &options) {
  constexpr const char *operation = "natsci::common::quantum::lindblad";
  require_density(initial, 1e-9, operation);
  require_solve_options(options, operation);
  require_sparse_space(hamiltonian, initial.spaces(),
                       initial.value().platform(), operation);
  if (!hamiltonian.hermitian())
    throw std::domain_error(std::string(operation) +
                            ": sparse Hamiltonian must carry the Hermitian "
                            "contract");
  ScalarType dtype = pluss::codem::promote_types(
      initial.value().scalar_type(), hamiltonian.map().values().scalar_type());
  for (const SparseOp &value : collapse) {
    require_sparse_space(value, initial.spaces(), initial.value().platform(),
                         operation);
    dtype =
        pluss::codem::promote_types(dtype, value.map().values().scalar_type());
  }
  const auto t = time_values(times, operation);
  const auto rhs = [&](double, const Series &rho) {
    Series derivative =
        minus_i(
            sparse_left(hamiltonian, rho).sub(sparse_right(rho, hamiltonian)))
            .div(options.hbar);
    for (const SparseOp &value : collapse) {
      const SparseOp adjoint = detail::Access::sparse_op(
          value.map().adjoint(), value.spaces(), value.hermitian());
      const Series c_rho = sparse_left(value, rho);
      const Series gain = sparse_right(c_rho, adjoint);
      const Series gram_left = sparse_left(adjoint, c_rho);
      const Series rho_cdag = sparse_right(rho, adjoint);
      const Series gram_right = sparse_right(rho_cdag, value);
      derivative = derivative.add(gain.sub(gram_left.add(gram_right).mul(0.5)));
    }
    return derivative;
  };
  return detail::Access::op(
      evolve(rhs, cast_to(initial.value(), dtype), t, 2, options),
      initial.spaces());
}

} // namespace pluss::subsp::natsci::common::quantum
