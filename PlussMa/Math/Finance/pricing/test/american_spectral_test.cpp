// The spectral (integral-equation) American engine, checked against references that share no code with it: a Leisen-Reimer tree,
// a Crank-Nicolson finite-difference solver with Rannacher start and projected SOR, and the defining conditions of the boundary.
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using fin::pricing::OptionType;

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

Config cfg() { return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU); }

struct Case {
  bool call;
  double s, k, r, q, t, sigma;
};

double peizer_pratt(double z, double n) {
  const double a = z / (n + 1.0 / 3.0 + 0.1 / (n + 1.0));
  return 0.5 + (z >= 0 ? 0.5 : -0.5) * std::sqrt(1.0 - std::exp(-a * a * (n + 1.0 / 6.0)));
}

// Leisen-Reimer binomial tree (odd n): second-order and monotone, unlike the Cox-Ross-Rubinstein tree.
double leisen_reimer(const Case& c, int n, bool american) {
  const double dt = c.t / n, sq = c.sigma * std::sqrt(c.t);
  const double d1 = (std::log(c.s / c.k) + (c.r - c.q + 0.5 * c.sigma * c.sigma) * c.t) / sq, d2 = d1 - sq;
  const double p = peizer_pratt(d2, n), pp = peizer_pratt(d1, n);
  const double growth = std::exp((c.r - c.q) * dt), up = growth * pp / p, down = (growth - p * up) / (1.0 - p), disc = std::exp(-c.r * dt);
  auto pay = [&](double s) { return c.call ? std::max(s - c.k, 0.0) : std::max(c.k - s, 0.0); };
  std::vector<double> v(static_cast<std::size_t>(n) + 1);
  for (int i = 0; i <= n; ++i) v[static_cast<std::size_t>(i)] = pay(c.s * std::pow(up, i) * std::pow(down, n - i));
  for (int j = n - 1; j >= 0; --j)
    for (int i = 0; i <= j; ++i) {
      double x = disc * (p * v[static_cast<std::size_t>(i) + 1] + (1.0 - p) * v[static_cast<std::size_t>(i)]);
      if (american) x = std::max(x, pay(c.s * std::pow(up, i) * std::pow(down, j - i)));
      v[static_cast<std::size_t>(i)] = x;
    }
  return v[0];
}

// Crank-Nicolson on ln S with projected SOR; two implicit half-step pairs first (Rannacher) to damp the payoff kink.  Puts only.
double crank_nicolson_put(const Case& c, int nx, int nt) {
  const double half_width = std::max(5.0 * c.sigma * std::sqrt(c.t), 1.5) + std::abs(c.r - c.q) * c.t;
  const double h = 2.0 * half_width / nx, x0 = std::log(c.s) - half_width, dt = c.t / nt;
  const double a = 0.5 * c.sigma * c.sigma, b = c.r - c.q - 0.5 * c.sigma * c.sigma;
  std::vector<double> payoff(static_cast<std::size_t>(nx) + 1), v(payoff.size()), rhs(payoff.size());
  for (int j = 0; j <= nx; ++j) payoff[static_cast<std::size_t>(j)] = v[static_cast<std::size_t>(j)] = std::max(c.k - std::exp(x0 + j * h), 0.0);
  const double lo = a / (h * h) - b / (2 * h), mid = -2 * a / (h * h) - c.r, hi = a / (h * h) + b / (2 * h);
  auto step = [&](double theta, double dts) {
    for (int j = 1; j < nx; ++j) {
      const auto k = static_cast<std::size_t>(j);
      rhs[k] = v[k] + (1.0 - theta) * dts * (lo * v[k - 1] + mid * v[k] + hi * v[k + 1]);
    }
    const double left = std::max(c.k - std::exp(x0), 0.0);
    std::vector<double> w = v;
    w[0] = left;
    w[static_cast<std::size_t>(nx)] = 0.0;
    for (int it = 0; it < 20000; ++it) {
      double err = 0.0;
      for (int j = 1; j < nx; ++j) {
        const auto k = static_cast<std::size_t>(j);
        const double diag = 1.0 - theta * dts * mid;
        double fresh = (rhs[k] + theta * dts * (lo * w[k - 1] + hi * w[k + 1])) / diag;
        fresh = std::max(w[k] + 1.3 * (fresh - w[k]), payoff[k]);
        err = std::max(err, std::abs(fresh - w[k]));
        w[k] = fresh;
      }
      if (err < 1e-13) break;
    }
    v = w;
  };
  for (int m = 0; m < 4; ++m) step(1.0, 0.5 * dt);  // two implicit steps, each in two halves
  for (int m = 2; m < nt; ++m) step(0.5, dt);
  return v[static_cast<std::size_t>(nx / 2)];
}

