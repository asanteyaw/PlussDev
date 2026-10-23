#include "CodeM/core/Series.h"
#include "pluss/subsp/math/diff.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace pluss::codem {
// Series method implementations live in OpsInternal.cpp.

Series Series::calc_diff_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          std::int64_t order,
                          const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.diff: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.diff: function is required");
  PLUSS_CHECK(order >= 0, "Series.calc.diff: order must be non-negative");

  if (!options.elementwise) {
    pluss::subsp::math::diff::DerivativePolicy policy;
    policy.strict_dependency = options.strict_dependency;
    return pluss::subsp::math::diff::nth_diff(
        std::move(callback), input, order,
        pluss::subsp::math::diff::HvpOptions{}, policy);
  }

  // Elementwise derivatives d^n fn_i / d x_i^n: one reverse pass per order on
  // the sum of the outputs, each pass keeping the graph of its own gradient so
  // the next order can differentiate it. Cost is O(order) passes, not N^order.
  Series leaf = input.detach();
  pluss::subsp::math::diff::detail::set_tracks_slope_(leaf, true);
  Series current = callback(leaf);
  PLUSS_CHECK(current.defined(),
              "Series.calc.diff: function returned an undefined Series");
  PLUSS_CHECK(
      current.sizes() == input.sizes(),
      "Series.calc.diff: elementwise mode needs fn(x) to have the shape of x");
  for (std::int64_t level = 1; level <= order; ++level) {
    pluss::codem::series_tape::clear_sensitivity(leaf);
    const bool last = level == order;
    if (pluss::subsp::math::diff::detail::has_reverse_path_(current)) {
      current.sum().reverse(/*keep_graph=*/!last,
                            /*accumulate_sensitivity=*/false,
                            /*track_higher_order=*/!last);
    }
    if (leaf.has_sensitivity()) {
      current = leaf.sensitivity();
    } else {
      PLUSS_CHECK(!options.strict_dependency,
                  "Series.calc.diff: the function does not depend on x at "
                  "derivative order " +
                      std::to_string(level));
      current = pluss::series::zeros(input.sizes(), input.scalar_type())
                    .to(input.platform());
    }
  }
  return current;
}

namespace {

pluss::subsp::math::diff::DerivativePolicy
calc_policy(const Series::CalcAccessor::CalcOptions &options) {
  pluss::subsp::math::diff::DerivativePolicy policy;
  policy.strict_dependency = options.strict_dependency;
  return policy;
}

std::function<Series(const Series &)>
scalar_objective(std::function<Series(const Series &)> callback,
                 const char *operation) {
  return [callback = std::move(callback), operation](const Series &value) {
    Series output = callback(value);
    PLUSS_CHECK(output.defined(),
                std::string(operation) +
                    ": function returned an undefined Series");
    PLUSS_CHECK(output.dim() == 0,
                std::string(operation) +
                    ": function must return a rank-zero scalar Series");
    return output;
  };
}

} // namespace

Series Series::calc_grad_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.grad: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.grad: function is required");
  return pluss::subsp::math::diff::nth_diff(
      scalar_objective(std::move(callback), "Series.calc.grad"), input, 1,
      pluss::subsp::math::diff::HvpOptions{}, calc_policy(options));
}

Series Series::calc_jac_(const Series &input,
                         std::function<Series(const Series &)> callback,
                         const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.jac: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.jac: function is required");
  return pluss::subsp::math::diff::nth_diff(
      std::move(callback), input, 1, pluss::subsp::math::diff::HvpOptions{},
      calc_policy(options));
}

Series Series::calc_hess_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.hess: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.hess: function is required");
  return pluss::subsp::math::diff::nth_diff(
      scalar_objective(std::move(callback), "Series.calc.hess"), input, 2,
      pluss::subsp::math::diff::HvpOptions{}, calc_policy(options));
}

