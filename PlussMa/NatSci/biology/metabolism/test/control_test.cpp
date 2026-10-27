// Metabolic control analysis against COPASI's link-matrix formula and finite differences of re-solved steady states.
#include "pluss/subsp/natsci/biology/metabolism/control.h"
#include "control_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mc = pluss::subsp::natsci::biology::metabolism::control;
namespace ref = control_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series cols(const std::vector<Series> &c) { // stack along a new last axis
  std::vector<Series> rest;
  for (std::size_t i = 1; i < c.size(); ++i)
    rest.push_back(c[i].unsqueeze(-1));
  return c[0].unsqueeze(-1).unite(rest, c[0].dim());
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
double max_diff(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch");
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]) / (1.0 + std::abs(b[i])));
  return w;
}
} // namespace

int main() {
  try {
    const std::int64_t lanes = 4;
    const Series n = mat({1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, -1, 0, 1, -1, 0, 1, 0, -1, 1}, {4, 5});
    const Series k1 = mat(ref::k1_lanes, {lanes});
    const auto &bp = ref::base_params;
    // rate law v(x, k1): x [..., 4], k1 broadcasts over lanes
    const auto rate = [&](const Series &x, const Series &p1) {
      const Series s1 = x.select(-1, 0), s2 = x.select(-1, 1), atp = x.select(-1, 2), adp = x.select(-1, 3);
      const Series v2 = s1.mul(atp).mul(bp[1]).div(s1.add(bp[2]).mul(atp.add(bp[3])));
      const Series v3 = s2.mul(bp[4]).div(s2.add(bp[5]));
      const Series v4 = adp.mul(bp[6]).div(adp.add(bp[7]));
      const Series v5 = atp.mul(bp[8]);
      return cols({p1.add(v2.mul(0.0)), v2, v3, v4, v5});
    };
    // conservation laws found from N alone
    const Series gamma = mc::conservation_laws(n);
    expect(gamma.size(0) == 1 && gamma.size(1) == 4, "one conservation law (ATP + ADP)");
    const auto g = gamma.contiguous().host<double>();
    expect(std::abs(g[0]) < 1e-12 && std::abs(g[1]) < 1e-12 && std::abs(std::abs(g[2]) - std::sqrt(0.5)) < 1e-12 && std::abs(g[2] - g[3]) < 1e-12, "Gamma is (0,0,1,1)/sqrt2");
    expect(gamma.matmul(n).abs().max().item<double>() < 1e-12, "Gamma N = 0");
    // steady state with the total T = 5 closed by Gamma x = T/sqrt2... use the unnormalised law instead for the reference total
    const Series gam = mat({0, 0, 1, 1}, {1, 4});
    const auto v_of = [&](const Series &x) { return rate(x, k1); };
    const auto ss = mc::steady_state(v_of, n, mat(std::vector<double>(16, 1.5), {lanes, 4}), gam, mat({5.0, 5.0, 5.0, 5.0}, {lanes, 1}).squeeze(-1).unsqueeze(-1));
    expect(ss.converged, "steady state converged");
    expect(max_diff(ss.state, ref::x) < 1e-9, "steady state vs fsolve: " + std::to_string(max_diff(ss.state, ref::x)));
    const Series x = ss.state, v = v_of(x);
    expect(max_diff(v, ref::v) < 1e-9, "steady fluxes vs fsolve");
    // elasticities (finite differences) vs the oracle's
    const Series e = mc::elasticities(v_of, x);
    expect(max_diff(e, ref::E) < 2e-6, "elasticities: " + std::to_string(max_diff(e, ref::E)));
    // control coefficients: unscaled -> scaled, vs COPASI link-matrix formula and vs re-solved enzyme scaling
    const auto cc = mc::coefficients(n, e, gam);
    const Series cs_scaled = mc::scale_concentration(cc.concentration, x, v), cj_scaled = mc::scale_flux(cc.flux, v);
    expect(max_diff(cc.concentration, ref::cs_copasi) < 2e-6, "C^S vs COPASI link-matrix formula: " + std::to_string(max_diff(cc.concentration, ref::cs_copasi)));
    expect(max_diff(cs_scaled, ref::cs_fd) < 5e-6, "scaled C^S vs finite differences of re-solved steady states: " + std::to_string(max_diff(cs_scaled, ref::cs_fd)));
    expect(max_diff(cj_scaled, ref::cj_fd) < 5e-6, "scaled C^J vs finite differences: " + std::to_string(max_diff(cj_scaled, ref::cj_fd)));
    // theorems
    expect(mc::summation_defect(cj_scaled, cs_scaled).max().item<double>() < 1e-8, "summation theorems");
    const Series b = mc::free_directions(gamma);
    expect(b.size(0) == 4 && b.size(1) == 3, "free directions are S x (S - C)");
    expect(mc::connectivity_defect(cc, e, b).max().item<double>() < 1e-8, "connectivity theorems on ker Gamma");
    // response of the steady state to k5 = C * dv/dk5 (parameter 8), vs finite differences of re-solved states
    const auto vp = [&](const Series &xs, const Series &p) { // xs [..., K, 4], p [..., K, 9]: replace k5 only
      const Series s1 = xs.select(-1, 0), s2 = xs.select(-1, 1), atp = xs.select(-1, 2), adp = xs.select(-1, 3);
      const auto P = [&](std::int64_t i) { return p.select(-1, i); };
      const Series v2 = P(1).mul(s1).mul(atp).div(s1.add(P(2)).mul(atp.add(P(3))));
      return cols({P(0).add(v2.mul(0.0)), v2, P(4).mul(s2).div(s2.add(P(5))), P(6).mul(adp).div(adp.add(P(7))), P(8).mul(atp)});
    };
    std::vector<double> pv;
    for (std::int64_t i = 0; i < lanes; ++i)
      for (std::size_t j = 0; j < bp.size(); ++j)
        pv.push_back(j == 0 ? ref::k1_lanes[static_cast<std::size_t>(i)] : bp[j]);
    const Series dv_dp = mc::parameter_derivatives(vp, x, mat(pv, {lanes, 9}));
    const Series dx = mc::response(cc.concentration, dv_dp.select(-1, 8).unsqueeze(-1)).squeeze(-1); // [lanes, 4]
    const Series dj = mc::response(cc.flux, dv_dp.select(-1, 8).unsqueeze(-1)).squeeze(-1);            // [lanes, 5]
    expect(max_diff(dx, ref::dx_dk5) < 5e-6, "dx*/dk5 = C^S dv/dk5: " + std::to_string(max_diff(dx, ref::dx_dk5)));
    // total flux response: dv_r/dk5 (explicit) + E dx*/dk5 should equal the response of the flux, = reference dv/dk5 with x re-solved
    expect(max_diff(dj, ref::dv_dk5) < 5e-6, "dJ/dk5 = C^J dv/dk5: " + std::to_string(max_diff(dj, ref::dv_dk5)));
    std::cout << "control tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