double european(const Case& c) {
  const auto cdf = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
  const double d1 = (std::log(c.s / c.k) + (c.r - c.q + 0.5 * c.sigma * c.sigma) * c.t) / (c.sigma * std::sqrt(c.t)), d2 = d1 - c.sigma * std::sqrt(c.t);
  return c.call ? c.s * std::exp(-c.q * c.t) * cdf(d1) - c.k * std::exp(-c.r * c.t) * cdf(d2)
                : c.k * std::exp(-c.r * c.t) * cdf(-d2) - c.s * std::exp(-c.q * c.t) * cdf(-d1);
}

double spectral(const Case& c, const fin::pricing::SpectralOptions& o = {}) {
  return fin::pricing::detail::spectral_price(c.call ? OptionType::Call : OptionType::Put, c.s, c.k, c.r, c.q, c.t, c.sigma, o);
}

const std::vector<Case> kCases{
    {false, 100, 100, 0.05, 0.0, 1.0, 0.2},   {false, 80, 100, 0.04, 0.01, 3.0, 0.2}, {false, 100, 100, 0.02, 0.06, 1.0, 0.3},
    {true, 100, 100, 0.03, 0.07, 1.0, 0.25},  {true, 100, 100, 0.05, 0.0, 1.0, 0.2},  {false, 40, 45, 0.06, 0.0, 0.5, 0.4},
    {false, 120, 100, 0.08, 0.0, 0.25, 0.15}, {true, 90, 100, 0.01, 0.1, 2.0, 0.35},  {false, 100, 100, 0.05, 0.0, 10.0, 0.2}};

// Early exercise limits any tree to first-order convergence, so the reference is Richardson-extrapolated from two sizes.
double leisen_reimer_extrapolated(const Case& c) {
  const double n1 = 2001, n2 = 4001;
  return (n2 * leisen_reimer(c, 4001, true) - n1 * leisen_reimer(c, 2001, true)) / (n2 - n1);  // error ~ 1/N
}

void test_matches_leisen_reimer() {
  for (const auto& c : kCases) {
    if (c.t > 5.0) continue;  // a tree this coarse in time is no reference for a ten-year option
    const double want = leisen_reimer_extrapolated(c), got = spectral(c);
    expect(std::abs(got - want) < 5e-5, "spectral vs Leisen-Reimer: " + std::to_string(got) + " vs " + std::to_string(want));
  }
}

void test_matches_crank_nicolson() {
  for (const auto& c : kCases) {
    if (c.call) continue;
    const double want = crank_nicolson_put(c, 1600, 800), got = spectral(c);
    expect(std::abs(got - want) < 3e-4, "spectral vs finite difference: " + std::to_string(got) + " vs " + std::to_string(want));
  }
}

void test_spectral_converges_geometrically_in_the_nodes() {
  for (const auto& c : {kCases[0], kCases[2], kCases[8]}) {
    const double ref = spectral(c, {.nodes = 64, .quadrature = 128, .max_iterations = 200});
    double previous = 1e9;
    for (const int n : {4, 8, 12, 24}) {
      const double err = std::abs(spectral(c, {.nodes = n, .quadrature = 64}) - ref);
      expect(err < previous || err < 1e-12, "the error falls as the nodes rise");
      previous = err;
    }
    expect(previous < 1e-8, "24 nodes are accurate to 1e-8: " + std::to_string(previous));
    expect(std::abs(spectral(c) - ref) < 1e-8, "the defaults are accurate to 1e-8");
  }
}

void test_boundary_conditions() {
  // value matching and smooth pasting at the boundary: the price meets the intrinsic value K - S with slope -1
  for (const auto& c : {Case{false, 100, 100, 0.05, 0.0, 1.0, 0.2}, Case{false, 100, 100, 0.02, 0.06, 1.0, 0.3}}) {
    const auto times = pluss::make_series({c.t * 0.25, c.t * 0.5, c.t}, {3}, cfg());
    const auto b = fin::pricing::detail::spectral_boundary(OptionType::Put, c.k, c.r, c.q, c.t, c.sigma, times).items<double>();
    expect(b[0] > b[1] && b[1] > b[2], "the boundary falls as the time to maturity rises");
    const double edge = b[2], h = 1e-5 * c.k;
    Case at = c;
    at.s = edge + h;
    const double gap = spectral(at) - (c.k - at.s);
    expect(gap >= -1e-12 && gap < 2e-8, "value matching at the boundary: gap " + std::to_string(gap));
    Case lo = c, hi = c;
    lo.s = edge + 2e-5 * c.k;
    hi.s = edge + 4e-5 * c.k;
    const double slope = (spectral(hi) - spectral(lo)) / (hi.s - lo.s);
    expect(std::abs(slope + 1.0) < 5e-4, "smooth pasting: slope " + std::to_string(slope));
    Case in = c;
    in.s = edge * 0.9;
    expect(std::abs(spectral(in) - (c.k - in.s)) < 1e-12, "below the boundary the price is the intrinsic value");
  }
  const auto zero = fin::pricing::detail::spectral_boundary(OptionType::Put, 100.0, 0.02, 0.06, 1.0, 0.3, pluss::make_series({0.0}, {1}, cfg())).items<double>();
  expect(std::abs(zero[0] - 100.0 * 0.02 / 0.06) < 1e-12, "at expiry the boundary is K r / q when q > r");
  const auto zero2 = fin::pricing::detail::spectral_boundary(OptionType::Put, 100.0, 0.05, 0.0, 1.0, 0.2, pluss::make_series({0.0}, {1}, cfg())).items<double>();
  expect(std::abs(zero2[0] - 100.0) < 1e-12, "at expiry the boundary is K when q <= r");
}