Series Series::calc_antidiff_(
    const Series &upper, std::function<Series(const Series &)> callback,
    double lower, const SeriesCalcAccessor::AntidiffOptions &options) {
  PLUSS_CHECK(upper.defined(),
              "Series.calc.antidiff: upper endpoint must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.antidiff: integrand callback is required");
  PLUSS_CHECK(
      upper.dtype().is_floating() && !upper.dtype().is_complex(),
      "Series.calc.antidiff: upper endpoints must have a real floating dtype");
  PLUSS_CHECK(std::isfinite(lower),
              "Series.calc.antidiff: lower endpoint must be finite");
  PLUSS_CHECK(options.order >= 1,
              "Series.calc.antidiff: order must be at least one");
  PLUSS_CHECK(options.intervals >= 1,
              "Series.calc.antidiff: intervals must be positive");
  PLUSS_CHECK(options.max_refinements >= 0,
              "Series.calc.antidiff: max_refinements must be non-negative");
  PLUSS_CHECK(std::isfinite(options.atol) && options.atol >= 0.0,
              "Series.calc.antidiff: atol must be finite and non-negative");
  PLUSS_CHECK(std::isfinite(options.rtol) && options.rtol >= 0.0,
              "Series.calc.antidiff: rtol must be finite and non-negative");
  PLUSS_CHECK(options.initial.empty() ||
                  options.initial.size() ==
                      static_cast<std::size_t>(options.order),
              "Series.calc.antidiff: initial must be empty or contain one "
              "value per order");
  PLUSS_CHECK(options.rule == SeriesCalcAccessor::Rule::Trapezoid ||
                  options.rule == SeriesCalcAccessor::Rule::Simpson,
              "Series.calc.antidiff: rule is not recognized");

  if (upper.numel() == 0)
    return upper.clone();

  auto evaluate = [&](std::int64_t requested_intervals) {
    std::int64_t intervals = std::max<std::int64_t>(requested_intervals, 1);
    if (options.rule == SeriesCalcAccessor::Rule::Simpson) {
      intervals = std::max<std::int64_t>(intervals, 2);
      if (intervals % 2 != 0)
        ++intervals;
    }

    const std::int64_t nodes = intervals + 1;
    const auto dtype = upper.scalar_type();
    const auto platform = upper.platform();
    const Series unit = Series::linspace(0.0, 1.0, nodes, dtype, platform);
    const Series delta = upper.sub(lower);
    const Series points = delta.unsqueeze(-1).mul(unit).add(lower);
    Series values = callback(points);
    PLUSS_CHECK(values.defined(),
                "Series.calc.antidiff: integrand returned an undefined Series");
    PLUSS_CHECK(values.platform() == platform,
                "Series.calc.antidiff: integrand output must remain on the "
                "endpoint platform");
    PLUSS_CHECK(values.sizes() == points.sizes(),
                "Series.calc.antidiff: integrand output shape must match the "
                "quadrature-node shape");
    PLUSS_CHECK(
        values.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
        "Series.calc.antidiff: the integrand returned a non-finite value (nan "
        "or inf) at a "
        "quadrature node; the integral may be singular or the integrand "
        "undefined on "
        "[lower, upper]");

    if (options.order > 1) {
      values = values.mul(
          unit.mul(-1.0).add(1.0).pow(static_cast<double>(options.order - 1)));
    }

    Series unit_integral = pluss::series::undefined();
    if (options.rule == SeriesCalcAccessor::Rule::Trapezoid) {
      unit_integral =
          values.antidiff.trapezoid(1.0 / static_cast<double>(intervals), -1);
    } else {
      const Series indices =
          pluss::series::arange(0, nodes, 1, ScalarType::Int64).to(platform);
      const Series one =
          pluss::series::full({nodes}, 1.0, values.scalar_type()).to(platform);
      const Series two =
          pluss::series::full({nodes}, 2.0, values.scalar_type()).to(platform);
      const Series four =
          pluss::series::full({nodes}, 4.0, values.scalar_type()).to(platform);
      Series weights = indices.remainder(2.0).eq(0.0).where(two, four);
      const Series endpoint = indices.eq(0.0).logical_or(indices.eq(nodes - 1));
      weights = endpoint.where(one, weights);
      unit_integral = values.mul(weights).sum(-1).mul(
          1.0 / (3.0 * static_cast<double>(intervals)));
    }

    Series result =
        unit_integral.mul(delta.pow(static_cast<double>(options.order)) /
                          std::tgamma(static_cast<double>(options.order)));
    for (std::int64_t k = 0; k < options.order; ++k) {
      const double initial = options.initial.empty()
                                 ? 0.0
                                 : options.initial[static_cast<std::size_t>(k)];
      if (initial == 0.0)
        continue;
      if (k == 0) {
        result = result.add(initial);
      } else {
        result = result.add(
            delta.pow(static_cast<double>(k))
                .mul(initial / std::tgamma(static_cast<double>(k + 1))));
      }
    }
    // One value per endpoint, shaped like the endpoints (a 0-d endpoint stays
    // 0-d).
    return result.reshape(upper.sizes());
  };

  Series estimate = evaluate(options.intervals);
  if (!options.adaptive)
    return estimate;

  double last_error = std::numeric_limits<double>::infinity();
  double last_tolerance = 0.0;
  std::int64_t intervals = options.intervals;
  for (std::int64_t refinement = 0; refinement < options.max_refinements;
       ++refinement) {
    PLUSS_CHECK(intervals <= std::numeric_limits<std::int64_t>::max() / 2,
                "Series.calc.antidiff: interval refinement overflows int64");
    intervals *= 2;
    Series refined = evaluate(intervals);
    last_error = refined.sub(estimate)
                     .abs()
                     .max()
                     .to(pluss::pCPU, ScalarType::Float64)
                     .item<double>();
    const double scale =
        refined.abs().max().to(pluss::pCPU, ScalarType::Float64).item<double>();
    last_tolerance = options.atol + options.rtol * scale;
    estimate = std::move(refined);
    if (last_error <= last_tolerance)
      return estimate;
  }

  if (options.require_convergence) {
    PLUSS_FAIL(
        "Series.calc.antidiff: adaptive quadrature did not converge; error=" +
        std::to_string(last_error) +
        ", tolerance=" + std::to_string(last_tolerance));
  }
  return estimate;
}
} // namespace pluss::codem

