#include <pluss/ma.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static int failures = 0;
static void check(bool ok, const std::string &what) {
  if (!ok) {
    ++failures;
    std::printf("FAIL: %s\n", what.c_str());
  }
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series num(double v) { return pluss::series::make(std::vector<double>{v}); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }
static std::vector<double> all(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static bool close(double a, double b, double rel = 1e-9) { return std::abs(a - b) <= rel * (1.0 + std::abs(b)); }
template <class F> static bool throws(F &&f) {
  try {
    f();
  } catch (const pluss::Error &) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

static void network_checks() {
  // Two conductances of 2 W/K in series between 100 and 20: the middle node is at 60 and the heat flow is 80 / (0.5 + 0.5).
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto r = th::steady(net);
  check(close(all(r.temperature)[1], 60.0), "series midpoint temperature");
  check(close(all(r.link_flow)[0], 80.0) && close(all(r.link_flow)[1], 80.0), "series heat flow");
  check(close(all(r.fixed_flow)[0], 80.0) && close(all(r.fixed_flow)[1], -80.0), "boundary heat delivered / absorbed");

  // Heat injected at a node leaves through both boundaries.
  th::Net injected = net;
  injected.heats = {{1}, vec({30.0})};
  const auto q = th::steady(injected);
  check(close(all(q.temperature)[1], 60.0 + 30.0 / 4.0), "injected heat raises the node by Q / (G1 + G2)");
  check(close(all(q.fixed_flow)[0] + all(q.fixed_flow)[1], -30.0), "boundaries carry the injected heat away");

  // Lumped capacity relaxing to a fixed temperature: T(t) = 20 + 80 exp(-t G / C).
  th::Net rc;
  rc.nodes = 2;
  rc.links = {{0, 1}, {1}, vec({0.5})};
  rc.links.a = {0};
  rc.links.b = {1};
  rc.stores = {{0}, vec({100.0})};
  rc.fixed = {{1}, vec({20.0})};
  std::vector<double> t;
  for (int k = 0; k <= 100; ++k) t.push_back(k * 2.0);
  const auto run = th::tran(rc, vec(t), {}, th::Start{vec({100.0})});
  const double got = all(run.temperature)[100 * 2];
  check(std::abs(got - (20.0 + 80.0 * std::exp(-200.0 * 0.5 / 100.0))) < 5e-3, "lumped cooling curve");

  // Batching over conductance.
  th::Net batch = net;
  batch.links.conductance = pluss::series::make(std::vector<double>{0.5, 0.5, 1.0, 0.25}).reshape({2, 2});
  const auto b = th::steady(batch);
  check(b.temperature.size(0) == 2 && b.temperature.size(1) == 3, "batched temperature shape");
  check(close(all(b.temperature)[1], 60.0) && close(all(b.temperature)[4], 100.0 - 80.0 * (1.0 / (1.0 + 4.0))), "batched midpoints");

  // Rejections.
  th::Net floating = net;
  floating.fixed = {{0}, vec({100.0})};
  floating.links = {{0}, {1}, vec({2.0})};
  check(throws([&] { th::steady(floating); }), "node without a path to a fixed temperature");
  th::Net none = net;
  none.fixed = {};
  check(throws([&] { th::steady(none); }), "no fixed temperature");
  th::Net bad = net;
  bad.links.a = {0, 5};
  check(throws([&] { th::steady(bad); }), "link index out of range");
}

static void resistance_checks() {
  check(close(one(th::wall(num(0.5), num(0.1), num(2.0))), 0.1), "plane wall");
  check(close(one(th::tube(num(1.0), num(1.0), num(std::exp(1.0)), num(1.0 / (2.0 * M_PI)))), 1.0), "cylinder shell");
  check(close(one(th::shell(num(1.0), num(1.0), num(2.0))), 0.5 / (4.0 * M_PI)), "spherical shell");
  check(close(one(th::film(num(10.0), num(0.5))), 0.2), "film");
  check(close(one(th::biot(num(10.0), num(2.0), num(0.1))), 0.5), "Biot");
  check(throws([] { th::wall(num(0.0), num(0.1), num(1.0)); }), "wall needs a positive conductivity");

  // Fin: adiabatic tip q = M tanh(mL); convective tip tends to the adiabatic one as h -> 0 relative to m k; infinite = M.
  const auto a = th::fin(num(25.0), num(200.0), num(0.04), num(1e-4), num(0.1), num(100.0), num(20.0), th::Tip::Adiabatic);
  const double m = std::sqrt(25.0 * 0.04 / (200.0 * 1e-4)), M = std::sqrt(25.0 * 0.04 * 200.0 * 1e-4) * 80.0;
  check(close(one(a.heat), M * std::tanh(m * 0.1)), "adiabatic fin heat");
  check(close(one(a.tip), 20.0 + 80.0 / std::cosh(m * 0.1)), "adiabatic fin tip");
  const auto inf = th::fin(num(25.0), num(200.0), num(0.04), num(1e-4), num(10.0), num(100.0), num(20.0), th::Tip::Infinite);
  check(close(one(inf.heat), M), "infinite fin heat");
  const auto conv = th::fin(num(25.0), num(200.0), num(0.04), num(1e-4), num(0.1), num(100.0), num(20.0), th::Tip::Convective);
  check(one(conv.heat) > one(a.heat), "a convective tip loses a little more than an adiabatic one");
  check(one(a.efficiency) > 0.0 && one(a.efficiency) < 1.0, "fin efficiency");
  check(throws([] { th::fin(num(25.0), num(200.0), num(0.04), num(1e-4), num(0.1), num(20.0), num(20.0)); }), "fin needs a temperature excess");

  // Layered wall: steady heat flow equals the series resistance.
  th::Layers layers;
  layers.cells = {3, 2};
  layers.thickness = pluss::series::make(std::vector<double>{0.2, 0.05}).reshape({2});
  layers.conductivity = pluss::series::make(std::vector<double>{0.8, 0.04}).reshape({2});
  layers.density = pluss::series::make(std::vector<double>{1800.0, 30.0}).reshape({2});
  layers.specific_heat = pluss::series::make(std::vector<double>{900.0, 1400.0}).reshape({2});
  th::Slab w = th::mesh(layers, num(1.0).reshape({}));
  check(w.net.nodes == 7 && w.inner == 0 && w.outer == 6 && w.cells == 5, "wall mesh node count");
  w.net.fixed = {{0, 6}, vec({20.0, 0.0})};
  const auto s = th::steady(w.net);
  const double rtot = 0.2 / 0.8 + 0.05 / 0.04;
  check(close(all(s.link_flow)[0], 20.0 / rtot, 1e-9), "wall steady flow equals the series resistance");
}

static void exchanger_checks() {
  // Limits: cr = 0 gives 1 - exp(-ntu) for every arrangement; counter flow at cr = 1 gives ntu / (1 + ntu).
  for (auto f : {th::Flow::Parallel, th::Flow::Counter, th::Flow::Cross, th::Flow::Shell, th::Flow::Boil})
    check(close(one(th::eff(num(2.0), num(0.0), f)), 1.0 - std::exp(-2.0), 1e-6), "cr = 0 effectiveness");
  check(close(one(th::eff(num(3.0), num(1.0), th::Flow::Counter)), 0.75, 1e-6), "balanced counter flow");
  check(close(one(th::eff(num(1.0), num(0.5), th::Flow::Parallel)), (1.0 - std::exp(-1.5)) / 1.5), "parallel effectiveness");
  // Round trip.
  for (auto f : {th::Flow::Parallel, th::Flow::Counter, th::Flow::Cross, th::Flow::Shell}) {
    const auto e = th::eff(num(1.7), num(0.6), f);
    check(close(one(th::ntu(e, num(0.6), f)), 1.7, 1e-7), "ntu inverts eff");
  }
  check(throws([] { th::ntu(num(0.9), num(0.5), th::Flow::Parallel); }), "parallel limit");
  // Energy balance of a rating.
  const auto r = th::rate(num(1000.0), num(2000.0), num(90.0), num(20.0), num(1500.0), th::Flow::Counter);
  check(close(1000.0 * (90.0 - one(r.hot_out)), one(r.duty)) && close(2000.0 * (one(r.cold_out) - 20.0), one(r.duty)), "rating energy balance");
  // LMTD and UA agree with the effectiveness method: duty = UA LMTD for counter flow.
  const double lm = one(th::lmtd(num(90.0), r.hot_out, num(20.0), r.cold_out, th::Flow::Counter));
  check(close(one(r.duty), 1500.0 * lm, 1e-7), "duty equals UA LMTD");
  check(throws([] { th::lmtd(num(50.0), num(30.0), num(40.0), num(45.0), th::Flow::Counter); }), "temperature cross");
}

static void moist_checks() {
  // Saturation pressure against the accepted tables.
  check(std::abs(one(th::psat(num(20.0))) - 2339.2) < 3.0, "psat 20 C");
  check(std::abs(one(th::psat(num(100.0))) - 101418.0) < 100.0, "psat 100 C");
  check(std::abs(one(th::psat(num(0.0))) - 611.2) < 1.0, "psat 0 C");
  const double p = 101325.0;
  const auto w = th::ratio(num(25.0), num(0.5), num(p));
  check(std::abs(one(w) - 0.00987) < 1e-4, "humidity ratio 25 C 50%");
  check(close(one(th::rh(num(25.0), w, num(p))), 0.5, 1e-9), "rh inverts ratio");
  check(std::abs(one(th::dew(w, num(p))) - 13.9) < 0.2, "dew point");
  check(std::abs(one(th::wet(num(25.0), w, num(p))) - 17.9) < 0.3, "wet bulb");
  const double td = one(th::dew(w, num(p)));
  check(close(one(th::rh(num(td), w, num(p))), 1.0, 1e-7), "air is saturated at its dew point");
  check(std::abs(one(th::wet(num(td), w, num(p))) - td) < 1e-4, "saturated air: wet bulb equals dry bulb");
  check(std::abs(one(th::enth(num(25.0), w)) - 50300.0) < 300.0, "enthalpy");
  check(std::abs(one(th::vol(num(25.0), w, num(p))) - 0.8585) < 0.005, "specific volume");
  const auto m = th::mix(num(30.0), num(0.012), num(1.0), num(10.0), num(0.006), num(1.0));
  check(close(one(m.w), 0.009) && std::abs(one(m.temperature) - 20.0) < 0.1, "mixing");
  check(close(one(th::enth(m.temperature, m.w)), 0.5 * (one(th::enth(num(30.0), num(0.012))) + one(th::enth(num(10.0), num(0.006)))), 1e-9), "mixing conserves enthalpy");
  check(throws([] { th::psat(num(500.0)); }), "psat range");
  check(throws([&] { th::ratio(num(25.0), num(1.5), num(p)); }), "relative humidity above one");
}

static void property_checks() {
  const auto water = th::props(th::Fluid::Water, num(293.15));
  check(std::abs(one(water.density) - 998.2) < 0.1, "water density 20 C");
  check(std::abs(one(water.specific_heat) - 4183.0) < 6.0, "water cp 20 C");
  check(std::abs(one(water.prandtl) - 7.0) < 0.4, "water Prandtl 20 C");
  const auto air = th::props(th::Fluid::Air, num(300.0));
  check(std::abs(one(air.density) - 1.177) < 0.003, "air density 300 K");
  check(std::abs(one(air.prandtl) - 0.707) < 0.005, "air Prandtl 300 K");
  const auto high = th::props(th::Fluid::Air, num(300.0), num(2.0 * 101325.0));
  check(close(one(high.density), 2.0 * one(air.density)), "air density scales with pressure");
  check(throws([] { th::props(th::Fluid::Water, num(400.0)); }), "water range");
  check(!th::source(th::Fluid::Water).reference.empty() && !th::source(th::Fluid::Air).state.empty(), "provenance strings");
}

int main() {
  network_checks();
  resistance_checks();
  exchanger_checks();
  moist_checks();
  property_checks();
  if (failures == 0) std::printf("thermal: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