void test_limits_and_symmetry() {
  const Case nodiv_call{true, 100, 100, 0.05, 0.0, 1.0, 0.25};
  expect(std::abs(spectral(nodiv_call) - european(nodiv_call)) < 1e-9, "a call without dividends is a European call");
  const Case neg_rate{false, 100, 100, -0.01, 0.0, 1.0, 0.2};
  expect(std::abs(spectral(neg_rate) - european(neg_rate)) < 1e-12, "a put with non-positive interest is European");
  const Case c{true, 100, 95, 0.03, 0.07, 1.0, 0.25};
  expect(std::abs(spectral(c) - fin::pricing::detail::spectral_price(OptionType::Put, 95, 100, 0.07, 0.03, 1.0, 0.25)) < 1e-12, "call-put symmetry");
  const Case deep{false, 40, 100, 0.05, 0.0, 1.0, 0.2};
  expect(std::abs(spectral(deep) - 60.0) < 1e-9, "deep in the money: intrinsic");
  const Case expired{false, 90, 100, 0.05, 0.0, 0.0, 0.2};
  expect(std::abs(spectral(expired) - 10.0) < 1e-15, "zero maturity: intrinsic");
  for (const auto& k : kCases) expect(spectral(k) >= european(k) - 1e-12, "never below the European value");
}

void test_field_engine() {
  mod::Field f;
  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.market = fin::market::make(100.0, 0.05, 0.0);
  f.model = fin::model::make(fin::model::Kind::BlackScholes, {0.2});
  f.controls.engine = mod::Engine::Spectral;
  f.controls.risk = mod::Risk::All;
  const auto out = f.price();
  expect(std::abs(out.price.items<double>()[0] - leisen_reimer_extrapolated(kCases[0])) < 5e-5, "Field price");
  const double delta = out.greeks.delta.items<double>()[0], gamma = out.greeks.gamma.items<double>()[0];
  expect(delta < -0.38 && delta > -0.44 && gamma > 0.0 && gamma < 0.05, "Greeks are those of an American put");
  expect(f.engine_name() == "Spectral", "engine name");

  auto lattice = f;
  lattice.controls.engine = mod::Engine::Lattice;
  lattice.controls.lattice.steps = 4000;
  expect(std::abs(lattice.price().price.items<double>()[0] - out.price.items<double>()[0]) < 5e-4, "lattice agrees with the spectral price");

  f.instrument = fin::instrument::bermudan(OptionType::Put, 100.0, 1.0, std::vector<double>{0.5});
  bool threw = false;
  try {
    f.price();
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("Bermudan") != std::string::npos;
  }
  expect(threw, "a Bermudan option is refused by name");

  f.instrument = fin::instrument::make(OptionType::Put, 100.0, 1.0);
  expect(std::abs(f.price().price.items<double>()[0] - european(kCases[0])) < 1e-10, "European exercise through the Spectral engine is the closed form");

  f.instrument = fin::instrument::american(OptionType::Put, 100.0, 1.0);
  f.instrument.strike = pluss::make_series({90.0, 100.0, 110.0}, {3}, cfg());
  const auto strip = f.price().price.items<double>();
  expect(strip.size() == 3 && strip[0] < strip[1] && strip[1] < strip[2], "a strip of strikes");
}

}  // namespace

int main() {
  try {
    test_matches_leisen_reimer();
    test_matches_crank_nicolson();
    test_spectral_converges_geometrically_in_the_nodes();
    test_boundary_conditions();
    test_limits_and_symmetry();
    test_field_engine();
    std::cout << "american_spectral_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "american_spectral_test failed: " << e.what() << '\n';
    return 1;
  }
}
