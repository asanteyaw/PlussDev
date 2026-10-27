// Levenberg-Marquardt, parameter intervals and profile likelihood against MATLAB's nlinfit / nlparci (R2025b) and exact linear-model results.
#include "estimation_reference.h"
#include "pluss/subsp/natsci/common/estimation.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace est = pluss::subsp::natsci::common::estimation;
namespace ref = estimation_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
Series mat(const std::vector<double> &v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(F64); }
double rel(double a, double b) { return std::abs(a - b) / (1e-12 + std::abs(b)); }
} // namespace

int main() {
  try {
    // critical values vs MATLAB tinv / chi2inv
    expect(rel(est::student_t_quantile(0.975, 10.0), ref::tinv_975_10[0]) < 1e-9, "tinv(0.975, 10)");
    expect(rel(est::student_t_quantile(0.95, 19.0), ref::tinv_95_19[0]) < 1e-9, "tinv(0.95, 19)");
    expect(rel(est::chi2_quantile(0.95, 1), ref::chi2inv_95_1[0]) < 1e-9, "chi2inv(0.95, 1)");
    expect(rel(est::chi2_quantile(0.95, 3), ref::chi2inv_95_3[0]) < 1e-9, "chi2inv(0.95, 3)");

    // ---- P1 Michaelis-Menten, P2 offset exponential decay, P3 Hill: beta, SSE and nlparci intervals
    const Series x1 = vec(ref::p1_x);
    const auto mm = [&](const Series &b) { return b.select(-1, 0).unsqueeze(-1).mul(x1).div(b.select(-1, 1).unsqueeze(-1).add(x1)); };
    {
      const auto fit = est::levenberg_marquardt(mm, vec(ref::p1_y), mat(ref::p1_b0, 1, 2));
      const auto bv = fit.beta.host<double>();
      for (int i = 0; i < 2; ++i)
        expect(rel(bv[static_cast<std::size_t>(i)], ref::p1_beta[static_cast<std::size_t>(i)]) < 1e-6, "P1 beta vs nlinfit, parameter " + std::to_string(i) + ": " + std::to_string(bv[static_cast<std::size_t>(i)]) + " vs " + std::to_string(ref::p1_beta[static_cast<std::size_t>(i)]));
      expect(rel(fit.sse.host<double>().front(), ref::p1_sse[0]) < 1e-7, "P1 SSE");
      const auto ci = est::confidence_intervals(fit);
      const auto lo = ci.lower.host<double>(), hi = ci.upper.host<double>();
      for (int i = 0; i < 2; ++i)
        expect(rel(lo[static_cast<std::size_t>(i)], ref::p1_ci[static_cast<std::size_t>(2 * i)]) < 1e-5 && rel(hi[static_cast<std::size_t>(i)], ref::p1_ci[static_cast<std::size_t>(2 * i + 1)]) < 1e-5, "P1 nlparci interval " + std::to_string(i));
    }
    {
      const Series t = vec(ref::p2_x);
      const auto m2 = [&](const Series &b) { return b.select(-1, 0).unsqueeze(-1).mul(t.unsqueeze(0).mul(b.select(-1, 1).unsqueeze(-1)).mul(-1.0).exp()).add(b.select(-1, 2).unsqueeze(-1)); };
      const auto fit = est::levenberg_marquardt(m2, vec(ref::p2_y), mat(ref::p2_b0, 1, 3));
      const auto bv = fit.beta.host<double>();
      for (int i = 0; i < 3; ++i)
        expect(rel(bv[static_cast<std::size_t>(i)], ref::p2_beta[static_cast<std::size_t>(i)]) < 1e-6, "P2 beta vs nlinfit " + std::to_string(i));
      const auto ci = est::confidence_intervals(fit);
      const auto lo = ci.lower.host<double>(), hi = ci.upper.host<double>();
      for (int i = 0; i < 3; ++i)
        expect(rel(lo[static_cast<std::size_t>(i)], ref::p2_ci[static_cast<std::size_t>(2 * i)]) < 1e-5 && rel(hi[static_cast<std::size_t>(i)], ref::p2_ci[static_cast<std::size_t>(2 * i + 1)]) < 1e-5, "P2 nlparci interval " + std::to_string(i));
    }
    {
      const Series xh = vec(ref::p3_x);
      const auto m3 = [&](const Series &b) {
        const Series v = b.select(-1, 0).unsqueeze(-1), k = b.select(-1, 1).unsqueeze(-1), n = b.select(-1, 2).unsqueeze(-1);
        const Series xn = xh.unsqueeze(0).pow(n);
        return v.mul(xn).div(k.pow(n).add(xn));
      };
      const auto fit = est::levenberg_marquardt(m3, vec(ref::p3_y), mat(ref::p3_b0, 1, 3));
      const auto bv = fit.beta.host<double>();
      for (int i = 0; i < 3; ++i)
        expect(rel(bv[static_cast<std::size_t>(i)], ref::p3_beta[static_cast<std::size_t>(i)]) < 1e-6, "P3 beta vs nlinfit " + std::to_string(i));
      const auto ci = est::confidence_intervals(fit, 0.1);
      const auto lo = ci.lower.host<double>(), hi = ci.upper.host<double>();
      for (int i = 0; i < 3; ++i)
        expect(rel(lo[static_cast<std::size_t>(i)], ref::p3_ci90[static_cast<std::size_t>(2 * i)]) < 1e-5 && rel(hi[static_cast<std::size_t>(i)], ref::p3_ci90[static_cast<std::size_t>(2 * i + 1)]) < 1e-5, "P3 nlparci 90% interval " + std::to_string(i));
    }
    // ---- lanes: several starting points of the same fit in one call. MATLAB's nlinfit reaches the optimum from three of the four starts and
    // gets stuck at (1.737056908, -0.2163744913), SSE 11.74631985 from (0.5, 3): the batched solver follows the same trajectories.
    {
      const Series starts = mat({1.0, 1.0, 5.0, 0.2, 0.5, 3.0, 8.0, 8.0}, 4, 2);
      const auto fit = est::levenberg_marquardt(mm, vec(ref::p1_y), starts);
      const auto bv = fit.beta.host<double>();
      for (const int lane : {0, 1, 3})
        expect(rel(bv[static_cast<std::size_t>(2 * lane)], ref::p1_beta[0]) < 1e-5 && rel(bv[static_cast<std::size_t>(2 * lane + 1)], ref::p1_beta[1]) < 1e-5, "multi-start lane " + std::to_string(lane) + " reaches the optimum");
      expect(rel(bv[4], 1.737056908) < 1e-5 && rel(bv[5], -0.2163744913) < 1e-5, "the stuck lane ends where MATLAB's nlinfit ends: " + std::to_string(bv[4]) + ", " + std::to_string(bv[5]));
      expect(rel(fit.sse.select(0, 2).item<double>(), 11.74631985) < 1e-6, "SSE of the stuck lane equals MATLAB's");
    }
    // ---- profile likelihood on a linear model: exactly parabolic, interval = beta +- sqrt(chi2inv) sigma sqrt(cov_ii)
    {
      const std::vector<double> xs{0, 1, 2, 3, 4, 5, 6, 7}, ys{1.1, 2.9, 5.2, 6.8, 9.1, 11.2, 12.8, 15.1};
      const Series x = vec(xs);
      const auto lin = [&](const Series &b) { return b.select(-1, 0).unsqueeze(-1).mul(x).add(b.select(-1, 1).unsqueeze(-1)); };
      const auto fit = est::levenberg_marquardt(lin, vec(ys), mat({1.0, 0.0}, 1, 2));
      const double b0 = fit.beta.select(-1, 0).host<double>().front(), b1 = fit.beta.select(-1, 1).host<double>().front(), sse = fit.sse.host<double>().front();
      // cov = sigma^2 (X^T X)^{-1}; for y = b0 x + b1:  Var(b0) = sigma^2 / Sxx
      double sxx = 0, mean = 0;
      for (double v : xs) mean += v / static_cast<double>(xs.size());
      for (double v : xs) sxx += (v - mean) * (v - mean);
      const double sigma2 = sse / static_cast<double>(xs.size() - 2);
      std::vector<double> grid;
      for (int i = -20; i <= 20; ++i)
        grid.push_back(b0 + 0.05 * i);
      const auto prof = est::profile_likelihood(lin, vec(ys), fit.beta.select(0, 0), 0, vec(grid));
      const auto sv = prof.sse.host<double>();
      for (std::size_t i = 0; i < grid.size(); ++i)
        expect(rel(sv[i], sse + (grid[i] - b0) * (grid[i] - b0) * sxx) < 1e-6, "linear-model profile is the exact parabola, point " + std::to_string(i));
      const auto interval = est::profile_interval(prof, sse, sigma2, 0.05, 1);
      const double half = std::sqrt(est::chi2_quantile(0.95, 1)) * std::sqrt(sigma2 / sxx);
      expect(std::abs(interval.first - (b0 - half)) < 2e-3 && std::abs(interval.second - (b0 + half)) < 2e-3, "profile interval equals the Wald interval for a linear model");
      (void)b1;
    }
    // ---- profile likelihood detects non-identifiability: y = (a + b) x has a completely flat profile along a - b
    {
      const std::vector<double> xs{1, 2, 3, 4, 5}, ys{2.1, 3.9, 6.2, 7.8, 10.1};
      const Series x = vec(xs);
      const auto degenerate = [&](const Series &b) { return b.select(-1, 0).unsqueeze(-1).add(b.select(-1, 1).unsqueeze(-1)).mul(x); };
      const auto prof = est::profile_likelihood(degenerate, vec(ys), vec({1.0, 1.0}), 0, vec({-2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0}));
      const auto sv = prof.sse.host<double>();
      for (std::size_t i = 1; i < sv.size(); ++i)
        expect(std::abs(sv[i] - sv[0]) < 1e-6 * (1.0 + sv[0]), "flat profile for a structurally non-identifiable parameter");
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "estimation tests passed\n";
  return 0;
}
