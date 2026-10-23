// Discrete population maps: derivatives, fixed points and multipliers, Lyapunov exponents, periods, cycles and the Feigenbaum cascade.
#include "pluss/subsp/natsci/biology/population/maps.h"
#include "maps_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mp = pluss::subsp::natsci::biology::population::maps;
namespace ref = maps_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
std::vector<double> vals(const Series &s) { return s.contiguous().to_vec<double>(); }
} // namespace

int main() {
  try {
    // analytic derivatives vs central differences on a grid, for every map
    {
      const Series x = vec({0.05, 0.2, 0.5, 0.9, 1.3, 2.0});
      const double h = 1e-6;
      const auto check = [&](const mp::Map &f, const mp::Map &df, const std::string &name) {
        const Series fd = f(x.add(h)).sub(f(x.sub(h))).div(2.0 * h);
        expect(fd.sub(df(x)).abs().max().item<double>() < 1e-6, name + ": derivative vs finite difference");
      };
      check([](const Series &s) { return mp::logistic(s, 3.7); }, [](const Series &s) { return mp::logistic_derivative(s, 3.7); }, "logistic");
      check([](const Series &s) { return mp::ricker(s, 2.3, 1.4); }, [](const Series &s) { return mp::ricker_derivative(s, 2.3, 1.4); }, "ricker");
      check([](const Series &s) { return mp::beverton_holt(s, 4.0, 1.7); }, [](const Series &s) { return mp::beverton_holt_derivative(s, 4.0, 1.7); }, "beverton_holt");
      check([](const Series &s) { return mp::hassell(s, 12.0, 0.5, 3.0); }, [](const Series &s) { return mp::hassell_derivative(s, 12.0, 0.5, 3.0); }, "hassell");
      check([](const Series &s) { return mp::maynard_smith_slatkin(s, 6.0, 0.8, 2.5); }, [](const Series &s) { return mp::maynard_smith_slatkin_derivative(s, 6.0, 0.8, 2.5); }, "maynard_smith_slatkin");
    }
    // fixed points and their multipliers: logistic x* = 1 - 1/r with f' = 2 - r; Ricker x* = K with f' = 1 - r; Beverton-Holt f'(K) = 1/R0
    {
      const Series r = vec({1.5, 2.5, 2.9});
      const mp::Map f = [&](const Series &x) { return mp::logistic(x, r); }, df = [&](const Series &x) { return mp::logistic_derivative(x, r); };
      const auto c = mp::periodic_point(f, df, vec({0.4, 0.55, 0.6}), 1);
      expect(c.converged, "logistic fixed point converged");
      expect(c.point.sub(r.reciprocal().neg().add(1.0)).abs().max().item<double>() < 1e-12 && c.multiplier.sub(r.neg().add(2.0)).abs().max().item<double>() < 1e-12, "logistic fixed point and multiplier 2 - r");
      const mp::Map g = [](const Series &x) { return mp::ricker(x, 1.8, 2.0); }, dg = [](const Series &x) { return mp::ricker_derivative(x, 1.8, 2.0); };
      const auto rc = mp::periodic_point(g, dg, vec({1.6}), 1);
      expect(std::abs(rc.point.item<double>() - 2.0) < 1e-12 && std::abs(rc.multiplier.item<double>() - (1.0 - 1.8)) < 1e-12, "Ricker fixed point K with multiplier 1 - r");
      const mp::Map b = [](const Series &x) { return mp::beverton_holt(x, 3.0, 5.0); }, db = [](const Series &x) { return mp::beverton_holt_derivative(x, 3.0, 5.0); };
      const auto bc = mp::periodic_point(b, db, vec({4.0}), 1);
      expect(std::abs(bc.point.item<double>() - 5.0) < 1e-12 && std::abs(bc.multiplier.item<double>() - 1.0 / 3.0) < 1e-12, "Beverton-Holt fixed point K with multiplier 1/R0");
    }
    // exact 2-cycle of the logistic map: x = [(r + 1) +- sqrt((r + 1)(r - 3))] / (2 r), Lyapunov exponent (1/2) ln|f'(x1) f'(x2)| = (1/2) ln|4 + 2 r - r^2|
    {
      const double r = 3.2;
      const mp::Map f = [&](const Series &x) { return mp::logistic(x, r); }, df = [&](const Series &x) { return mp::logistic_derivative(x, r); };
      const auto c = mp::periodic_point(f, df, vec({0.80}), 2);
      expect(c.converged, "2-cycle converged");
      const double x1 = ((r + 1.0) + std::sqrt((r + 1.0) * (r - 3.0))) / (2.0 * r), x2 = ((r + 1.0) - std::sqrt((r + 1.0) * (r - 3.0))) / (2.0 * r);
      const double xp = c.point.item<double>();
      expect((std::abs(xp - x1) < 1e-12 || std::abs(xp - x2) < 1e-12) && std::abs(c.multiplier.item<double>() - (4.0 + 2.0 * r - r * r)) < 1e-11, "2-cycle point and multiplier 4 + 2r - r^2");
      const double lam = mp::lyapunov(f, df, vec({0.3}), 1000, 4000).item<double>();
      expect(std::abs(lam - 0.5 * std::log(std::abs(4.0 + 2.0 * r - r * r))) < 1e-9, "Lyapunov exponent of the 2-cycle");
      const Series orbit = mp::attractor(f, vec({0.3}), 1000, 40);
      expect(std::abs(mp::detect_period(orbit, 16, 1e-9).item<double>() - 2.0) < 1e-12, "detected period 2");
    }
    // r = 4: exponent ln 2, chaotic (no period)
    {
      const mp::Map f = [](const Series &x) { return mp::logistic(x, 3.999999999); }, df = [](const Series &x) { return mp::logistic_derivative(x, 3.999999999); };
      const double lam = mp::lyapunov(f, df, vec({0.3123}), 500, 200000).item<double>();
      expect(std::abs(lam - std::log(2.0)) < 3e-3, "logistic r = 4 exponent ln 2: " + std::to_string(lam));
    }
    // sweeps against the numpy long-run exponents (all lanes at once)
    {
      const auto sweep = [&](const std::vector<double> &p, const std::vector<double> &want, const std::function<Series(const Series &, const Series &)> &f,
                             const std::function<Series(const Series &, const Series &)> &df, double x0, double tol, const std::string &name) {
        const Series pr = vec(p);
        const Series lam = mp::lyapunov([&](const Series &x) { return f(x, pr); }, [&](const Series &x) { return df(x, pr); }, pr.mul(0.0).add(x0), 2000, 200000);
        const auto lv = vals(lam);
        for (std::size_t i = 0; i < p.size(); ++i)
          expect(std::abs(lv[i] - want[i]) < tol, name + " exponent at parameter " + std::to_string(p[i]) + ": " + std::to_string(lv[i]) + " vs " + std::to_string(want[i]));
      };
      sweep(ref::logistic_r, ref::logistic_lyap, [](const Series &x, const Series &r) { return mp::logistic(x, r); }, [](const Series &x, const Series &r) { return mp::logistic_derivative(x, r); }, 0.3, 1e-2, "logistic");
      sweep(ref::ricker_r, ref::ricker_lyap, [](const Series &x, const Series &r) { return mp::ricker(x, r); }, [](const Series &x, const Series &r) { return mp::ricker_derivative(x, r); }, 0.4, 1e-2, "ricker");
      sweep(ref::hassell_lambda, ref::hassell_lyap, [](const Series &x, const Series &l) { return mp::hassell(x, l, 0.5, 3.0); }, [](const Series &x, const Series &l) { return mp::hassell_derivative(x, l, 0.5, 3.0); }, 0.5, 1e-2, "hassell");
    }
    // Feigenbaum cascade: superstable parameters R_n of the logistic map (mpmath, 50 digits) and the ratio to delta = 4.669201609...
    {
      const auto map = [](const Series &x, const Series &r) { return mp::logistic(x, r); };
      std::vector<double> got;
      double guess = 2.0;
      for (std::size_t n = 0; n < ref::superstable.size(); ++n) {
        if (n >= 2)
          guess = got[n - 1] + (got[n - 1] - got[n - 2]) / 4.669;
        else if (n == 1)
          guess = 3.2;
        const double r = mp::superstable_parameter(map, 0.5, 1 << n, vec({guess})).item<double>();
        got.push_back(r);
        expect(std::abs(r - ref::superstable[n]) < 1e-11, "superstable parameter R_" + std::to_string(n) + ": " + std::to_string(r));
      }
      const std::size_t n = got.size();
      const double delta = (got[n - 2] - got[n - 3]) / (got[n - 1] - got[n - 2]);
      expect(std::abs(delta - 4.669201609) < 2e-3, "Feigenbaum ratio " + std::to_string(delta));
      // the cycle at R_2 has multiplier 0
      const double r2 = got[2];
      const mp::Map f = [&](const Series &x) { return mp::logistic(x, r2); }, df = [&](const Series &x) { return mp::logistic_derivative(x, r2); };
      const auto c = mp::periodic_point(f, df, vec({0.5}), 4);
      expect(std::abs(c.multiplier.item<double>()) < 1e-6, "superstable cycle has multiplier 0");
    }
    std::cout << "maps tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