// ---------------------------------------------------------------------------
// Stochastic calculus on Series: Ito/Stratonovich differentials of a function
// of processes, and stochastic integrals along a path.
// ---------------------------------------------------------------------------
namespace pluss::codem {
namespace {

namespace dd = pluss::subsp::math::diff;

std::vector<std::int64_t> shape_of(const Series &s) {
  return std::vector<std::int64_t>(s.sizes().begin(), s.sizes().end());
}

std::vector<std::int64_t> with_leading(std::int64_t lead,
                                       const std::vector<std::int64_t> &rest) {
  std::vector<std::int64_t> out;
  out.reserve(rest.size() + 1);
  out.push_back(lead);
  out.insert(out.end(), rest.begin(), rest.end());
  return out;
}

bool is_real_floating(const Series &s) {
  return s.dtype().is_floating() && !s.dtype().is_complex();
}

// sigma of shape S (one factor) -> S + [1]; shape S + [m] stays.
Series normalize_sigma(const Series &sigma, const std::vector<std::int64_t> &S,
                       const char *op) {
  PLUSS_CHECK(sigma.defined(), std::string(op) + ": sigma must be defined");
  const auto sh = shape_of(sigma);
  if (sh == S)
    return sigma.unsqueeze(-1);
  PLUSS_CHECK(sh.size() == S.size() + 1 &&
                  std::equal(S.begin(), S.end(), sh.begin()),
              std::string(op) +
                  ": sigma must have the shape of the state, or the shape of "
                  "the state followed by one axis of Brownian factors");
  return sigma;
}

// A [K] (or [K]+S) jump array broadcast against [K]+S.
Series jump_array(const Series &a, std::int64_t K,
                  const std::vector<std::int64_t> &S, const char *op) {
  PLUSS_CHECK(a.defined() && a.dim() >= 1 && a.size(0) == K,
              std::string(op) + ": jump arrays must have K atoms on axis 0");
  if (a.dim() == 1) {
    std::vector<std::int64_t> shape(1 + S.size(), 1);
    shape[0] = K;
    return a.reshape(shape);
  }
  PLUSS_CHECK(shape_of(a) == with_leading(K, S),
              std::string(op) + ": a jump array must be [K] or [K] + shape(x)");
  return a;
}

Series fetch_sensitivity(const Series &leaf, const Series &zeros_like_s) {
  return leaf.has_sensitivity() ? leaf.sensitivity() : zeros_like_s;
}

} // namespace

SeriesItoDifferential Series::calc_ito_(
    const Series &x, const SeriesCalcAccessor::ItoFn &g, const Series &mu,
    const Series &sigma, const std::vector<SeriesItoState> &others,
    const SeriesJumpMeasure *jumps, const SeriesItoOptions &options,
    bool ito_convention,
    const std::function<std::vector<Series>(const std::vector<Series> &)>
        *marcus_h) {
  const bool marcus = marcus_h != nullptr;
  const char *op = marcus ? "Series.calc.diff.marcus"
                  : ito_convention ? "Series.calc.diff.ito"
                                  : "Series.calc.diff.strat";
  PLUSS_CHECK(x.defined(), std::string(op) + ": the state must be defined");
  PLUSS_CHECK(static_cast<bool>(g), std::string(op) + ": function is required");
  PLUSS_CHECK(is_real_floating(x),
              std::string(op) + ": states must have a real floating dtype");

  std::vector<Series> xs{x}, mus{mu}, sigs;
  const auto S = shape_of(x);
  sigs.push_back(normalize_sigma(sigma, S, op));
  for (const auto &o : others) {
    xs.push_back(o.x);
    mus.push_back(o.mu);
    sigs.push_back(normalize_sigma(o.sigma, S, op));
  }
  const std::size_t k = xs.size();
  const std::int64_t m = sigs[0].size(-1);
  for (std::size_t i = 0; i < k; ++i) {
    PLUSS_CHECK(xs[i].defined() && shape_of(xs[i]) == S,
                std::string(op) + ": all states must have the same shape");
    PLUSS_CHECK(mus[i].defined() && shape_of(mus[i]) == S,
                std::string(op) + ": each drift must have the shape of its state");
    PLUSS_CHECK(xs[i].platform() == x.platform() &&
                    xs[i].scalar_type() == x.scalar_type(),
                std::string(op) + ": all states must share dtype and platform");
    PLUSS_CHECK(sigs[i].size(-1) == m,
                std::string(op) +
                    ": all states must be driven by the same number of Brownian "
                    "factors");
  }
  if (jumps != nullptr && jumps->intensity.defined()) {
    PLUSS_CHECK(ito_convention || marcus,
                "Series.calc.diff.strat: a Stratonovich convention for jumps "
                "is the Marcus one; call calc.diff.marcus(...)");
    PLUSS_CHECK(marcus ? jumps->sizes.size() == 1 : jumps->sizes.size() == k,
                marcus ? std::string(op) +
                             ": a Marcus jump measure holds the sizes of the "
                             "scalar driver in sizes[0]"
                       : std::string(op) +
                             ": jumps need one size array per state");
  }

  const auto dtype = x.scalar_type();
  const auto platform = x.platform();
  const Series zeros_s = Series::zeros(S, dtype, platform);
  const bool has_time = options.time.defined();
  Series t_full = pluss::series::undefined();
  if (has_time)
    t_full = zeros_s.add(options.time.to(platform, dtype));

  // Differentiable leaves, detached from the caller's graph so the derivatives
  // below are exact partials of g at the point.
  std::vector<Series> leaves;
  for (const auto &s : xs) {
    Series leaf = s.detach();
    dd::detail::set_tracks_slope_(leaf, true);
    leaves.push_back(leaf);
  }
  Series t_leaf = pluss::series::undefined();
  if (has_time) {
    t_leaf = t_full.detach();
    dd::detail::set_tracks_slope_(t_leaf, true);
  }
  std::vector<Series> args;
  if (has_time)
    args.push_back(t_leaf);
  args.insert(args.end(), leaves.begin(), leaves.end());

  Series y = g(args);
  PLUSS_CHECK(y.defined(),
              std::string(op) + ": the function returned an undefined Series");
  PLUSS_CHECK(shape_of(y) == S,
              std::string(op) +
                  ": the function must act elementwise (output shape equals "
                  "the shape of the state)");

  auto clear_all = [&]() {
    for (const auto &leaf : leaves)
      pluss::codem::series_tape::clear_sensitivity(leaf);
    if (has_time)
      pluss::codem::series_tape::clear_sensitivity(t_leaf);
  };

  // First derivatives: one reverse pass on the sum (the points are independent,
  // so the sensitivity of each leaf is the elementwise partial).
  std::vector<Series> grads(k, zeros_s);
  Series g_t = zeros_s;
  clear_all();
  if (dd::detail::has_reverse_path_(y)) {
    y.sum().reverse(/*keep_graph=*/true, /*accumulate_sensitivity=*/false,
                    /*track_higher_order=*/ito_convention);
    for (std::size_t i = 0; i < k; ++i)
      grads[i] = fetch_sensitivity(leaves[i], zeros_s);
    if (has_time)
      g_t = fetch_sensitivity(t_leaf, zeros_s);
  }

  // Second derivatives (Ito only): row i of the Hessian from the derivative of
  // the sum of grads[i].
  std::vector<std::vector<Series>> hess(k, std::vector<Series>(k, zeros_s));
  if (ito_convention) {
    for (std::size_t i = 0; i < k; ++i) {
      if (!dd::detail::has_reverse_path_(grads[i]))
        continue;
      clear_all();
      grads[i].sum().reverse(/*keep_graph=*/true,
                             /*accumulate_sensitivity=*/false,
                             /*track_higher_order=*/false);
      for (std::size_t j = 0; j < k; ++j)
        hess[i][j] = fetch_sensitivity(leaves[j], zeros_s);
    }
  }
  clear_all();

  std::vector<Series> gd;
  for (const auto &gr : grads)
    gd.push_back(gr.detach());

  Series drift = has_time ? g_t.detach() : zeros_s;
  for (std::size_t i = 0; i < k; ++i)
    drift = drift.add(gd[i].mul(mus[i]));
  if (ito_convention) {
    for (std::size_t i = 0; i < k; ++i) {
      for (std::size_t j = 0; j < k; ++j) {
        const Series cov = sigs[i].mul(sigs[j]).sum(-1); // d[X_i, X_j]/dt
        drift = drift.add(hess[i][j].detach().mul(cov).mul(0.5));
      }
    }
  }
  Series diffusion = Series::zeros(with_leading(0, S).size() == 0 ? S : S, dtype,
                                   platform);
  diffusion = Series::zeros(
      [&] {
        auto sh = S;
        sh.push_back(m);
        return sh;
      }(),
      dtype, platform);
  for (std::size_t i = 0; i < k; ++i)
    diffusion = diffusion.add(gd[i].unsqueeze(-1).mul(sigs[i]));

  SeriesItoDifferential out;
  out.value = y.detach();

  if (jumps != nullptr && jumps->intensity.defined()) {
    const std::int64_t K = jumps->intensity.size(0);
    PLUSS_CHECK(K >= 1, std::string(op) +
                            ": a jump measure needs at least one atom");
    std::vector<Series> shifted_args;
    std::vector<Series> z;      // per-state additive sizes (Ito)
    std::vector<Series> hx;     // jump coefficient at the point (Marcus)
    Series zk = pluss::series::undefined();
    if (has_time) {
      shifted_args.push_back(zeros_s.unsqueeze(0).expand(with_leading(K, S)).add(
          t_full.unsqueeze(0)));
    }
    if (marcus) {
      zk = jump_array(jumps->sizes[0].to(platform, dtype), K, S, op);
      std::vector<Series> detached;
      for (const auto &s : xs)
        detached.push_back(s.detach());
      hx = (*marcus_h)(detached);
      PLUSS_CHECK(hx.size() == k,
                  std::string(op) + ": the jump coefficient must return one "
                                    "array per state");
      // Time-one flow of dy/du = h(y) z for every atom, by Runge-Kutta.
      const auto stacked_zero = Series::zeros(with_leading(K, S), dtype, platform);
      std::vector<Series> y;
      for (const auto &s : xs)
        y.push_back(s.detach().unsqueeze(0).add(stacked_zero));
      const std::int64_t nsteps = std::max<std::int64_t>(options.flow_steps, 1);
      const double du = 1.0 / static_cast<double>(nsteps);
      auto field = [&](const std::vector<Series> &state) {
        auto hv = (*marcus_h)(state);
        PLUSS_CHECK(hv.size() == k,
                    std::string(op) + ": the jump coefficient must return one "
                                      "array per state");
        for (auto &v : hv)
          v = v.mul(zk);
        return hv;
      };
      auto axpy = [&](const std::vector<Series> &base,
                      const std::vector<Series> &dir, double c) {
        std::vector<Series> out_state;
        for (std::size_t j = 0; j < k; ++j)
          out_state.push_back(base[j].add(dir[j].mul(c)));
        return out_state;
      };
      for (std::int64_t step = 0; step < nsteps; ++step) {
        const auto k1 = field(y);
        const auto k2 = field(axpy(y, k1, 0.5 * du));
        const auto k3 = field(axpy(y, k2, 0.5 * du));
        const auto k4 = field(axpy(y, k3, du));
        for (std::size_t j = 0; j < k; ++j)
          y[j] = y[j].add(k1[j].add(k2[j].mul(2.0)).add(k3[j].mul(2.0)).add(k4[j]).mul(du / 6.0));
      }
      for (std::size_t j = 0; j < k; ++j)
        shifted_args.push_back(y[j]);
    } else {
      for (std::size_t j = 0; j < k; ++j) {
        z.push_back(jump_array(jumps->sizes[j].to(platform, dtype), K, S, op));
        shifted_args.push_back(xs[j].detach().unsqueeze(0).add(z.back()));
      }
    }
    const Series y_shift = g(shifted_args);
    PLUSS_CHECK(y_shift.defined() && shape_of(y_shift) == with_leading(K, S),
                std::string(op) + ": the function must accept the stacked "
                                  "[K] + shape(x) arguments used for the jump "
                                  "atoms");
    const Series dg = y_shift.detach().sub(out.value.unsqueeze(0));
    const Series nu = jump_array(jumps->intensity.to(platform, dtype), K, S, op);
    if (jumps->compensated) {
      Series linear = Series::zeros(with_leading(K, S), dtype, platform);
      for (std::size_t j = 0; j < k; ++j)
        linear = linear.add(marcus ? gd[j].unsqueeze(0).mul(hx[j].unsqueeze(0)).mul(zk)
                                   : gd[j].unsqueeze(0).mul(z[j]));
      drift = drift.add(nu.mul(dg.sub(linear)).sum(0));
    }
    out.jump_sizes = dg;
    out.jump_intensity = nu;
    out.jumps_compensated = jumps->compensated;
  }

  out.drift = drift;
  out.diffusion = diffusion;
  return out;
}

Series Series::calc_convention_drift_(
    const Series &x, const Series &drift,
    const std::function<Series(const Series &)> &sigma, double sign) {
  PLUSS_CHECK(x.defined() && drift.defined(),
              "Series.calc.diff: state and drift must be defined");
  PLUSS_CHECK(static_cast<bool>(sigma),
              "Series.calc.diff: sigma callback is required");
  PLUSS_CHECK(shape_of(drift) == shape_of(x),
              "Series.calc.diff: the drift must have the shape of the state");
  const auto S = shape_of(x);
  Series leaf = x.detach();
  dd::detail::set_tracks_slope_(leaf, true);
  Series sig = normalize_sigma(sigma(leaf), S, "Series.calc.diff");
  Series correction = Series::zeros(S, x.scalar_type(), x.platform());
  const std::int64_t m = sig.size(-1);
  for (std::int64_t f = 0; f < m; ++f) {
    const Series column = sig.select(-1, f);
    pluss::codem::series_tape::clear_sensitivity(leaf);
    if (dd::detail::has_reverse_path_(column)) {
      column.sum().reverse(true, false, false);
      if (leaf.has_sensitivity())
        correction = correction.add(column.detach().mul(leaf.sensitivity()));
    }
  }
  pluss::codem::series_tape::clear_sensitivity(leaf);
  // Ito -> Stratonovich: mu_S = mu_I - 1/2 sigma sigma' (sign = -1);
  // Stratonovich -> Ito: mu_I = mu_S + 1/2 sigma sigma' (sign = +1).
  return drift.add(correction.mul(0.5 * sign));
}

Series Series::calc_stochastic_integral_(
    const Series &path, const SeriesCalcAccessor::IntegrandFn &f,
    const Series &times, double theta, std::int64_t dim, bool cumulative) {
  const char *op = "Series.calc.antidiff (stochastic integral)";
  PLUSS_CHECK(path.defined(), std::string(op) + ": the path must be defined");
  PLUSS_CHECK(static_cast<bool>(f),
              std::string(op) + ": integrand callback is required");
  PLUSS_CHECK(is_real_floating(path),
              std::string(op) + ": the path must have a real floating dtype");
  PLUSS_CHECK(theta >= 0.0 && theta <= 1.0 && std::isfinite(theta),
              std::string(op) + ": theta must lie in [0, 1]");
  const std::int64_t rank = path.dim();
  PLUSS_CHECK(rank >= 1, std::string(op) + ": the path needs a time axis");
  if (dim < 0)
    dim += rank;
  PLUSS_CHECK(dim >= 0 && dim < rank, std::string(op) + ": dim out of range");
  const std::int64_t n = path.size(dim);
  PLUSS_CHECK(n >= 2, std::string(op) + ": the path needs at least two samples");
  PLUSS_CHECK(times.defined() && times.dim() == 1 && times.size(0) == n,
              std::string(op) +
                  ": the time grid must be one-dimensional with one entry per "
                  "path sample");
  const Series t = times.to(path.platform(), path.scalar_type());
  PLUSS_CHECK(t.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
              std::string(op) + ": the time grid must be finite");
  PLUSS_CHECK((t.diff(1, 0) > 0.0).all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
              std::string(op) + ": the time grid must be strictly increasing");

  std::vector<std::int64_t> tshape(static_cast<std::size_t>(rank), 1);
  tshape[static_cast<std::size_t>(dim)] = n;
  const Series t_b = t.reshape(tshape);

  const Series left = path.slice(dim, 0, n - 1, 1);
  const Series right = path.slice(dim, 1, n, 1);
  const Series dw = right.sub(left);

  Series integrand = pluss::series::undefined();
  if (theta == 0.0) {
    integrand = f(t_b.slice(dim, 0, n - 1, 1), left);
    // An integrand that depends on time alone is allowed to return the broadcastable shape of `t`.
    if (integrand.defined() && shape_of(integrand) != shape_of(left))
      integrand = integrand.add(Series::zeros(shape_of(left), left.scalar_type(), left.platform()));
  } else {
    Series full = f(t_b, path);
    if (full.defined() && shape_of(full) != shape_of(path))
      full = full.add(Series::zeros(shape_of(path), path.scalar_type(), path.platform()));
    PLUSS_CHECK(full.defined() && shape_of(full) == shape_of(path),
                std::string(op) +
                    ": the integrand must return one value per path sample "
                    "(shape of the path)");
    const Series f_left = full.slice(dim, 0, n - 1, 1);
    const Series f_right = full.slice(dim, 1, n, 1);
    integrand = f_left.mul(1.0 - theta).add(f_right.mul(theta));
  }
  PLUSS_CHECK(integrand.defined() && shape_of(integrand) == shape_of(left),
              std::string(op) +
                  ": the integrand must act pointwise (output shape equals the "
                  "shape of its path argument)");
  PLUSS_CHECK(
      integrand.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
      std::string(op) + ": the integrand returned a non-finite value");

  const Series terms = integrand.mul(dw);
  if (!cumulative)
    return terms.sum(dim);
  const Series zero = Series::zeros(shape_of(terms), terms.scalar_type(),
                                    terms.platform())
                          .slice(dim, 0, 1, 1);
  return zero.unite({terms.cumsum(dim)}, dim);
}

// ---- accessor methods ------------------------------------------------------

namespace {
using ItoFn = Series::CalcAccessor::ItoFn;

ItoFn wrap1(const std::function<Series(const Series &)> &g) {
  return [g](const std::vector<Series> &a) { return g(a.at(0)); };
}
ItoFn wrap2(const std::function<Series(const Series &, const Series &)> &g) {
  return [g](const std::vector<Series> &a) { return g(a.at(0), a.at(1)); };
}
} // namespace

SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito(
    const std::function<Series(const Series &)> &g, const Series &mu,
    const Series &sigma) const {
  return Series::calc_ito_(owner(), wrap1(g), mu, sigma, {}, nullptr,
                           SeriesItoOptions{}, true);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito(
    const std::function<Series(const Series &)> &g, const Series &mu,
    const Series &sigma, const SeriesJumpMeasure &jumps) const {
  return Series::calc_ito_(owner(), wrap1(g), mu, sigma, {}, &jumps,
                           SeriesItoOptions{}, true);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito(
    const std::function<Series(const Series &, const Series &)> &g,
    const Series &mu, const Series &sigma, const SeriesItoState &other) const {
  return Series::calc_ito_(owner(), wrap2(g), mu, sigma, {other}, nullptr,
                           SeriesItoOptions{}, true);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito(
    const std::function<Series(const Series &, const Series &)> &g,
    const Series &mu, const Series &sigma, const SeriesItoState &other,
    const SeriesJumpMeasure &jumps) const {
  return Series::calc_ito_(owner(), wrap2(g), mu, sigma, {other}, &jumps,
                           SeriesItoOptions{}, true);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito_product(
    const Series &mu, const Series &sigma, const SeriesItoState &other) const {
  return ito([](const Series &a, const Series &b) { return a.mul(b); }, mu,
             sigma, other);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito_quotient(
    const Series &mu, const Series &sigma, const SeriesItoState &other) const {
  return ito([](const Series &a, const Series &b) { return a.div(b); }, mu,
             sigma, other);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito(
    const ItoFn &g, const Series &mu, const Series &sigma,
    const std::vector<SeriesItoState> &others, const SeriesJumpMeasure *jumps,
    const SeriesItoOptions &options) const {
  return Series::calc_ito_(owner(), g, mu, sigma, others, jumps, options, true);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::ito_at(
    const Series &time,
    const std::function<Series(const Series &, const Series &)> &g,
    const Series &mu, const Series &sigma) const {
  SeriesItoOptions o;
  o.time = time;
  return Series::calc_ito_(owner(), wrap2(g), mu, sigma, {}, nullptr, o, true);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::strat(
    const std::function<Series(const Series &)> &g, const Series &mu,
    const Series &sigma) const {
  return Series::calc_ito_(owner(), wrap1(g), mu, sigma, {}, nullptr,
                           SeriesItoOptions{}, false);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::strat(
    const std::function<Series(const Series &, const Series &)> &g,
    const Series &mu, const Series &sigma, const SeriesItoState &other) const {
  return Series::calc_ito_(owner(), wrap2(g), mu, sigma, {other}, nullptr,
                           SeriesItoOptions{}, false);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::strat(
    const ItoFn &g, const Series &mu, const Series &sigma,
    const std::vector<SeriesItoState> &others,
    const SeriesItoOptions &options) const {
  return Series::calc_ito_(owner(), g, mu, sigma, others, nullptr, options,
                           false);
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::marcus(
    const std::function<Series(const Series &)> &g, const Series &mu,
    const Series &sigma, const std::function<Series(const Series &)> &h,
    const SeriesJumpMeasure &jumps) const {
  return marcus(g, mu, sigma, h, jumps, SeriesItoOptions{});
}
SeriesItoDifferential Series::CalcAccessor::DiffAccessor::marcus(
    const std::function<Series(const Series &)> &g, const Series &mu,
    const Series &sigma, const std::function<Series(const Series &)> &h,
    const SeriesJumpMeasure &jumps, const SeriesItoOptions &options) const {
  PLUSS_CHECK(static_cast<bool>(h),
              "Series.calc.diff.marcus: jump coefficient callback is required");
  const std::function<std::vector<Series>(const std::vector<Series> &)> hv =
      [h](const std::vector<Series> &s) { return std::vector<Series>{h(s.at(0))}; };
  return Series::calc_ito_(owner(), wrap1(g), mu, sigma, {}, &jumps, options,
                           false, &hv);
}
Series Series::CalcAccessor::DiffAccessor::marcus_flow(
    const std::function<Series(const Series &)> &h, const Series &z,
    std::int64_t steps) const {
  PLUSS_CHECK(static_cast<bool>(h),
              "Series.calc.diff.marcus_flow: vector field callback is required");
  PLUSS_CHECK(z.defined(), "Series.calc.diff.marcus_flow: z must be defined");
  const Series &x0 = owner();
  PLUSS_CHECK(x0.defined(),
              "Series.calc.diff.marcus_flow: the start must be defined");
  const std::int64_t n = std::max<std::int64_t>(steps, 1);
  const double du = 1.0 / static_cast<double>(n);
  Series y = x0.detach().add(Series::zeros(shape_of(x0), x0.scalar_type(), x0.platform()));
  auto field = [&](const Series &s) { return h(s).mul(z); };
  for (std::int64_t i = 0; i < n; ++i) {
    const Series k1 = field(y);
    const Series k2 = field(y.add(k1.mul(0.5 * du)));
    const Series k3 = field(y.add(k2.mul(0.5 * du)));
    const Series k4 = field(y.add(k3.mul(du)));
    y = y.add(k1.add(k2.mul(2.0)).add(k3.mul(2.0)).add(k4).mul(du / 6.0));
  }
  return y;
}
Series Series::CalcAccessor::DiffAccessor::malliavin(
    const std::function<Series(const Series &)> &F, std::int64_t dim) const {
  return Series::calc_malliavin_(owner(), F, dim, false);
}
Series Series::CalcAccessor::DiffAccessor::malliavin_path(
    const std::function<Series(const Series &)> &F, std::int64_t dim) const {
  return Series::calc_malliavin_(owner(), F, dim, true);
}
Series Series::CalcAccessor::DiffAccessor::skorokhod(
    const std::function<Series(const Series &)> &u, double dt,
    std::int64_t dim) const {
  return Series::calc_skorokhod_(owner(), u, dt, dim);
}
Series Series::CalcAccessor::DiffAccessor::strat_drift(
    const Series &ito_drift,
    const std::function<Series(const Series &)> &sigma) const {
  return Series::calc_convention_drift_(owner(), ito_drift, sigma, -1.0);
}
Series Series::CalcAccessor::DiffAccessor::ito_drift(
    const Series &strat_drift,
    const std::function<Series(const Series &)> &sigma) const {
  return Series::calc_convention_drift_(owner(), strat_drift, sigma, 1.0);
}

namespace {
Series uniform_times(const Series &path, std::int64_t dim, double t0, double t1) {
  PLUSS_CHECK(path.defined(), "Series.calc.antidiff: the path must be defined");
  const std::int64_t rank = path.dim();
  PLUSS_CHECK(rank >= 1, "Series.calc.antidiff: the path needs a time axis");
  if (dim < 0)
    dim += rank;
  PLUSS_CHECK(dim >= 0 && dim < rank, "Series.calc.antidiff: dim out of range");
  PLUSS_CHECK(std::isfinite(t0) && std::isfinite(t1) && t1 > t0,
              "Series.calc.antidiff: need finite t0 < t1");
  return Series::linspace(t0, t1, path.size(dim), path.scalar_type(),
                          path.platform());
}
} // namespace

#define PLUSS_SERIES_STOCH_FORMS(NAME, THETA, CUM)                             \
  Series Series::CalcAccessor::AntidiffAccessor::NAME(                           \
      const IntegrandFn &f, double t0, double t1, std::int64_t dim) const {    \
    return Series::calc_stochastic_integral_(                                  \
        owner(), f, uniform_times(owner(), dim, t0, t1), THETA, dim, CUM);     \
  }                                                                            \
  Series Series::CalcAccessor::AntidiffAccessor::NAME(                         \
      const IntegrandFn &f, const Series &times, std::int64_t dim) const {     \
    return Series::calc_stochastic_integral_(owner(), f, times, THETA, dim,    \
                                             CUM);                             \
  }
PLUSS_SERIES_STOCH_FORMS(ito, 0.0, false)
PLUSS_SERIES_STOCH_FORMS(strat, 0.5, false)
PLUSS_SERIES_STOCH_FORMS(ito_cumulative, 0.0, true)
PLUSS_SERIES_STOCH_FORMS(strat_cumulative, 0.5, true)
#undef PLUSS_SERIES_STOCH_FORMS

Series Series::CalcAccessor::AntidiffAccessor::theta(const IntegrandFn &f,
                                                   double theta, double t0,
                                                   double t1,
                                                   std::int64_t dim) const {
  return Series::calc_stochastic_integral_(
      owner(), f, uniform_times(owner(), dim, t0, t1), theta, dim, false);
}
Series Series::CalcAccessor::AntidiffAccessor::theta(const IntegrandFn &f,
                                                   double theta,
                                                   const Series &times,
                                                   std::int64_t dim) const {
  return Series::calc_stochastic_integral_(owner(), f, times, theta, dim,
                                           false);
}

// ---- Malliavin derivative, Skorokhod integral, Marcus integral, array forms --

namespace {

std::int64_t normalized_dim(std::int64_t dim, std::int64_t rank, const char *op) {
  if (dim < 0)
    dim += rank;
  PLUSS_CHECK(dim >= 0 && dim < rank, std::string(op) + ": dim out of range");
  return dim;
}

// 8-point Gauss-Legendre rule on [0, 1].
constexpr double kGlNodes[8] = {0.0198550717512319, 0.1016667612931866,
                                0.2372337950418355, 0.4082826787521751,
                                0.5917173212478249, 0.7627662049581645,
                                0.8983332387068134, 0.9801449282487681};
constexpr double kGlWeights[8] = {0.0506142681451881, 0.1111905172266872,
                                  0.1568533229389436, 0.1813418916891810,
                                  0.1813418916891810, 0.1568533229389436,
                                  0.1111905172266872, 0.0506142681451881};

} // namespace

Series Series::calc_malliavin_(const Series &receiver,
                               const std::function<Series(const Series &)> &F,
                               std::int64_t dim, bool from_path) {
  const char *op = from_path ? "Series.calc.diff.malliavin_path"
                             : "Series.calc.diff.malliavin";
  PLUSS_CHECK(receiver.defined(), std::string(op) + ": the receiver must be defined");
  PLUSS_CHECK(static_cast<bool>(F), std::string(op) + ": functional is required");
  PLUSS_CHECK(is_real_floating(receiver),
              std::string(op) + ": real floating dtype required");
  dim = normalized_dim(dim, receiver.dim(), op);
  Series leaf = from_path ? receiver.detach().diff(1, dim) : receiver.detach();
  PLUSS_CHECK(leaf.size(dim) >= 1, std::string(op) + ": no increments");
  leaf = leaf.add(Series::zeros(shape_of(leaf), leaf.scalar_type(), leaf.platform())).detach();
  dd::detail::set_tracks_slope_(leaf, true);
  Series arg = leaf;
  if (from_path) {
    const Series first = receiver.detach().slice(dim, 0, 1, 1);
    arg = first.unite({first.add(leaf.cumsum(dim))}, dim);
  }
  const Series value = F(arg);
  PLUSS_CHECK(value.defined(), std::string(op) + ": functional returned an undefined Series");
  pluss::codem::series_tape::clear_sensitivity(leaf);
  if (dd::detail::has_reverse_path_(value))
    value.sum().reverse(true, false, false);
  Series out = leaf.has_sensitivity()
                   ? leaf.sensitivity().detach()
                   : Series::zeros(shape_of(leaf), leaf.scalar_type(), leaf.platform());
  pluss::codem::series_tape::clear_sensitivity(leaf);
  return out;
}

Series Series::calc_skorokhod_(const Series &increments,
                               const std::function<Series(const Series &)> &u,
                               double dt, std::int64_t dim) {
  const char *op = "Series.calc.diff.skorokhod";
  PLUSS_CHECK(increments.defined(), std::string(op) + ": increments must be defined");
  PLUSS_CHECK(static_cast<bool>(u), std::string(op) + ": integrand callback is required");
  PLUSS_CHECK(std::isfinite(dt) && dt > 0.0, std::string(op) + ": dt must be positive");
  PLUSS_CHECK(is_real_floating(increments), std::string(op) + ": real floating dtype required");
  dim = normalized_dim(dim, increments.dim(), op);
  const std::int64_t N = increments.size(dim);
  Series leaf = increments.detach().add(
      Series::zeros(shape_of(increments), increments.scalar_type(), increments.platform())).detach();
  dd::detail::set_tracks_slope_(leaf, true);
  const Series uval = u(leaf);
  PLUSS_CHECK(uval.defined() && shape_of(uval) == shape_of(leaf),
              std::string(op) + ": u must return one value per increment");
  const Series ito_part = uval.detach().mul(increments.detach()).sum(dim);
  Series correction = Series::zeros(shape_of(ito_part), ito_part.scalar_type(), ito_part.platform());
  for (std::int64_t i = 0; i < N; ++i) {
    pluss::codem::series_tape::clear_sensitivity(leaf);
    const Series ui = uval.select(dim, i);
    if (!dd::detail::has_reverse_path_(ui))
      continue;
    ui.sum().reverse(true, false, false);
    if (leaf.has_sensitivity())
      correction = correction.add(leaf.sensitivity().select(dim, i));
  }
  pluss::codem::series_tape::clear_sensitivity(leaf);
  return ito_part.sub(correction.mul(dt));
}

Series Series::calc_marcus_integral_(
    const Series &path, const SeriesCalcAccessor::IntegrandFn &f,
    const Series &times, const Series &jump_mask, std::int64_t dim,
    bool cumulative) {
  const char *op = "Series.calc.antidiff.marcus";
  PLUSS_CHECK(path.defined(), std::string(op) + ": the path must be defined");
  PLUSS_CHECK(static_cast<bool>(f), std::string(op) + ": integrand callback is required");
  PLUSS_CHECK(is_real_floating(path), std::string(op) + ": real floating dtype required");
  dim = normalized_dim(dim, path.dim(), op);
  const std::int64_t n = path.size(dim);
  PLUSS_CHECK(n >= 2, std::string(op) + ": the path needs at least two samples");
  PLUSS_CHECK(times.defined() && times.dim() == 1 && times.size(0) == n,
              std::string(op) + ": the time grid needs one entry per path sample");
  PLUSS_CHECK(jump_mask.defined(), std::string(op) + ": the jump mask must be defined");
  const Series t = times.to(path.platform(), path.scalar_type());
  PLUSS_CHECK((t.diff(1, 0) > 0.0).all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
              std::string(op) + ": the time grid must be strictly increasing");

  std::vector<std::int64_t> tshape(static_cast<std::size_t>(path.dim()), 1);
  tshape[static_cast<std::size_t>(dim)] = n;
  const Series t_b = t.reshape(tshape);
  const Series t_left = t_b.slice(dim, 0, n - 1, 1);
  const Series left = path.slice(dim, 0, n - 1, 1);
  const Series right = path.slice(dim, 1, n, 1);
  const Series dw = right.sub(left);
  const auto zero_like_left = [&] {
    return Series::zeros(shape_of(left), left.scalar_type(), left.platform());
  };
  auto broadcast_to_left = [&](Series v) {
    PLUSS_CHECK(v.defined(), std::string(op) + ": the integrand returned an undefined Series");
    if (shape_of(v) != shape_of(left))
      v = v.add(zero_like_left());
    PLUSS_CHECK(shape_of(v) == shape_of(left),
                std::string(op) + ": the integrand must act pointwise");
    return v;
  };

  // Continuous steps: Stratonovich average of the endpoint values.
  Series full = f(t_b, path);
  if (shape_of(full) != shape_of(path))
    full = full.add(Series::zeros(shape_of(path), path.scalar_type(), path.platform()));
  PLUSS_CHECK(shape_of(full) == shape_of(path),
              std::string(op) + ": the integrand must act pointwise");
  Series term = full.slice(dim, 0, n - 1, 1).add(full.slice(dim, 1, n, 1)).mul(0.5).mul(dw);

  const Series mask_f = jump_mask.to(path.platform(), path.scalar_type()).add(zero_like_left());
  const Series mask = mask_f.gt(0.5);
  if (mask.any().to(pluss::pCPU, ScalarType::Bool).item<bool>()) {
    // Jump steps: dw * integral_0^1 f(t, w + s dw) ds along the straight segment.
    Series acc = zero_like_left();
    for (int q = 0; q < 8; ++q)
      acc = acc.add(broadcast_to_left(f(t_left, left.add(dw.mul(kGlNodes[q])))).mul(kGlWeights[q]));
    term = mask.where(acc.mul(dw), term);
  }
  PLUSS_CHECK(term.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
              std::string(op) + ": the integrand returned a non-finite value");
  if (!cumulative)
    return term.sum(dim);
  const Series zero = Series::zeros(shape_of(term), term.scalar_type(), term.platform())
                          .slice(dim, 0, 1, 1);
  return zero.unite({term.cumsum(dim)}, dim);
}

Series Series::array_stochastic_integral_(const Series &values,
                                          const Series &path, double theta,
                                          std::int64_t dim, bool cumulative) {
  const char *op = "Series.antidiff (stochastic integral of samples)";
  PLUSS_CHECK(values.defined() && path.defined(),
              std::string(op) + ": integrand and path must be defined");
  PLUSS_CHECK(is_real_floating(path) && is_real_floating(values),
              std::string(op) + ": real floating dtypes required");
  PLUSS_CHECK(theta >= 0.0 && theta <= 1.0 && std::isfinite(theta),
              std::string(op) + ": theta must lie in [0, 1]");
  dim = normalized_dim(dim, path.dim(), op);
  PLUSS_CHECK(values.dim() == path.dim(),
              std::string(op) + ": integrand and path must have the same rank");
  const std::int64_t n = path.size(dim);
  PLUSS_CHECK(n >= 2, std::string(op) + ": the path needs at least two samples");
  const std::int64_t m = values.size(dim);
  PLUSS_CHECK(m == n || (m == n - 1 && theta == 0.0),
              std::string(op) +
                  ": the integrand needs one sample per path sample (n - 1 are "
                  "enough for the Ito form, which uses left endpoints only)");
  for (std::int64_t d = 0; d < path.dim(); ++d)
    if (d != dim)
      PLUSS_CHECK(values.size(d) == path.size(d),
                  std::string(op) + ": integrand and path shapes must agree off the time axis");
  PLUSS_CHECK(values.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
              std::string(op) + ": the integrand contains a non-finite value");
  const Series dw = path.slice(dim, 1, n, 1).sub(path.slice(dim, 0, n - 1, 1));
  Series integrand = values.slice(dim, 0, n - 1, 1);
  if (theta > 0.0)
    integrand = integrand.mul(1.0 - theta).add(values.slice(dim, 1, n, 1).mul(theta));
  const Series terms = integrand.mul(dw);
  if (!cumulative)
    return terms.sum(dim);
  const Series zero = Series::zeros(shape_of(terms), terms.scalar_type(), terms.platform())
                          .slice(dim, 0, 1, 1);
  return zero.unite({terms.cumsum(dim)}, dim);
}

Series Series::SeriesAntidiffProxy::ito(std::int64_t dim) const {
  return Series::array_stochastic_integral_(self(), peer1(), 0.0, dim, false);
}
Series Series::SeriesAntidiffProxy::ito_cumulative(std::int64_t dim) const {
  return Series::array_stochastic_integral_(self(), peer1(), 0.0, dim, true);
}
Series Series::SeriesAntidiffProxy::strat(std::int64_t dim) const {
  return Series::array_stochastic_integral_(self(), peer1(), 0.5, dim, false);
}
Series Series::SeriesAntidiffProxy::strat_cumulative(std::int64_t dim) const {
  return Series::array_stochastic_integral_(self(), peer1(), 0.5, dim, true);
}
Series Series::SeriesAntidiffProxy::theta(double th, std::int64_t dim) const {
  return Series::array_stochastic_integral_(self(), peer1(), th, dim, false);
}
Series Series::SeriesAntidiffProxy::quadratic_variation(std::int64_t dim) const {
  const Series &w = self();
  PLUSS_CHECK(w.defined() && is_real_floating(w), "Series.antidiff.quadratic_variation: real path required");
  dim = normalized_dim(dim, w.dim(), "Series.antidiff.quadratic_variation");
  const Series d = w.diff(1, dim);
  return d.mul(d).sum(dim);
}
Series Series::SeriesAntidiffProxy::quadratic_variation_cumulative(std::int64_t dim) const {
  const Series &w = self();
  PLUSS_CHECK(w.defined() && is_real_floating(w), "Series.antidiff.quadratic_variation: real path required");
  dim = normalized_dim(dim, w.dim(), "Series.antidiff.quadratic_variation");
  const Series d = w.diff(1, dim);
  const Series terms = d.mul(d);
  const Series zero = Series::zeros(shape_of(terms), terms.scalar_type(), terms.platform()).slice(dim, 0, 1, 1);
  return zero.unite({terms.cumsum(dim)}, dim);
}
Series Series::SeriesAntidiffProxy::covariation(std::int64_t dim) const {
  const Series &x = self();
  const Series &y = peer1();
  PLUSS_CHECK(x.defined() && y.defined() && shape_of(x) == shape_of(y),
              "Series.antidiff.covariation: the two paths must have the same shape");
  dim = normalized_dim(dim, x.dim(), "Series.antidiff.covariation");
  return x.diff(1, dim).mul(y.diff(1, dim)).sum(dim);
}
Series Series::SeriesAntidiffProxy::covariation_cumulative(std::int64_t dim) const {
  const Series &x = self();
  const Series &y = peer1();
  PLUSS_CHECK(x.defined() && y.defined() && shape_of(x) == shape_of(y),
              "Series.antidiff.covariation: the two paths must have the same shape");
  dim = normalized_dim(dim, x.dim(), "Series.antidiff.covariation");
  const Series terms = x.diff(1, dim).mul(y.diff(1, dim));
  const Series zero = Series::zeros(shape_of(terms), terms.scalar_type(), terms.platform()).slice(dim, 0, 1, 1);
  return zero.unite({terms.cumsum(dim)}, dim);
}

Series Series::CalcAccessor::AntidiffAccessor::marcus(
    const IntegrandFn &f, const Series &jump_mask, double t0, double t1,
    std::int64_t dim) const {
  return Series::calc_marcus_integral_(owner(), f, uniform_times(owner(), dim, t0, t1), jump_mask, dim, false);
}
Series Series::CalcAccessor::AntidiffAccessor::marcus(
    const IntegrandFn &f, const Series &jump_mask, const Series &times,
    std::int64_t dim) const {
  return Series::calc_marcus_integral_(owner(), f, times, jump_mask, dim, false);
}
Series Series::CalcAccessor::AntidiffAccessor::marcus(
    const IntegrandFn &f, double jump_threshold, double t0, double t1,
    std::int64_t dim) const {
  PLUSS_CHECK(std::isfinite(jump_threshold) && jump_threshold > 0.0,
              "Series.calc.antidiff.marcus: jump_threshold must be positive");
  const Series mask = owner().diff(1, dim).abs().gt(jump_threshold);
  return Series::calc_marcus_integral_(owner(), f, uniform_times(owner(), dim, t0, t1), mask, dim, false);
}
Series Series::CalcAccessor::AntidiffAccessor::marcus_cumulative(
    const IntegrandFn &f, const Series &jump_mask, double t0, double t1,
    std::int64_t dim) const {
  return Series::calc_marcus_integral_(owner(), f, uniform_times(owner(), dim, t0, t1), jump_mask, dim, true);
}

} // namespace pluss::codem

