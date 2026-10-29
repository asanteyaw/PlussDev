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
inline Series flatten_data(const Series &substrate) { return substrate.reshape({substrate.size(0) * substrate.size(1)}); }
[[nodiscard]] inline EnzymeFit finish(const est::ModelFn &model, const ProgressData &d, const Series &guess, const est::Options &opt) {
  EnzymeFit out;
  out.fit = est::levenberg_marquardt(model, flatten_data(d.substrate), guess, opt);
  out.intervals = est::confidence_intervals(out.fit);
  return out;
}
} // namespace detail

// Global fit of Vmax and Km to all curves with the closed-form integrated rate law (guess [lanes, 2] = (Vmax, Km))
[[nodiscard]] inline EnzymeFit fit_integrated(const ProgressData &d, const Series &guess, const est::Options &opt = {}) {
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
