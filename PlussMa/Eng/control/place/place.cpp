#include "pluss/subsp/eng/control.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::eng::control {
namespace {

using Complex = std::complex<double>;

bool is_complex(ScalarType dtype) noexcept {
  return dtype == ScalarType::Complex64 || dtype == ScalarType::Complex128;
}

bool is_narrow(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float32 || dtype == ScalarType::Complex64;
}

bool finite(const Series &value) {
  if (!is_complex(value.scalar_type()))
    return value.isfinite().all().item<bool>();
  return value.real().isfinite().all().item<bool>() &&
         value.imag().isfinite().all().item<bool>();
}

ScalarType complex_dtype(ScalarType dtype) noexcept {
  return is_narrow(dtype) ? ScalarType::Complex64 : ScalarType::Complex128;
}

ScalarType real_component_dtype(ScalarType dtype) noexcept {
  return is_narrow(dtype) ? ScalarType::Float32 : ScalarType::Float64;
}

Series adjoint(const Series &value) { return value.transpose(-2, -1).conj(); }

Series join_columns(const std::vector<Series> &columns,
                    std::int64_t omitted = -1) {
  std::vector<Series> retained;
  retained.reserve(columns.size() - (omitted >= 0 ? 1U : 0U));
  for (std::size_t lane = 0; lane < columns.size(); ++lane) {
    if (static_cast<std::int64_t>(lane) != omitted)
      retained.push_back(columns[lane]);
  }
  PLUSS_CHECK(!retained.empty(),
              "eng::control::place: cannot assemble an empty basis");
  if (retained.size() == 1)
    return retained.front();
  const std::vector<Series> tail(retained.begin() + 1, retained.end());
  return retained.front().unite(tail, -1, true);
}

double rank_tolerance(ScalarType dtype, double requested) {
  return std::max(requested, is_narrow(dtype) ? 2e-5 : 5e-12);
}

std::int64_t common_rank(const Series &singular, std::int64_t rows,
                         std::int64_t cols, double tolerance,
                         std::string_view name) {
  const auto values = singular.to(ScalarType::Float64).host<double>();
  const std::int64_t count = singular.size(-1);
  PLUSS_CHECK(count > 0 && values.size() % static_cast<std::size_t>(count) == 0,
              "eng::control::place: invalid singular-value layout");
  std::int64_t shared = -1;
  for (std::size_t offset = 0; offset < values.size();
       offset += static_cast<std::size_t>(count)) {
    const double scale = values[offset];
    const double threshold = tolerance *
                             static_cast<double>(std::max(rows, cols)) *
                             std::max(1.0, scale);
    std::int64_t rank = 0;
    for (std::int64_t lane = 0; lane < count; ++lane)
      rank += values[offset + static_cast<std::size_t>(lane)] > threshold;
    if (shared < 0)
      shared = rank;
    PLUSS_CHECK(rank == shared,
                std::string("eng::control::place: ") + std::string(name) +
                    " must have one numerical rank across batches");
  }
  return shared;
}

Series controllability(const Series &a, const Series &b) {
  std::vector<Series> blocks;
  blocks.reserve(static_cast<std::size_t>(a.size(-1)));
  Series term = b;
  blocks.push_back(term);
  for (std::int64_t power = 1; power < a.size(-1); ++power) {
    term = a.matmul(term);
    blocks.push_back(term);
  }
  return join_columns(blocks);
}

bool close(Complex lhs, Complex rhs, double tolerance) {
  return std::abs(lhs - rhs) <=
         tolerance * std::max({1.0, std::abs(lhs), std::abs(rhs)});
}

std::vector<std::int64_t> pair_poles(const std::vector<Complex> &poles,
                                     bool real_system, std::int64_t input_rank,
                                     double tolerance) {
  std::vector<std::int64_t> pairs(poles.size(), -1);
  std::vector<bool> used(poles.size(), false);
  for (std::size_t lane = 0; lane < poles.size(); ++lane) {
    std::int64_t multiplicity = 0;
    for (const Complex candidate : poles)
      multiplicity += close(candidate, poles[lane], tolerance);
    PLUSS_CHECK(multiplicity <= input_rank,
                "eng::control::place: pole multiplicity exceeds input rank");

    if (!real_system || std::abs(poles[lane].imag()) <= tolerance)
      continue;
    if (used[lane])
      continue;
    std::int64_t match = -1;
    for (std::size_t other = lane + 1; other < poles.size(); ++other) {
      if (!used[other] &&
          close(poles[other], std::conj(poles[lane]), tolerance)) {
        match = static_cast<std::int64_t>(other);
        break;
      }
    }
    PLUSS_CHECK(match >= 0,
                "eng::control::place: complex poles for a real system must "
                "appear in conjugate pairs");
    pairs[lane] = match;
    pairs[static_cast<std::size_t>(match)] = static_cast<std::int64_t>(lane);
    used[lane] = true;
    used[static_cast<std::size_t>(match)] = true;
  }
  return pairs;
}

Series make_poles(const std::vector<Complex> &values, const Series &like,
                  ScalarType dtype) {
  return pluss::series::make(
      values, {static_cast<std::int64_t>(values.size())},
      Config{}.with_dtype(dtype).with_platform(like.platform()));
}

Series direct_basis(std::int64_t states, const std::vector<Complex> &poles,
                    const std::vector<std::int64_t> &pairs, const Series &like,
                    ScalarType dtype) {
  std::vector<Complex> values(static_cast<std::size_t>(states * states));
  std::vector<bool> assigned(static_cast<std::size_t>(states), false);
  for (std::int64_t column = 0; column < states; ++column) {
    if (assigned[static_cast<std::size_t>(column)])
      continue;
    const std::int64_t peer = pairs[static_cast<std::size_t>(column)];
    if (peer < 0) {
      values[static_cast<std::size_t>(column * states + column)] = {1.0, 0.0};
      assigned[static_cast<std::size_t>(column)] = true;
      continue;
    }
    const bool positive = poles[static_cast<std::size_t>(column)].imag() > 0.0;
    const std::int64_t pos = positive ? column : peer;
    const std::int64_t neg = positive ? peer : column;
    values[static_cast<std::size_t>(pos * states + pos)] = {1.0, 0.0};
    values[static_cast<std::size_t>(neg * states + pos)] = {0.0, 1.0};
    values[static_cast<std::size_t>(pos * states + neg)] = {1.0, 0.0};
    values[static_cast<std::size_t>(neg * states + neg)] = {0.0, -1.0};
    assigned[static_cast<std::size_t>(pos)] = true;
    assigned[static_cast<std::size_t>(neg)] = true;
  }
  return pluss::series::make(
      values, {states, states},
      Config{}.with_dtype(dtype).with_platform(like.platform()));
}

} // namespace

