#pragma once

// Fitting enzyme progress curves: substrate concentration S(t) measured at several initial substrate levels, fitted globally (shared kinetic constants) by
// Levenberg-Marquardt (common::estimation, nlinfit-compatible, with nlparci intervals). No new optimiser, integrator or special function is introduced:
//  * integrated Michaelis-Menten (closed form through the Lambert W function of systems::enzymes::integrated_michaelis_menten, valid when E0 << S0 + Km)
//        S(t) = Km W((S0/Km) exp((S0 - Vmax t)/Km));
//  * competitive product inhibition, dS/dt = -Vmax S / (Km (1 + (S0 - S)/Kp) + S), integrated by RK4 (common::dynamics::simulate); every Levenberg-Marquardt
//    finite-difference lane and every initial concentration advances in the same Series operations.
// Parameters are fitted on their natural scale; the model uses their magnitudes so a step through zero cannot produce a non-physical rate.

#include "pluss/subsp/natsci/biology/systems/enzymes.h"
#include "pluss/subsp/natsci/common/estimation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace pluss::subsp::natsci::biology::systems::enzyme_fitting {

namespace est = ::pluss::subsp::natsci::common::estimation;

struct ProgressData final {
  Series times = pluss::series::undefined();     // [M] observation times (increasing, starting after 0)
  Series substrate = pluss::series::undefined(); // [K, M] measured substrate concentration for each initial level
  Series initial = pluss::series::undefined();   // [K] initial substrate concentration of each curve
};
struct EnzymeFit final {
  est::Fit fit;
  est::Intervals intervals;
};

// crude starting values: Km at the median initial concentration, Vmax from the steepest initial decline of the highest curve
[[nodiscard]] inline Series initial_guess(const ProgressData &d, bool product_inhibition = false) {
  const auto s0 = d.initial.contiguous().host<double>();
  auto sorted = s0;
  std::ranges::sort(sorted);
  const double km = sorted[sorted.size() / 2];
  const Series top = d.substrate.select(0, std::ranges::max_element(s0) - s0.begin());
  const double drop = (top.select(0, 0).item<double>() - d.initial.max().item<double>()) / d.times.select(0, 0).item<double>();
  std::vector<double> g{std::abs(drop) * 1.5, km};
  if (product_inhibition) g.push_back(km);
  return pluss::make_series(g, {1, static_cast<std::int64_t>(g.size())}).to(d.substrate.scalar_type());
}

namespace detail {
inline Series flatten_predictions(const std::vector<Series> &at_time) { // each [L, K] -> [L, K * M] with curve-major layout (curve k occupies columns k M .. k M + M - 1)
  const Series stacked = at_time.front().unsqueeze(-1).unite([&] { std::vector<Series> r; for (std::size_t i = 1; i < at_time.size(); ++i) r.push_back(at_time[i].unsqueeze(-1)); return r; }(), at_time.front().dim()); // [L, K, M]
  return stacked.reshape({stacked.size(0), stacked.size(1) * stacked.size(2)});
}
inline void validate(const ProgressData &d, const Series &guess, std::int64_t parameters, const char *who) {
  const std::string name = who;
  dynamics::require(d.times.defined() && d.substrate.defined() && d.initial.defined(), name + ": times, substrate and initial must all be given");
  dynamics::require(d.times.dim() == 1 && d.initial.dim() == 1 && d.substrate.dim() == 2, name + ": times and initial must be 1-D and substrate 2-D [curves, times]");
  dynamics::require(d.substrate.size(0) == d.initial.size(0) && d.substrate.size(1) == d.times.size(0),
                    name + ": substrate must be [curves, times] with one row per starting concentration and one column per time");
  dynamics::require(guess.dim() == 2 && guess.size(1) == parameters, name + ": the starting guess must be [fits, " + std::to_string(parameters) + "]");
  dynamics::require(guess.abs().gt(0.0).all().item<bool>() && guess.abs().lt(std::numeric_limits<double>::infinity()).all().item<bool>(),
                    name + ": every starting value must be non-zero and finite (a zero constant makes the rate law undefined)");
}
inline Series flatten_data(const Series &substrate) { return substrate.reshape({substrate.size(0) * substrate.size(1)}); }
[[nodiscard]] inline EnzymeFit finish(const est::ModelFn &model, const ProgressData &d, const Series &guess, const est::Options &opt) {
  EnzymeFit out;
  out.fit = est::levenberg_marquardt(model, flatten_data(d.substrate), guess, opt);
  // the models use the magnitude of every parameter, so the optimiser may stop at a negative value; report the physical (positive) one, with the Jacobian
  // taken with respect to it, so the estimate and its confidence interval describe the same quantity
  const Series sign = pluss::series::where(out.fit.beta.lt(0.0), out.fit.beta.mul(0.0).sub(1.0), out.fit.beta.mul(0.0).add(1.0));
  out.fit.beta = out.fit.beta.mul(sign);
  out.fit.jacobian = out.fit.jacobian.mul(sign.unsqueeze(1));
  // the optimiser's tolerances are relative to the size of the parameters, so a fit that runs off along a flat direction (a constant growing without
  // bound) can satisfy them. A kinetic constant a million times larger than every starting value is such a runaway: report it as not converged
  const Series limit = guess.abs().amax(-1, true).mul(1.0e6);
  const Series sound = out.fit.beta.abs().le(limit).to(out.fit.beta.scalar_type()).amin(-1, false); // 0 where any constant ran away or is not finite
  out.fit.converged = out.fit.converged.mul(sound.to(out.fit.converged.scalar_type()));
  try {
    out.intervals = est::confidence_intervals(out.fit);
  } catch (const pluss::Error &e) {
    // a constant that the data cannot determine (a runaway fit, or an inhibition constant for data with no inhibition) makes J^T J singular and has no
    // interval: keep the fit and report undefined (NaN) intervals instead of losing it. Other failures (too few observations) still throw.
    if (e.message().find("singular") == std::string::npos)
      throw;
    const Series nan = out.fit.beta.mul(std::numeric_limits<double>::quiet_NaN());
    out.intervals = {nan, nan, nan};
  }
  return out;
}
} // namespace detail

