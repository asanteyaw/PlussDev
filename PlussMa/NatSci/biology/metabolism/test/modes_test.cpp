// Elementary flux modes vs brute-force minimal supports, and hit-and-run flux sampling vs exact moments of simple polytopes.
#include "pluss/subsp/natsci/biology/metabolism/modes.h"
#include "modes_reference.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace mt = pluss::subsp::natsci::biology::metabolism;
namespace ref = modes_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }

// canonical row-sorted list of modes for set comparison
std::vector<std::vector<double>> rows_of(const std::vector<double> &flat, std::size_t r) {
  std::vector<std::vector<double>> out;
  for (std::size_t i = 0; i + r <= flat.size(); i += r)
    out.emplace_back(flat.begin() + static_cast<std::ptrdiff_t>(i), flat.begin() + static_cast<std::ptrdiff_t>(i + r));
  std::sort(out.begin(), out.end());
  return out;
}
bool same_modes(std::vector<std::vector<double>> a, std::vector<std::vector<double>> b, double tol) {
  if (a.size() != b.size())
    return false;
  for (auto &x : a) {
    bool found = false;
    for (std::size_t q = 0; q < b.size() && !found; ++q) {
      double d = 0.0;
      for (std::size_t j = 0; j < x.size(); ++j) d = std::max(d, std::abs(x[j] - b[q][j]));
      if (d < tol) { found = true; b.erase(b.begin() + static_cast<std::ptrdiff_t>(q)); }
    }
    if (!found)
      return false;
  }
  return true;
}
} // namespace

