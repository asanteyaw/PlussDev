// Enzyme progress-curve fitting (integrated Michaelis-Menten via Lambert W, and competitive product inhibition via RK4) against scipy least_squares and nlparci-style intervals.
#include "pluss/subsp/natsci/biology/systems/enzyme_fitting.h"
#include "enzyme_fit_reference.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ef = pluss::subsp::natsci::biology::systems::enzyme_fitting;
namespace ref = enzyme_fit_reference;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> s) { return pluss::make_series(v, s).to(F64); }
} // namespace

int main() {
  try {
    const std::int64_t m = static_cast<std::int64_t>(ref::times.size()), k = static_cast<std::int64_t>(ref::s0.size());
    ef::ProgressData d{.times = mat(ref::times, {m}), .substrate = mat(ref::y1, {k, m}), .initial = mat(ref::s0, {k})};
    // ---- closed-form integrated rate law
    {
      const auto r = ef::fit_integrated(d, ef::initial_guess(d));
      expect(r.fit.converged.host<double>().front() == 1.0, "Levenberg-Marquardt converged");
      const auto b = r.fit.beta.contiguous().host<double>(), se = r.intervals.standard_error.contiguous().host<double>(), lo = r.intervals.lower.contiguous().host<double>(), hi = r.intervals.upper.contiguous().host<double>();
      for (std::size_t i = 0; i < 2; ++i) {
        expect(std::abs(std::abs(b[i]) - ref::fit1[i]) < 1e-6 * ref::fit1[i], "parameter " + std::to_string(i) + ": " + std::to_string(b[i]) + " vs scipy " + std::to_string(ref::fit1[i]));
        expect(std::abs(se[i] - ref::se1[i]) < 1e-4 * ref::se1[i], "standard error " + std::to_string(i));
        expect(std::abs(lo[i] - ref::lo1[i]) < 1e-5 && std::abs(hi[i] - ref::hi1[i]) < 1e-5, "95% interval " + std::to_string(i));
      }
      expect(std::abs(r.fit.sse.host<double>().front() - ref::sse1[0]) < 1e-9, "sum of squares at the optimum");
      expect(r.fit.observations == m * k && r.fit.parameters == 2, "bookkeeping");
      // the truth (0.9, 1.4) lies inside the intervals
      expect(lo[0] < 0.9 && 0.9 < hi[0] && lo[1] < 1.4 && 1.4 < hi[1], "the generating parameters are covered by the 95% intervals");
      // a whole batch of starting points in one call reaches the same optimum
      const Series starts = mat({1.5, 3.0, 0.5, 0.7, 0.8, 1.0}, {3, 2});
      const auto many = ef::fit_integrated(d, starts);
      expect(many.fit.beta.abs().sub(mat({ref::fit1[0], ref::fit1[1], ref::fit1[0], ref::fit1[1], ref::fit1[0], ref::fit1[1]}, {3, 2})).abs().max().item<double>() < 1e-5, "all starting points converge to the same fit");
    }
    // ---- malformed inputs are rejected with a message that names the problem
    {
      const auto rejects = [&](const std::function<void()> &call, const std::string &needle) {
        try {
          call();
        } catch (const pluss::Error &e) {
          return e.message().find(needle) != std::string::npos;
        }
        return false;
      };
      expect(rejects([&] { (void)ef::fit_product_inhibition(d, mat({1.0, 1.0}, {1, 2})); }, "starting guess must be"), "a two-column guess for the three-parameter model is rejected");
      expect(rejects([&] { (void)ef::fit_product_inhibition(d, mat({1.0, 1.0, 0.0}, {1, 3})); }, "non-zero and finite"), "a zero starting constant is rejected");
      ef::ProgressData bad = d;
      bad.initial = mat({1.0, 3.0, 5.0}, {3});
      expect(rejects([&] { (void)ef::fit_integrated(bad, mat({1.0, 1.0}, {1, 2})); }, "one row per starting concentration"), "mismatched curve count is rejected");
    }
    // ---- a runaway fit is not reported as converged: from a far-off guess the constants grow without bound along a flat direction of the sum of squares
    {
      ef::ProgressData d4{.times = d.times, .substrate = mat(ref::y2, {k, m}), .initial = d.initial};
      const auto far = ef::fit_product_inhibition(d4, mat({5.0, 0.2, 20.0}, {1, 3}));
      const auto good = ef::fit_product_inhibition(d4, mat({1.4, 2.5, 1.0}, {1, 3}));
      expect(far.fit.converged.host<double>().front() == 0.0, "a runaway fit is reported as not converged");
      expect(std::isnan(far.intervals.standard_error.host<double>().front()), "a runaway fit has undefined intervals");
      expect(good.fit.converged.host<double>().front() == 1.0, "the sound fit is still converged");
    }
    // ---- product inhibition by ODE integration
    {
      ef::ProgressData d2{.times = d.times, .substrate = mat(ref::y2, {k, m}), .initial = d.initial};
      const auto r = ef::fit_product_inhibition(d2, mat({1.4, 2.5, 1.0}, {1, 3}), 40);
      const auto b = r.fit.beta.contiguous().host<double>(), se = r.intervals.standard_error.contiguous().host<double>();
      for (std::size_t i = 0; i < 3; ++i) {
        expect(std::abs(std::abs(b[i]) - ref::fit2[i]) < 2e-4 * ref::fit2[i], "inhibition parameter " + std::to_string(i) + ": " + std::to_string(std::abs(b[i])) + " vs scipy " + std::to_string(ref::fit2[i]));
        expect(std::abs(se[i] - ref::se2[i]) < 2e-2 * ref::se2[i], "inhibition standard error " + std::to_string(i));
        // the estimate is reported as the positive physical value and sits inside its own interval, whichever sign the optimiser stopped at
        const auto lo2 = r.intervals.lower.contiguous().host<double>(), hi2 = r.intervals.upper.contiguous().host<double>();
        expect(b[i] > 0.0 && lo2[i] < b[i] && b[i] < hi2[i], "inhibition parameter " + std::to_string(i) + " is positive and inside its interval");
      }
      expect(std::abs(r.fit.sse.host<double>().front() - ref::sse2[0]) < 1e-6 * ref::sse2[0], "inhibition sum of squares");
      // with a very weak inhibitor (Kp -> large) the fitted rate law reduces to the plain integrated Michaelis-Menten fit of the same data
      ef::ProgressData d3{.times = d.times, .substrate = d.substrate, .initial = d.initial};
      const auto plain = ef::fit_product_inhibition(d3, mat({0.95, 1.5, 200.0}, {1, 3}), 40);
      expect(std::abs(std::abs(plain.fit.beta.select(1, 0).host<double>().front()) - ref::fit1[0]) < 5e-2 && plain.fit.sse.host<double>().front() < 1.3 * ref::sse1[0] + 1e-4, "no inhibition in the data -> Kp is large and the fit matches the uninhibited one");
    }
    std::cout << "enzyme_fitting tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
