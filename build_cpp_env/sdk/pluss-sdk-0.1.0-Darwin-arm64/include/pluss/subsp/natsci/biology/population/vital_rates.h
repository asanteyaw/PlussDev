#pragma once

// From individual data to a population projection: vital-rate regressions feeding an integral projection model (Easterling, Ellner & Dixon 2000; Ellner, Childs &
// Rees 2016 -- not obtained). Built entirely on existing layers: `stats::regression` (OLS and GLM with canonical links) for the fits and `population::ipm`
// (mesh / discretize / analyze) for the kernel; this header only wires them together.
//
//   survival     s(z)        logistic GLM  of survived ~ size  (+ size^2)
//   growth       G(z'|z)     OLS  of next size ~ size over survivors; Gaussian with the residual standard deviation
//   fecundity    f(z)        Poisson GLM (log link) of offspring count ~ size
//   recruits     D(z')       Gaussian with the sample mean and standard deviation of recruit sizes
//   kernel       K(z',z) = s(z) G(z'|z) + f(z) D(z')   on the midpoint mesh (P and F returned separately)

#include "pluss/subsp/natsci/biology/population/ipm.h"
#include "pluss/subsp/stats/regression.h"
#include "pluss/tils/aux/rng_seed.h"

#include <algorithm>
#include <cmath>

