// Delayed population models: complex Lambert W roots, Hopf onset, and the method-of-steps integrator against an independent DOP853 solution.
#include "pluss/subsp/natsci/biology/population/delay.h"
#include "delay_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace dl = pluss::subsp::natsci::biology::population::delay;
namespace ref = delay_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
} // namespace

int main() {
  try {
    // ---- rightmost characteristic root vs mpmath's complex Lambert W, and the characteristic equation itself
    {
      const std::size_t cases = ref::roots.size() / 5;
      std::vector<double> a, b, tau;
      for (std::size_t i = 0; i < cases; ++i) {
        a.push_back(ref::roots[5 * i]);
        b.push_back(ref::roots[5 * i + 1]);
        tau.push_back(ref::roots[5 * i + 2]);
      }
      const auto root = dl::rightmost_root(vec(a), vec(b), vec(tau));
      const auto re = root.re.contiguous().to_vec<double>(), im = root.im.contiguous().to_vec<double>();
      for (std::size_t i = 0; i < cases; ++i) {
        expect(std::abs(re[i] - ref::roots[5 * i + 3]) < 1e-10 && std::abs(im[i] - ref::roots[5 * i + 4]) < 1e-10,
               "root " + std::to_string(i) + ": " + std::to_string(re[i]) + " + " + std::to_string(im[i]) + "i vs " + std::to_string(ref::roots[5 * i + 3]) + " + " + std::to_string(ref::roots[5 * i + 4]) + "i");
        // lambda + a + b e^{-lambda tau} = 0 (complex)
        const double e = std::exp(-re[i] * tau[i]);
        const double rr = re[i] + a[i] + b[i] * e * std::cos(im[i] * tau[i]), ri = im[i] - b[i] * e * std::sin(im[i] * tau[i]);
        expect(std::abs(rr) < 1e-10 && std::abs(ri) < 1e-10, "the root satisfies the characteristic equation");
      }
    }
    // ---- Hopf onset of Hutchinson's equation: Re lambda crosses 0 at r tau = pi / 2 with Im lambda = r
    {
      const double r = 1.3, tc = dl::hutchinson_critical_delay(r);
      const auto at = [&](double tau) { return dl::rightmost_root(vec({0.0}), vec({r}), vec({tau})); };
      const auto crit = at(tc);
      expect(std::abs(crit.re.item<double>()) < 1e-10 && std::abs(crit.im.item<double>() - r) < 1e-10, "Hopf: Re lambda = 0 and Im lambda = r at tau = pi / (2 r)");
      expect(at(0.95 * tc).re.item<double>() < 0.0 && at(1.05 * tc).re.item<double>() > 0.0, "stable below, unstable above the critical delay");
    }
    // ---- linear regime: measured envelope growth rate and frequency of x' = -b x(t - tau) from successive peaks
    for (const double tau : {1.2, 1.9}) {
      const double r = 1.0;
      const auto f = [&](const Series &x, const Series &xd) { return xd.mul(-r); };
      const auto hist = [&](double) { return vec({1e-8}); };
      // history: a small constant perturbation of the equilibrium of the linearised equation
      const auto path = dl::integrate(f, [&](double t) { return vec({t <= -0.5 * tau ? 0.0 : 1e-8 * std::cos(3.0 * t)}); }, tau, 120.0, 400);
      (void)hist;
      std::vector<double> peak_t, peak_v;
      for (std::size_t k = 1; k + 1 < path.states.size(); ++k) {
        const double x0 = path.states[k - 1].item<double>(), x1 = path.states[k].item<double>(), x2 = path.states[k + 1].item<double>();
        if (x1 > x0 && x1 > x2 && path.times[k] > 40.0) {
          // parabolic peak refinement
          const double d = 0.5 * (x0 - x2) / (x0 - 2.0 * x1 + x2);
          peak_t.push_back(path.times[k] + d * (path.times[1] - path.times[0]));
          peak_v.push_back(x1 - 0.25 * (x0 - x2) * d);
        }
      }
      expect(peak_t.size() >= 4, "enough oscillation peaks for tau = " + std::to_string(tau));
      const std::size_t n = peak_t.size();
      const double period = (peak_t[n - 1] - peak_t[1]) / static_cast<double>(n - 2);
      const double alpha = std::log(std::abs(peak_v[n - 1] / peak_v[1])) / (peak_t[n - 1] - peak_t[1]);
      const auto root = dl::rightmost_root(vec({0.0}), vec({r}), vec({tau}));
      expect(std::abs(alpha - root.re.item<double>()) < 2e-4, "tau = " + std::to_string(tau) + ": envelope rate " + std::to_string(alpha) + " vs Re lambda " + std::to_string(root.re.item<double>()));
      expect(std::abs(2.0 * M_PI / period - root.im.item<double>()) < 5e-4, "tau = " + std::to_string(tau) + ": frequency " + std::to_string(2.0 * M_PI / period) + " vs Im lambda " + std::to_string(root.im.item<double>()));
    }
    // ---- nonlinear Hutchinson and Nicholson trajectories vs the independent DOP853 method-of-steps solution
    {
      const double r = 1.3, k = 2.0, tau = 1.9;
      const auto hut = dl::integrate([&](const Series &n, const Series &nd) { return dl::hutchinson_rate(n, nd, r, k); },
                                     [](double t) { return vec({0.5 + 0.1 * std::max(t, -1.0)}); }, tau, 20.0, 380); // dt = 0.005 so every sample time is a node
      const double db = 0.2, p = 8.0, n0 = 1.0, tb = 2.0;
      const auto nic = dl::integrate([&](const Series &n, const Series &nd) { return dl::nicholson_rate(n, nd, db, p, n0); },
                                     [](double t) { return vec({1.5 + 0.2 * std::sin(t)}); }, tb, 20.0, 400);
      const std::vector<double> ts{1.0, 2.5, 5.0, 8.0, 12.0, 20.0};
      for (std::size_t i = 0; i < ts.size(); ++i) {
        const auto at = [&](const dl::Path &path, double t) { // t falls on a node when dt divides it; otherwise the nearest node
          const double dt = path.times[1] - path.times[0];
          const auto idx = static_cast<std::size_t>(std::llround(t / dt));
          return path.states[idx].item<double>();
        };
        expect(std::abs(at(hut, ts[i]) - ref::hut[i]) < 2e-6 * (1.0 + ref::hut[i]), "Hutchinson at t = " + std::to_string(ts[i]) + ": " + std::to_string(at(hut, ts[i])) + " vs " + std::to_string(ref::hut[i]));
        expect(std::abs(at(nic, ts[i]) - ref::nic[i]) < 2e-6 * (1.0 + ref::nic[i]), "Nicholson at t = " + std::to_string(ts[i]) + ": " + std::to_string(at(nic, ts[i])) + " vs " + std::to_string(ref::nic[i]));
      }
      expect(std::abs(dl::nicholson_equilibrium(db, p, n0) - std::log(p / db)) < 1e-14, "Nicholson equilibrium N0 ln(P / d)");
    }
    // ---- lanes: two Hutchinson populations with different r integrate in one call
    {
      const auto lanes = dl::integrate([&](const Series &n, const Series &nd) { return dl::hutchinson_rate(n, nd, vec({0.8, 1.3}), 2.0); },
                                       [](double) { return vec({0.6, 0.6}); }, 1.9, 10.0, 200);
      const auto single = dl::integrate([&](const Series &n, const Series &nd) { return dl::hutchinson_rate(n, nd, 1.3, 2.0); }, [](double) { return vec({0.6}); }, 1.9, 10.0, 200);
      expect(std::abs(lanes.states.back().select(0, 1).item<double>() - single.states.back().item<double>()) < 1e-13, "lane-batched integration equals the single run");
    }
    std::cout << "delay tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