// Global fit of Vmax and Km to all curves with the closed-form integrated rate law (guess [lanes, 2] = (Vmax, Km))
[[nodiscard]] inline EnzymeFit fit_integrated(const ProgressData &d, const Series &guess, const est::Options &opt = {}) {
  detail::validate(d, guess, 2, "fit_integrated");
  const std::int64_t k = d.initial.size(0);
  const est::ModelFn model = [&](const Series &beta) { // beta [L, 2]
    const Series vmax = beta.select(1, 0).abs().unsqueeze(-1), km = beta.select(1, 1).abs().unsqueeze(-1); // [L, 1]
    std::vector<Series> rows;
    for (std::int64_t i = 0; i < k; ++i) { // curve i: [L, M]
      const Series t = d.times.unsqueeze(0).expand({beta.size(0), d.times.size(0)});
      rows.push_back(enzymes::integrated_michaelis_menten(t, d.initial.select(0, i), vmax, km).unsqueeze(1));
    }
    const Series all = rows.front().unite(std::vector<Series>(rows.begin() + 1, rows.end()), 1); // [L, K, M]
    return all.reshape({beta.size(0), k * d.times.size(0)});
  };
  return detail::finish(model, d, guess, opt);
}

// Global fit of Vmax, Km and the product-inhibition constant Kp (guess [lanes, 3]); `steps_per_interval` RK4 steps between consecutive observation times
[[nodiscard]] inline EnzymeFit fit_product_inhibition(const ProgressData &d, const Series &guess, std::int64_t steps_per_interval = 40, const est::Options &opt = {}) {
  detail::validate(d, guess, 3, "fit_product_inhibition");
  const std::int64_t k = d.initial.size(0), m = d.times.size(0);
  const auto times = d.times.contiguous().host<double>();
  const est::ModelFn model = [&](const Series &beta) {
    const std::int64_t lanes = beta.size(0);
    const Series vmax = beta.select(1, 0).abs().unsqueeze(-1), km = beta.select(1, 1).abs().unsqueeze(-1), kp = beta.select(1, 2).abs().unsqueeze(-1); // [L, 1]
    const Series s0 = d.initial.unsqueeze(0).expand({lanes, k});
    const auto rate = [&](const Series &s) { return s.mul(vmax).div(km.mul(s0.sub(s).div(kp).add(1.0)).add(s)).neg(); };
    Series s = s0;
    double t = 0.0;
    std::vector<Series> at_time;
    for (std::int64_t i = 0; i < m; ++i) {
      s = common::dynamics::simulate(rate, s, t, times[static_cast<std::size_t>(i)], steps_per_interval);
      t = times[static_cast<std::size_t>(i)];
      at_time.push_back(s);
    }
    return detail::flatten_predictions(at_time);
  };
  return detail::finish(model, d, guess, opt);
}

} // namespace pluss::subsp::natsci::biology::systems::enzyme_fitting