int main() {
  try {
    // ---- EFMs of eight random networks (4 metabolites, 9 reactions, mixed reversibility) against brute-force enumeration
    const std::vector<const std::vector<double> *> s{&ref::s0, &ref::s1, &ref::s2, &ref::s3, &ref::s4, &ref::s5, &ref::s6, &ref::s7};
    const std::vector<const std::vector<double> *> lo{&ref::lo0, &ref::lo1, &ref::lo2, &ref::lo3, &ref::lo4, &ref::lo5, &ref::lo6, &ref::lo7};
    const std::vector<const std::vector<double> *> hi{&ref::hi0, &ref::hi1, &ref::hi2, &ref::hi3, &ref::hi4, &ref::hi5, &ref::hi6, &ref::hi7};
    const std::vector<const std::vector<double> *> modes{&ref::modes0, &ref::modes1, &ref::modes2, &ref::modes3, &ref::modes4, &ref::modes5, &ref::modes6, &ref::modes7};
    const std::vector<int> mm{ref::m0, ref::m1, ref::m2, ref::m3, ref::m4, ref::m5, ref::m6, ref::m7}, rr{ref::r0, ref::r1, ref::r2, ref::r3, ref::r4, ref::r5, ref::r6, ref::r7}, cc{ref::count0, ref::count1, ref::count2, ref::count3, ref::count4, ref::count5, ref::count6, ref::count7};
    for (std::size_t i = 0; i < 8; ++i) {
      mt::Network net;
      net.stoichiometry = mat(*s[i], {mm[i], rr[i]});
      net.lower = mat(*lo[i], {rr[i]});
      net.upper = mat(*hi[i], {rr[i]});
      const Series e = mt::elementary_modes(net);
      expect(e.size(0) == cc[i], "network " + std::to_string(i) + ": " + std::to_string(e.size(0)) + " modes vs " + std::to_string(cc[i]) + " by brute force");
      expect(same_modes(rows_of(e.contiguous().host<double>().array().data, static_cast<std::size_t>(rr[i])), rows_of(*modes[i], static_cast<std::size_t>(rr[i])), 1e-7), "network " + std::to_string(i) + ": the mode sets differ");
      // every mode is steady and respects the signs
      const Series resid = net.stoichiometry.matmul(e.transpose(0, 1));
      expect(e.size(0) == 0 || resid.abs().max().item<double>() < 1e-9, "modes satisfy S v = 0");
    }
    // ---- a small hand-checkable network: glycolysis-like branch  A -> B -> C, B -> D, A -> D (all irreversible): 2 modes; make B <-> A reversible: still 2 + the cycle
    {
      mt::Network net;
      net.stoichiometry = mat({1, -1, -0, 0, -1, 0,   // A:  uptake(+1), A->B(-1), ..., A->D(-1)
                               0, 1, -1, -1, 0, 0,    // B:  A->B(+1), B->C(-1), B->D(-1)
                               0, 0, 1, 0, 0, -1,     // C:  B->C(+1), export C(-1)
                               0, 0, 0, 1, 1, 0},     // D:  B->D, A->D produce D (no consumer yet)
                              {4, 6});
      net.lower = mat({0, 0, 0, 0, 0, 0}, {6});
      net.upper = mat({10, 10, 10, 10, 10, 10}, {6});
      // D is a dead end: modes must avoid reactions 3 and 4 (only reaction 0, 1, 2, 5 form a mode)
      const Series e = mt::elementary_modes(net);
      expect(e.size(0) == 1, "dead-end metabolite D leaves exactly one mode: " + std::to_string(e.size(0)));
      const auto v = e.contiguous().host<double>().array().data;
      expect(std::abs(v[0] - 1.0) < 1e-12 && std::abs(v[1] - 1.0) < 1e-12 && std::abs(v[2] - 1.0) < 1e-12 && std::abs(v[3]) < 1e-12 && std::abs(v[4]) < 1e-12 && std::abs(v[5] - 1.0) < 1e-12, "the single mode is uptake -> A -> B -> C -> export");
    }
    // ---- hit-and-run: triangle v1 = v2 + v3, 0 <= v <= 1  (v2, v3 uniform on {v2 + v3 <= 1})
    {
      mt::Network net;
      net.stoichiometry = mat({1, -1, -1}, {1, 3});
      net.lower = mat({0, 0, 0}, {3});
      net.upper = mat({1, 1, 1}, {3});
      const auto sm = mt::sample_fluxes(net, 400, 150, 20, 7);
      const Series f = sm.fluxes.reshape({400 * 150, 3});
      const Series mu = f.mean(0, false);
      const auto m = mu.contiguous().host<double>().array().data;
      const Series c2 = f.select(-1, 1).sub(m[1]), c3 = f.select(-1, 2).sub(m[2]);
      expect(sm.chebyshev_radius > 0.1 && sm.fluxes.min().item<double>() >= -1e-9 && sm.fluxes.max().item<double>() <= 1.0 + 1e-9, "samples stay inside the polytope: radius " + std::to_string(sm.chebyshev_radius) + " min " + std::to_string(sm.fluxes.min().item<double>()) + " max " + std::to_string(sm.fluxes.max().item<double>()));
      expect(std::abs(m[1] - 1.0 / 3.0) < 4e-3 && std::abs(m[2] - 1.0 / 3.0) < 4e-3 && std::abs(m[0] - 2.0 / 3.0) < 4e-3, "triangle means (1/3, 1/3), v1 = 2/3");
      expect(std::abs(c2.square().mean().item<double>() - 1.0 / 18.0) < 2e-3 && std::abs(c2.mul(c3).mean().item<double>() + 1.0 / 36.0) < 2e-3, "triangle variance 1/18 and covariance -1/36");
      // S v = 0 holds for every sample
      expect(f.select(-1, 0).sub(f.select(-1, 1)).sub(f.select(-1, 2)).abs().max().item<double>() < 1e-9, "samples satisfy S v = 0");
    }
    // ---- box in the nullspace: v1 = v2 in [0, 1], v3 = v4 in [0, 2], v5 in [-1, 1], plus a fixed flux v6 = 0.5 (kept exactly)
    {
      mt::Network net;
      net.stoichiometry = mat({1, -1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0}, {2, 6});
      net.lower = mat({0, 0, 0, 0, -1, 0.5}, {6});
      net.upper = mat({1, 1, 2, 2, 1, 0.5}, {6});
      const auto sm = mt::sample_fluxes(net, 300, 200, 15, 11);
      const Series f = sm.fluxes.reshape({300 * 200, 6});
      const auto m = f.mean(0, false).contiguous().host<double>().array().data;
      expect(std::abs(m[0] - 0.5) < 6e-3 && std::abs(m[2] - 1.0) < 1.2e-2 && std::abs(m[4]) < 1.2e-2 && std::abs(m[5] - 0.5) < 1e-12, "box means (0.5, 1, 0) and the fixed flux");
      const Series v3 = f.select(-1, 2).sub(1.0);
      expect(std::abs(v3.square().mean().item<double>() - 1.0 / 3.0) < 1.5e-2, "uniform variance of v3 on [0, 2] is 1/3");
      expect(std::abs(f.select(-1, 0).sub(f.select(-1, 1)).abs().max().item<double>()) < 1e-9, "v1 = v2 for every sample");
      // fourth moment of a uniform on [-1, 1] is 1/5
      expect(std::abs(f.select(-1, 4).pow(4.0).mean().item<double>() - 0.2) < 1.2e-2, "fourth moment of v5 ~ U[-1, 1]");
    }
    // ---- robustness: sentinels / infinities (formerly a silent radius-0 failure), unbounded polytopes, and network-pinned reactions
    {
      // v1 = v2 in [0, +inf): truncated at the cap 10 -> uniform on [0, 10]; the bound is given three ways and must behave identically
      for (const double top : {std::numeric_limits<double>::infinity(), 1e300, 1e25}) {
        mt::Network net;
        net.stoichiometry = mat({1, -1}, {1, 2});
        net.lower = mat({0, 0}, {2});
        net.upper = mat({top, top}, {2});
        mt::SamplingOptions opt;
        opt.bound_cap = 10.0;
        const auto sm = mt::sample_fluxes(net, 300, 200, 15, 3, opt);
        expect(sm.truncated_bounds == 2 && sm.chebyshev_radius > 1.0, "unbounded sides are truncated and the Chebyshev radius is positive: " + std::to_string(sm.chebyshev_radius));
        const Series v = sm.fluxes.select(-1, 0).reshape({300 * 200});
        expect(std::abs(v.mean().item<double>() - 5.0) < 0.08 && std::abs(v.sub(5.0).square().mean().item<double>() - 100.0 / 12.0) < 0.25, "uniform on the truncated [0, 10]");
        expect(sm.fluxes.select(-1, 0).sub(sm.fluxes.select(-1, 1)).abs().max().item<double>() < 1e-9, "v1 = v2 for every sample");
      }
      // blocked reaction: D is produced by v3 and consumed by nothing, so v3 = 0 although its bounds allow [0, 1]
      mt::Network net;
      net.stoichiometry = mat({1, -1, -1, 0, 0, 1}, {2, 3});
      net.lower = mat({0, 0, 0}, {3});
      net.upper = mat({1, 1, 1}, {3});
      const auto sm = mt::sample_fluxes(net, 200, 150, 10, 5);
      expect(sm.fixed_reactions.size() == 1 && sm.fixed_reactions[0] == 2, "reaction 3 is detected as pinned by the network");
      expect(sm.fluxes.select(-1, 2).abs().max().item<double>() < 1e-12, "the blocked reaction stays exactly zero");
      const Series v = sm.fluxes.select(-1, 0).reshape({200 * 150});
      expect(std::abs(v.mean().item<double>() - 0.5) < 8e-3 && std::abs(v.sub(0.5).square().mean().item<double>() - 1.0 / 12.0) < 2e-3, "v1 = v2 uniform on [0, 1]");
      // a genuinely interior-free polytope after fixing (a single point) raises a clear error
      mt::Network point;
      point.stoichiometry = mat({1, -1}, {1, 2});
      point.lower = mat({0.5, 0.5}, {2});
      point.upper = mat({0.5, 0.5}, {2});
      bool threw = false;
      try { (void)mt::sample_fluxes(point, 10, 10); } catch (const std::exception &) { threw = true; }
      expect(threw, "a single-point flux space raises");
    }
    std::cout << "modes tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
