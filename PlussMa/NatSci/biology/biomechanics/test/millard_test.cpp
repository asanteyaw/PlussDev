// Millard quintic-Bezier curves (math::bezier + millard.h) against an independent numpy/scipy construction, defining key points, smoothness and the muscle machinery.
#include "pluss/subsp/natsci/biology/biomechanics/millard.h"
#include "millard_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mi = pluss::subsp::natsci::biology::biomechanics::millard;
namespace mu = pluss::subsp::natsci::biology::biomechanics::muscle;
namespace bz = pluss::subsp::math::bezier;
namespace ref = millard_reference;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double worst(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch");
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i) w = std::max(w, std::abs(av[i] - b[i]));
  return w;
}
} // namespace

int main() {
  try {
    const bz::Curve active = mi::active_force_length(), fv = mi::force_velocity(), passive = mi::passive_force_length(), tendon = mi::tendon_force_length();
    // ---- value, slope and curvature on grids spanning the segments and both linear extensions vs the independent implementation
    const auto check = [&](const std::string &name, const bz::Curve &c, const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &d1, const std::vector<double> &d2) {
      const auto r = bz::evaluate(c, vec(x));
      expect(worst(r.value, y) < 1e-12 && worst(r.slope, d1) < 1e-10 && worst(r.curvature, d2) < 1e-8, name + ": value " + std::to_string(worst(r.value, y)) + ", slope " + std::to_string(worst(r.slope, d1)) + ", curvature " + std::to_string(worst(r.curvature, d2)));
    };
    check("active", active, ref::act_x, ref::act_y, ref::act_d1, ref::act_d2);
    check("force-velocity", fv, ref::fv_x, ref::fv_y, ref::fv_d1, ref::fv_d2);
    check("passive", passive, ref::pas_x, ref::pas_y, ref::pas_d1, ref::pas_d2);
    check("tendon", tendon, ref::ten_x, ref::ten_y, ref::ten_d1, ref::ten_d2);
    // ---- the curves' defining key points
    expect(std::abs(bz::value(active, vec({1.0})).host<double>().front() - 1.0) < 1e-13 && std::abs(bz::value(active, vec({0.47 - 0.0259})).host<double>().front() - 0.1) < 1e-13 && std::abs(bz::value(active, vec({1.8123})).host<double>().front() - 0.1) < 1e-13, "active: f(1) = 1, f = 0.1 at both ends");
    expect(std::abs(bz::value(fv, vec({0.0})).host<double>().front() - 1.0) < 1e-13 && std::abs(bz::value(fv, vec({-1.0})).host<double>().front()) < 1e-13 && std::abs(bz::value(fv, vec({1.0})).host<double>().front() - 1.4) < 1e-13, "force-velocity: (-1, 0), (0, 1), (1, 1.4)");
    expect(std::abs(bz::evaluate(fv, vec({0.0})).slope.host<double>().front() - 5.0) < 1e-9, "force-velocity isometric slope 5");
    expect(std::abs(bz::value(passive, vec({1.7})).host<double>().front() - 1.0) < 1e-13 && std::abs(bz::value(passive, vec({1.0})).host<double>().front()) < 1e-13, "passive: 0 at strain 0, 1 at strain 0.7");
    expect(std::abs(bz::value(tendon, vec({1.049})).host<double>().front() - 1.0) < 1e-12 && std::abs(bz::value(tendon, vec({1.0})).host<double>().front()) < 1e-13, "tendon: 0 at slack length, ~1 at strain 0.049");
    // ---- C^2 continuity at every segment join and finite-difference consistency of the derivatives
    for (const bz::Curve *c : {&active, &fv, &passive, &tendon}) {
      for (std::size_t k = 0; k + 1 < c->segments.size(); ++k) {
        const double join = c->segments[k].x.back(), e = 1e-9;
        const auto lo = bz::evaluate(*c, vec({join - e})), hi = bz::evaluate(*c, vec({join + e}));
        expect(std::abs(lo.value.host<double>().front() - hi.value.host<double>().front()) < 1e-7 && std::abs(lo.slope.host<double>().front() - hi.slope.host<double>().front()) < 1e-6 && std::abs(lo.curvature.host<double>().front() - hi.curvature.host<double>().front()) < 1e-3, "C^2 join of segment " + std::to_string(k));
      }
      const Series x = vec({c->x_first() + 0.31 * (c->x_last() - c->x_first()), c->x_first() + 0.77 * (c->x_last() - c->x_first())});
      const double h = 1e-7; // the slope is differenced (the curvature is checked against the independent analytic derivatives above; the curves have sharp corners)
      const auto r = bz::evaluate(*c, x), up = bz::evaluate(*c, x.add(h)), dn = bz::evaluate(*c, x.sub(h));
      expect(up.value.sub(dn.value).div(2.0 * h).sub(r.slope).abs().max().item<double>() < 1e-5 * (1.0 + r.slope.abs().max().item<double>()), "slope = d value / dx by central differences");
    }
    // ---- monotone curves invert exactly (force-velocity, tendon, passive)
    {
      const Series y = vec({0.05, 0.4, 1.0, 1.2, 1.38});
      expect(bz::value(fv, bz::inverse(fv, y)).sub(y).abs().max().item<double>() < 1e-12, "force-velocity inverse round trip");
      const Series yt = vec({0.0, 0.01, 0.3, 0.9});
      expect(bz::value(tendon, bz::inverse(tendon, yt)).sub(yt).abs().max().item<double>() < 1e-12, "tendon inverse round trip");
    }
    // ---- muscle-tendon equilibrium with the Millard family vs the independent brentq solution
    {
      mu::Params p;
      p.max_isometric_force = ref::muscle[0]; p.optimal_fiber_length = ref::muscle[1]; p.tendon_slack_length = ref::muscle[2]; p.pennation_at_optimal = ref::muscle[3];
      p.curves = mi::curves();
      const std::size_t n = ref::eq.size() / 4;
      std::vector<double> a, len;
      for (std::size_t i = 0; i < n; ++i) { a.push_back(ref::eq[4 * i]); len.push_back(ref::eq[4 * i + 1]); }
      const auto e = mu::isometric_equilibrium(vec(a), vec(len), p);
      const auto f = e.norm_tendon_force.contiguous().host<double>(), ln = e.norm_fiber_length.contiguous().host<double>();
      for (std::size_t i = 0; i < n; ++i)
        expect(std::abs(f[i] - ref::eq[4 * i + 2]) < 1e-9 && std::abs(ln[i] - ref::eq[4 * i + 3]) < 1e-9, "equilibrium " + std::to_string(i) + ": f_T " + std::to_string(f[i]) + " vs " + std::to_string(ref::eq[4 * i + 2]));
      // at equilibrium and rest the state rates vanish when activation and excitation agree
      const Series act = vec(a), ft = e.norm_tendon_force;
      const Series rate = mu::state_rate(act, ft, act, vec(len), act.mul(0.0), p);
      expect(rate.abs().max().item<double>() < 1e-8, "state rates vanish at the isometric equilibrium: " + std::to_string(rate.abs().max().item<double>()));
    }
    std::cout << "millard tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
