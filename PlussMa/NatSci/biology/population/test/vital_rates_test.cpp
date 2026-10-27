// Vital-rate regressions -> integral projection model, against R's glm / lm / eigen on the same simulated individual data.
#include "pluss/subsp/natsci/biology/population/vital_rates.h"
#include "vital_rates_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace vr = pluss::subsp::natsci::biology::population::vital_rates;
namespace ip = pluss::subsp::natsci::biology::population::ipm;
namespace ref = vital_rates_reference;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
} // namespace

int main() {
  try {
    const vr::Observations data{.size = vec(ref::size), .next_size = vec(ref::next_size), .survived = vec(ref::survived), .offspring = vec(ref::offspring), .recruit_size = vec(ref::recruit_size)};
    const vr::Fit f = vr::fit(data);
    const auto sc = f.survival.coefficients.contiguous().host<double>(), gc = f.growth.coefficients.contiguous().host<double>(), fc = f.fecundity.coefficients.contiguous().host<double>();
    // the regressions reproduce R (the glm / lm results are the oracle; intercept first)
    for (std::size_t i = 0; i < 2; ++i) {
      expect(std::abs(sc[i] - ref::surv_coef[i]) < 1e-6, "survival coefficient " + std::to_string(i) + ": " + std::to_string(sc[i]) + " vs " + std::to_string(ref::surv_coef[i]));
      expect(std::abs(gc[i] - ref::growth_coef[i]) < 1e-9, "growth coefficient " + std::to_string(i));
      expect(std::abs(fc[i] - ref::fec_coef[i]) < 1e-6, "fecundity coefficient " + std::to_string(i));
    }
    const auto se = f.survival.std_errors.contiguous().host<double>();
    expect(std::abs(se[0] - ref::surv_se[0]) < 1e-5 && std::abs(se[1] - ref::surv_se[1]) < 1e-5, "survival standard errors");
    expect(std::abs(f.growth_sd - ref::growth_sd[0]) < 1e-9, "growth residual standard deviation");
    expect(std::abs(f.recruit_mean - ref::recruit_stats[0]) < 1e-12 && std::abs(f.recruit_sd - ref::recruit_stats[1]) < 1e-12, "recruit size mean / sd");
    // the fitted IPM: lambda and the survival curve on the mesh vs R
    const auto mesh = ip::mesh(0.0, 10.0, 120);
    const auto an = vr::analyze(f, mesh);
    expect(an.converged && std::abs(an.lambda.item<double>() - ref::lambda[0]) < 1e-6, "lambda of the fitted IPM: " + std::to_string(an.lambda.item<double>()) + " vs R " + std::to_string(ref::lambda[0]));
    const auto k = vr::kernel(f, mesh);
    expect(k.total().sub(k.survival_growth).sub(k.fecundity).abs().max().item<double>() < 1e-15, "P + F = K");
    // the fitted vital rates recover the generating model within sampling error (truth: -2 + 0.9 z, 0.6 + 0.85 z, sd 0.7, -2.2 + 0.35 z)
    expect(std::abs(sc[1] - 0.9) < 4.0 * se[1] && std::abs(gc[1] - 0.85) < 0.03 && std::abs(f.growth_sd - 0.7) < 0.04, "recovery of the generating parameters within sampling error");
    // quadratic survival adds a coefficient and keeps the pipeline running
    const vr::Fit q = vr::fit(data, true);
    expect(q.survival.coefficients.size(0) == 3 && vr::analyze(q, mesh).lambda.item<double>() > 0.0, "quadratic survival");
    // bootstrap: explicit index matrices from R reproduce R's replicate lambdas and percentile interval
    {
      const std::int64_t b = 100;
      const Series rows = pluss::make_series(ref::boot_rows, {b, 1800}).to(F64), rec = pluss::make_series(ref::boot_rec_rows, {b, 500}).to(F64);
      const auto bs = vr::bootstrap_lambda(data, mesh, rows, rec);
      const auto lv = bs.lambda.contiguous().host<double>();
      double worst = 0.0;
      for (std::size_t i = 0; i < lv.size(); ++i) worst = std::max(worst, std::abs(lv[i] - ref::boot_lambda[i]));
      expect(worst < 1e-5, "bootstrap replicate lambdas vs R, worst " + std::to_string(worst));
      expect(std::abs(bs.standard_error - ref::boot_se[0]) < 1e-5 && std::abs(bs.lower - ref::boot_ci[0]) < 1e-5 && std::abs(bs.upper - ref::boot_ci[1]) < 1e-5, "bootstrap standard error and percentile interval");
    }
    std::cout << "vital_rates tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
