// Integral projection models and integrodifference spreading, against closed forms, numpy and a direct front simulation.
#include "pluss/subsp/natsci/biology/population/ipm.h"
#include "ipm_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ip = pluss::subsp::natsci::biology::population::ipm;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double rel_diff(const Series &a, const std::vector<double> &b, double floor = 1e-12) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch");
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]) / (floor + std::abs(b[i])));
  return w;
}
} // namespace

int main() {
  try {
    // ---- Gaussian-growth kernel with constant survival c: lambda = c, w = stationary AR(1) density N(mu / (1 - b), s^2 / (1 - b^2)), v = 1
    {
      const double c = 0.9, mu = 0.5, b = 0.6, s = 0.7;
      for (std::int64_t n : {100, 200}) {
        const auto m = ip::mesh(-6.0, 9.0, n);
        const Series a = ip::discretize([&](const Series &zp, const Series &z) { return ip::gaussian_density(zp, z.mul(b).add(mu), zp.mul(0.0).add(s)).mul(c); }, m);
        const auto an = ip::analyze(a, m.h);
        expect(an.converged, "IPM eigen converged");
        expect(std::abs(an.lambda.item<double>() - c) < 1e-9, "lambda = c for n = " + std::to_string(n) + ": " + std::to_string(an.lambda.item<double>()));
        const double mw = mu / (1.0 - b), sw = s / std::sqrt(1.0 - b * b);
        const Series exact = ip::gaussian_density(m.nodes, m.nodes.mul(0.0).add(mw), m.nodes.mul(0.0).add(sw));
        expect(an.w.sub(exact).abs().max().item<double>() < 1e-6, "stable density is the stationary AR(1) normal law");
        const Series core = exact.gt(1e-3);  // away from the truncated tails, where the kernel loses mass through the domain edge
        expect(core.where(an.v.sub(1.0).abs(), an.v.mul(0.0)).max().item<double>() < 1e-6, "reproductive value is constant 1 where the population lives: " + std::to_string(core.where(an.v.sub(1.0).abs(), an.v.mul(0.0)).max().item<double>()));
        expect(std::abs(ip::mean_size(an.w, m).item<double>() - mw) < 1e-6 && std::abs(ip::variance_size(an.w, m).item<double>() - sw * sw) < 1e-6, "mean and variance of the stable size distribution");
      }
    }
    // ---- a realistic survival/growth/fecundity IPM vs numpy
    {
      const std::int64_t n = 200;
      const auto m = ip::mesh(0.0, 10.0, n);
      const auto surv = [](const Series &z) { return z.mul(0.8).sub(1.0).neg().exp().add(1.0).reciprocal(); };
      const Series p = ip::discretize([&](const Series &zp, const Series &z) { return ip::gaussian_density(zp, z.mul(0.85).add(0.5), zp.mul(0.0).add(0.8)).mul(surv(z)); }, m);
      const Series f = ip::discretize([&](const Series &zp, const Series &z) { return ip::gaussian_density(zp, zp.mul(0.0).add(1.0), zp.mul(0.0).add(0.6)).mul(z.mul(0.15)).mul(surv(z)); }, m);
      const Series a = p.add(f);
      const auto an = ip::analyze(a, m.h);
      expect(std::abs(an.lambda.item<double>() - ipm_reference::lam[0]) < 1e-10, "lambda vs numpy eig");
      expect(rel_diff(an.w, ipm_reference::w, 1e-9) < 1e-6 && rel_diff(an.v, ipm_reference::v, 1e-9) < 1e-6, "w and v vs numpy eig");
      const auto sd = an.sensitivity.contiguous().host<double>();
      double worst = 0.0;
      for (std::int64_t i = 0; i < n; ++i)
        worst = std::max(worst, std::abs(sd[static_cast<std::size_t>(i * n + i)] - ipm_reference::sens_diag[static_cast<std::size_t>(i)]) / (1e-9 + ipm_reference::sens_diag[static_cast<std::size_t>(i)]));
      expect(worst < 1e-6, "sensitivity kernel diagonal vs numpy");
      expect(std::abs(an.elasticity.sum().item<double>() * m.h * m.h - 1.0) < 1e-9 && std::abs(ipm_reference::elas_sum[0] - 1.0) < 1e-9, "elasticity kernel integrates to 1");
      expect(rel_diff(ip::life_expectancy(p), ipm_reference::eta) < 1e-9, "life expectancy vs numpy inverse");
      expect(std::abs(ip::net_reproductive_rate(p, f).item<double>() - ipm_reference::R0[0]) < 1e-10, "R0 vs numpy");
      // first-order perturbation: d lambda = h^2 sum s dK for a smooth bump added to the kernel
      const Series field = ip::discretize([&](const Series &zp, const Series &z) { return zp.sub(3.0).square().add(z.sub(2.0).square()).neg().exp(); }, m).div(m.h);
      const Series da = a.mul(field); // multiplicative perturbation keeps the matrix non-negative
      const double eps = 1e-5;
      const double exact = (ip::analyze(a.add(da.mul(eps)), m.h).lambda.item<double>() - ip::analyze(a.sub(da.mul(eps)), m.h).lambda.item<double>()) / (2.0 * eps);
      const double predicted = an.sensitivity.mul(da).sum().item<double>() * m.h; // d lambda = h sum s_ij dA_ij  (= h^2 sum s dK)
      expect(std::abs(exact - predicted) < 1e-6 * std::abs(predicted) + 1e-12, "sensitivity kernel predicts the change in lambda: " + std::to_string(exact) + " vs " + std::to_string(predicted));
    }
    // ---- spreading speed: Gaussian kernel c* = sigma sqrt(2 ln R0); Laplace kernel vs scipy minimisation
    {
      const Series lnr = vec({0.3, 0.7, 1.4});
      const double sigma = 1.3;
      const Series c = ip::spreading_speed(lnr, [&](const Series &s) { return s.square().mul(sigma * sigma / 2.0).exp(); }, 20.0);
      expect(c.sub(lnr.mul(2.0).sqrt().mul(sigma)).abs().max().item<double>() < 1e-9, "Gaussian spreading speed sigma sqrt(2 ln R0)");
      const double a = 0.8;
      const Series cl = ip::spreading_speed(lnr, [&](const Series &s) { return s.square().mul(a * a).neg().add(1.0).reciprocal(); }, 1.0 / a);
      expect(rel_diff(cl, ipm_reference::laplace_speed) < 1e-7, "Laplace spreading speed vs scipy: " + std::to_string(rel_diff(cl, ipm_reference::laplace_speed)));
    }
    // ---- simulate the integrodifference equation (Gaussian dispersal, Beverton-Holt growth) and compare the front speed with c* and the t^-1 correction
    {
      const double sigma = 1.0, r0 = 2.0, kcap = 1.0;
      const std::int64_t n = 1500;
      const auto m = ip::mesh(0.0, 300.0, n);
      const Series disp = ip::discretize([&](const Series &zp, const Series &z) { return ip::gaussian_density(zp, z, zp.mul(0.0).add(sigma)); }, m); // column j: kernel centred at z_j
      std::vector<double> init(static_cast<std::size_t>(n), 0.0);
      for (std::int64_t i = 0; i < n; ++i)
        init[static_cast<std::size_t>(i)] = (static_cast<double>(i) + 0.5) * m.h < 20.0 ? kcap : 0.0;
      Series dens = vec(init);
      const auto grow = [&](const Series &x) { return x.mul(r0).div(x.mul((r0 - 1.0) / kcap).add(1.0)); };
      const auto front = [&](const Series &x) {
        const auto v = x.contiguous().host<double>();
        for (std::size_t i = v.size() - 1; i > 0; --i)
          if (v[i - 1] >= 0.5 * kcap && v[i] < 0.5 * kcap)
            return (static_cast<double>(i) - 0.5 + (v[i - 1] - 0.5 * kcap) / (v[i - 1] - v[i])) * m.h;
        return -1.0;
      };
      double x100 = 0.0;
      for (int t = 1; t <= 200; ++t) {
        dens = disp.matmul(grow(dens).unsqueeze(-1)).squeeze(-1);
        if (t == 100)
          x100 = front(dens);
      }
      const double speed = (front(dens) - x100) / 100.0;
      const double cstar = sigma * std::sqrt(2.0 * std::log(r0)), sstar = std::sqrt(2.0 * std::log(r0)) / sigma;
      const double expected = cstar - 1.5 / sstar * std::log(200.0 / 100.0) / 100.0; // pulled-front logarithmic correction, c* - (3 / 2 s*) ln(t2 / t1) / (t2 - t1)
      expect(std::abs(speed - expected) < 0.01 * expected, "simulated IDE front speed " + std::to_string(speed) + " vs c* with the pulled-front correction " + std::to_string(expected) + " (c* = " + std::to_string(cstar) + ")");
    }
    std::cout << "ipm tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