namespace pluss::subsp::natsci::biology::population::vital_rates {

namespace ipm = ::pluss::subsp::natsci::biology::population::ipm;
namespace reg = ::pluss::subsp::stats::regression;

struct Observations final {
  Series size = pluss::series::undefined();         // [n] size at census t
  Series next_size = pluss::series::undefined();    // [n] size at t + 1 (read only where survived == 1)
  Series survived = pluss::series::undefined();     // [n] 0 / 1
  Series offspring = pluss::series::undefined();    // [n] offspring produced (counts)
  Series recruit_size = pluss::series::undefined(); // [m] sizes of new recruits
};
struct Fit final {
  reg::GLMFitResult survival;
  reg::OLSFitResult growth;
  reg::GLMFitResult fecundity;
  double growth_sd{0.0};
  double recruit_mean{0.0};
  double recruit_sd{0.0};
  bool quadratic_survival{false};
};

namespace detail {
// design matrix [n, 1] or [n, 2] (size, size^2)
inline Series design(const Series &z, bool quadratic) { return quadratic ? z.unsqueeze(-1).unite(std::vector<Series>{z.square().unsqueeze(-1)}, 1) : z.unsqueeze(-1); }
} // namespace detail

[[nodiscard]] inline Fit fit(const Observations &data, bool quadratic_survival = false) {
  Fit out;
  out.quadratic_survival = quadratic_survival;
  out.survival = reg::glm_fit(data.survived, detail::design(data.size, quadratic_survival), {.family = reg::GLMFamily::Binomial, .link = reg::GLMLink::Canonical});
  const Series alive = data.survived.gt(0.5);
  out.growth = reg::ols_fit(data.next_size.masked_select(alive), data.size.masked_select(alive).unsqueeze(-1));
  out.growth_sd = std::sqrt(out.growth.sse / static_cast<double>(out.growth.df_resid)); // unbiased residual standard deviation
  out.fecundity = reg::glm_fit(data.offspring, detail::design(data.size, false), {.family = reg::GLMFamily::Poisson, .link = reg::GLMLink::Log});
  const double m = static_cast<double>(data.recruit_size.size(0));
  out.recruit_mean = data.recruit_size.mean().item<double>();
  out.recruit_sd = std::sqrt(data.recruit_size.sub(out.recruit_mean).square().sum().item<double>() / (m - 1.0));
  return out;
}

struct Kernel final {
  Series survival_growth = pluss::series::undefined(); // P  (h s(z_j) G(z_i | z_j)), [n, n]
  Series fecundity = pluss::series::undefined();       // F  (h f(z_j) D(z_i)), [n, n]
  [[nodiscard]] Series total() const { return survival_growth.add(fecundity); }
};
[[nodiscard]] inline Kernel kernel(const Fit &f, const ipm::Mesh &mesh) {
  const Series z = mesh.nodes;
  const Series s = reg::glm_predict(f.survival, detail::design(z, f.quadratic_survival));              // [n] survival probability
  const Series mean_next = reg::ols_predict(f.growth, z.unsqueeze(-1));                                 // [n]
  const Series fec = reg::glm_predict(f.fecundity, z.unsqueeze(-1));                                    // [n] expected offspring
  const Series zp = z.unsqueeze(-1), zc = z.unsqueeze(0);
  const Series growth = ipm::gaussian_density(zp, mean_next.unsqueeze(0), zp.mul(0.0).add(f.growth_sd)); // [n(z'), n(z)]
  const Series recruits = ipm::gaussian_density(z, z.mul(0.0).add(f.recruit_mean), z.mul(0.0).add(f.recruit_sd)); // D(z') [n]
  return {.survival_growth = growth.mul(s.unsqueeze(0)).mul(mesh.h),
          .fecundity = recruits.unsqueeze(-1).mul(fec.unsqueeze(0)).mul(mesh.h)};
}
[[nodiscard]] inline ipm::Analysis analyze(const Fit &f, const ipm::Mesh &mesh) { return ipm::analyze(kernel(f, mesh).total(), mesh.h); }

// ---- bootstrap -------------------------------------------------------------------------------------------------------------------------------
// Case-resampling bootstrap of lambda: individuals (and, separately, recruits) are resampled with replacement, every vital-rate regression is refitted and the
// kernel re-analysed. `bootstrap_lambda` takes the index matrices explicitly (rows = replicates) so a result can be reproduced exactly; `resample` draws them from the
// Series random stream.
struct Bootstrap final {
  Series lambda = pluss::series::undefined(); // [B]
  double mean{0.0}, standard_error{0.0};
  double lower{0.0}, upper{0.0}; // percentile interval
};
[[nodiscard]] inline Series resample(std::int64_t replicates, std::int64_t n, std::uint64_t seed = 0) {
  const Config cfg = Config{}.with_dtype(pluss::ScalarType::Float64);
  return pluss::series::rand({replicates, n}, cfg, pluss::tils::seed_or_next(seed)).mul(static_cast<double>(n)).floor().clamp(0.0, static_cast<double>(n - 1));
}
[[nodiscard]] inline Bootstrap bootstrap_lambda(const Observations &data, const ipm::Mesh &mesh, const Series &individual_rows, const Series &recruit_rows, bool quadratic_survival = false, double alpha = 0.05) {
  dynamics::require(individual_rows.dim() == 2 && recruit_rows.dim() == 2 && individual_rows.size(0) == recruit_rows.size(0), "bootstrap_lambda: index matrices must be [replicates, n] and [replicates, m]");
  const std::int64_t b = individual_rows.size(0);
  std::vector<double> out;
  for (std::int64_t r = 0; r < b; ++r) {
    const Series rows = individual_rows.select(0, r).to(pluss::ScalarType::Int64), rec = recruit_rows.select(0, r).to(pluss::ScalarType::Int64);
    const Observations sample{.size = data.size.index({rows}), .next_size = data.next_size.index({rows}), .survived = data.survived.index({rows}),
                              .offspring = data.offspring.index({rows}), .recruit_size = data.recruit_size.index({rec})};
    out.push_back(analyze(fit(sample, quadratic_survival), mesh).lambda.item<double>());
  }
  Bootstrap res;
  res.lambda = pluss::make_series(out, {b}).to(data.size.scalar_type());
  res.mean = res.lambda.mean().item<double>();
  res.standard_error = std::sqrt(res.lambda.sub(res.mean).square().sum().item<double>() / static_cast<double>(b - 1));
  auto sorted = out;
  std::ranges::sort(sorted);
  const auto at = [&](double q) { const double pos = q * static_cast<double>(b - 1); const auto lo = static_cast<std::size_t>(std::floor(pos)); const double frac = pos - std::floor(pos); return lo + 1 < sorted.size() ? sorted[lo] * (1.0 - frac) + sorted[lo + 1] * frac : sorted[lo]; };
  res.lower = at(alpha / 2.0);
  res.upper = at(1.0 - alpha / 2.0);
  return res;
}

} // namespace pluss::subsp::natsci::biology::population::vital_rates