PlaceResult place(const Series &a, const Series &b, const Series &poles,
                  PlaceOptions options) {
  PLUSS_CHECK(a.defined() && b.defined() && poles.defined(),
              "eng::control::place: A, B, and poles must be defined");
  PLUSS_CHECK(a.dim() >= 2 && b.dim() >= 2,
              "eng::control::place: A and B must have matrix axes");
  PLUSS_CHECK(a.size(-2) == a.size(-1),
              "eng::control::place: A must be square");
  PLUSS_CHECK(b.size(-2) == a.size(-1),
              "eng::control::place: B rows must equal the state count");
  PLUSS_CHECK(a.size(-1) > 0 && b.size(-1) > 0,
              "eng::control::place: state and input axes must be non-empty");
  PLUSS_CHECK((a.dtype().is_floating() || a.dtype().is_complex()) &&
                  (b.dtype().is_floating() || b.dtype().is_complex()),
              "eng::control::place: A and B must be floating or complex");
  PLUSS_CHECK(
      poles.dim() == 1 && poles.size(0) == a.size(-1),
      "eng::control::place: poles must be a vector with one value per state");
  PLUSS_CHECK(poles.dtype().is_floating() || poles.dtype().is_complex(),
              "eng::control::place: poles must be floating or complex");
  PLUSS_CHECK(a.platform() == b.platform() && a.platform() == poles.platform(),
              "eng::control::place: A, B, and poles must share one platform");
  PLUSS_CHECK(finite(a) && finite(b) && finite(poles),
              "eng::control::place: inputs must contain only finite values");
  PLUSS_CHECK(options.sweeps >= 0,
              "eng::control::place: sweeps must be non-negative");
  PLUSS_CHECK(std::isfinite(options.tolerance) && options.tolerance > 0.0,
              "eng::control::place: tolerance must be finite and positive");
  PLUSS_CHECK(std::isfinite(options.condition_limit) &&
                  options.condition_limit > 1.0,
              "eng::control::place: condition_limit must be finite and greater "
              "than one");

  const std::int64_t states = a.size(-1);
  const std::int64_t inputs = b.size(-1);
  ScalarType real_dtype =
      codem::promote_types(a.scalar_type(), b.scalar_type());
  const bool real_system = !is_complex(real_dtype);
  const ScalarType work_dtype = complex_dtype(real_dtype);
  const double tolerance = rank_tolerance(real_dtype, options.tolerance);
  const Series aa = a.to(work_dtype);
  const Series bb = b.to(work_dtype);
  const Series singular_b = bb.linearalg.svd_s(false);
  const std::int64_t input_rank =
      common_rank(singular_b, states, inputs, tolerance, "B");
  PLUSS_CHECK(input_rank > 0,
              "eng::control::place: B must have non-zero numerical rank");

  const Series reach = controllability(aa, bb);
  const std::int64_t reach_rank =
      common_rank(reach.linearalg.svd_s(false), reach.size(-2), reach.size(-1),
                  tolerance, "controllability matrix");
  PLUSS_CHECK(reach_rank == states,
              "eng::control::place: (A, B) must be controllable");

  const auto desired =
      poles.to(ScalarType::Complex128).host<std::complex<double>>();
  const auto pairs = pair_poles(desired, real_system, input_rank, tolerance);
  const Series desired_series = make_poles(desired, a, work_dtype);
  const Series lambda = desired_series.diag();
  const Series identity =
      pluss::series::eye(states,
                         Config{}
                             .with_dtype(real_component_dtype(work_dtype))
                             .with_platform(a.platform()))
          .to(work_dtype);

  Series basis = pluss::series::undefined();
  std::int64_t completed_sweeps = 0;
  if (input_rank == states) {
    basis =
        direct_basis(states, desired, pairs, a, work_dtype).add(aa.mul(0.0));
  } else {
    const auto b_svd = bb.linearalg.svd(true);
    const Series left_null = b_svd.u.slice(-1, input_rank, states, 1);
    const Series left_null_h = adjoint(left_null);
    std::vector<Series> spaces;
    std::vector<Series> columns;
    spaces.reserve(static_cast<std::size_t>(states));
    columns.reserve(static_cast<std::size_t>(states));
    for (std::int64_t lane = 0; lane < states; ++lane) {
      const Series shifted =
          aa.sub(identity.mul(desired_series.select(0, lane)));
      const Series constraint = left_null_h.matmul(shifted);
      const auto decomposition = constraint.linearalg.svd(true);
      const std::int64_t constraint_rank =
          common_rank(decomposition.s, constraint.size(-2), constraint.size(-1),
                      tolerance, "assignable-pole constraint");
      PLUSS_CHECK(constraint_rank == states - input_rank,
                  "eng::control::place: a requested pole is not assignable");
      Series space =
          adjoint(decomposition.vh).slice(-1, states - input_rank, states, 1);
      Series column = space.select(-1, lane % input_rank).unsqueeze(-1);
      column = column.div(column.norm(2.0, -2, true));
      spaces.push_back(std::move(space));
      columns.push_back(std::move(column));
    }
    for (std::int64_t lane = 0; lane < states; ++lane) {
      const std::int64_t peer = pairs[static_cast<std::size_t>(lane)];
      if (peer >= 0 && desired[static_cast<std::size_t>(lane)].imag() > 0.0)
        columns[static_cast<std::size_t>(peer)] =
            columns[static_cast<std::size_t>(lane)].conj();
    }

    basis = join_columns(columns);
    if (input_rank > 1) {
      for (std::int64_t sweep = 0; sweep < options.sweeps; ++sweep) {
        for (std::int64_t lane = 0; lane < states; ++lane) {
          const std::int64_t peer = pairs[static_cast<std::size_t>(lane)];
          if (peer >= 0 && desired[static_cast<std::size_t>(lane)].imag() < 0.0)
            continue;
          const Series without = join_columns(columns, lane);
          const Series q = without.linearalg.qr_q(false);
          const Series perpendicular = q.select(-1, states - 1).unsqueeze(-1);
          Series candidate = spaces[static_cast<std::size_t>(lane)].matmul(
              adjoint(spaces[static_cast<std::size_t>(lane)])
                  .matmul(perpendicular));
          const Series candidate_norm = candidate.norm(2.0, -2, true);
          const double min_norm = candidate_norm.amin().item<double>();
          if (std::isfinite(min_norm) && min_norm > std::sqrt(tolerance)) {
            candidate = candidate.div(candidate_norm);
            columns[static_cast<std::size_t>(lane)] = candidate;
            if (peer >= 0)
              columns[static_cast<std::size_t>(peer)] = candidate.conj();
          }
        }
        basis = join_columns(columns);
        completed_sweeps = sweep + 1;
      }
    }
  }

  const auto basis_svd = basis.linearalg.svd(false);
  const Series condition =
      basis_svd.s.amax(-1, false).div(basis_svd.s.amin(-1, false));
  PLUSS_CHECK(
      condition.isfinite().all().item<bool>() &&
          condition.amax().item<double>() <= options.condition_limit,
      "eng::control::place: the assigned eigenvector basis is ill-conditioned");

  const Series target = basis.matmul(lambda).matmul(basis.linearalg.inverse());
  Series gain = bb.linearalg.pinverse(tolerance).matmul(aa.sub(target));
  if (real_system) {
    const double imag_scale = gain.imag().abs().amax().item<double>();
    const double real_scale = gain.real().abs().amax().item<double>();
    PLUSS_CHECK(
        imag_scale <= 100.0 * tolerance * std::max(1.0, real_scale),
        "eng::control::place: conjugate poles did not produce a real gain");
    gain = gain.real().to(real_dtype);
  }
  const Series closed_loop =
      a.to(gain.scalar_type()).sub(b.to(gain.scalar_type()).matmul(gain));
  const Series closed_work = closed_loop.to(work_dtype);
  const Series error = closed_work.matmul(basis).sub(basis.matmul(lambda));
  const Series residual = error.abs()
                              .amax({-2, -1}, false)
                              .div(closed_work.abs()
                                       .amax({-2, -1}, false)
                                       .add(lambda.abs().amax())
                                       .clamp_min(tolerance));
  const double residual_limit = is_narrow(real_dtype) ? 5e-4 : 1e-8;
  PLUSS_CHECK(residual.isfinite().all().item<bool>() &&
                  residual.amax().item<double>() <= residual_limit,
              "eng::control::place: pole assignment residual is too large");

  return {
      .gain = gain,
      .closed_loop = closed_loop,
      .basis = basis,
      .residual = residual,
      .condition = condition,
      .sweeps = completed_sweeps,
  };
}

PlaceResult place(const lti::StateSpace &system, const Series &poles,
                  PlaceOptions options) {
  return place(system.a(), system.b(), poles, options);
}

} // namespace pluss::subsp::eng::control
