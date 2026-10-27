// Ito and Stratonovich calculus on Series: differentials of functions of Ito processes (including
// products and jumps) via `x.calc.diff.ito(...)`, and stochastic integrals along a path via
// `w.calc.antidiff.ito(...)`.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <vector>

namespace {
using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::codem::SeriesItoState;
using pluss::codem::SeriesJumpMeasure;

Series vec(std::vector<double> v) {
  return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64);
}
Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) {
  return pluss::make_series(v, {r, c}).to(ScalarType::Float64);
}
double at(const Series& s, std::int64_t i) { return s.host<double>()[static_cast<std::size_t>(i)]; }

const std::vector<double> kX{0.5, 1.0, -0.3};
const std::vector<double> kMu{0.1, 0.2, -0.1};
const std::vector<double> kSigma{0.3, 0.5, 0.2};
}  // namespace

TEST_CASE("Ito's lemma for a function of one process", "[stochastic][ito]") {
  const Series x = vec(kX), mu = vec(kMu), sigma = vec(kSigma);
  const auto sq = x.calc.diff.ito([](const Series& s) { return s.mul(s); }, mu, sigma);
  const auto ex = x.calc.diff.ito([](const Series& s) { return s.exp(); }, mu, sigma);
  const auto sn = x.calc.diff.ito([](const Series& s) { return s.sin(); }, mu, sigma);
  REQUIRE(sq.diffusion.dim() == 2);  // shape(x) + [one factor]
  for (std::int64_t i = 0; i < 3; ++i) {
    const double xi = kX[i], m = kMu[i], s = kSigma[i];
    CHECK(at(sq.value, i) == Approx(xi * xi));
    CHECK(at(sq.drift, i) == Approx(2 * xi * m + s * s));              // g' mu + 1/2 sigma^2 g''
    CHECK(at(sq.diffusion, i) == Approx(2 * xi * s));                  // g' sigma
    CHECK(at(ex.drift, i) == Approx(std::exp(xi) * (m + 0.5 * s * s)));
    CHECK(at(ex.diffusion, i) == Approx(std::exp(xi) * s));
    CHECK(at(sn.drift, i) == Approx(std::cos(xi) * m - 0.5 * std::sin(xi) * s * s));
  }
}

TEST_CASE("several Brownian factors add their variances", "[stochastic][ito]") {
  const Series x = vec(kX), mu = vec(kMu);
  const std::vector<double> sg{0.3, 0.1, 0.5, 0.2, 0.2, 0.4};  // [3, 2]
  const auto d = x.calc.diff.ito([](const Series& s) { return s.mul(s); }, mu, mat(sg, 3, 2));
  REQUIRE(d.diffusion.sizes()[1] == 2);
  for (std::int64_t i = 0; i < 3; ++i) {
    const double s1 = sg[2 * i], s2 = sg[2 * i + 1];
    CHECK(at(d.drift, i) == Approx(2 * kX[i] * kMu[i] + s1 * s1 + s2 * s2));
    CHECK(at(d.diffusion, 2 * i) == Approx(2 * kX[i] * s1));
    CHECK(at(d.diffusion, 2 * i + 1) == Approx(2 * kX[i] * s2));
  }
}

TEST_CASE("Ito's product rule includes the covariation of the factors", "[stochastic][ito]") {
  const Series x = vec(kX), mu = vec(kMu), sx = mat({0.3, 0.1, 0.5, 0.2, 0.2, 0.4}, 3, 2);
  const std::vector<double> yv{2.0, 1.5, 0.8}, ym{0.05, -0.1, 0.3}, ys{0.2, 0.6, 0.1, 0.3, 0.4, 0.2};
  const SeriesItoState other{vec(yv), vec(ym), mat(ys, 3, 2)};
  const auto d = x.calc.diff.ito_product(mu, sx, other);
  const auto sxv = sx.host<double>();
  for (std::int64_t i = 0; i < 3; ++i) {
    const double cov = sxv[2 * i] * ys[2 * i] + sxv[2 * i + 1] * ys[2 * i + 1];
    CHECK(at(d.drift, i) == Approx(kX[i] * ym[i] + yv[i] * kMu[i] + cov));  // x dy + y dx + d[x, y]
    for (int f = 0; f < 2; ++f)
      CHECK(at(d.diffusion, 2 * i + f) == Approx(kX[i] * ys[2 * i + f] + yv[i] * sxv[2 * i + f]));
  }
  // the same state with itself: d(x^2) = 2 x dx + d[x]
  const auto self = x.calc.diff.ito_product(mu, sx, SeriesItoState{x, mu, sx});
  const auto sq = x.calc.diff.ito([](const Series& s) { return s.mul(s); }, mu, sx);
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(self.drift, i) == Approx(at(sq.drift, i)));
}

TEST_CASE("Ito's quotient rule", "[stochastic][ito]") {
  const Series x = vec(kX), mu = vec(kMu), sx = mat({0.3, 0.1, 0.5, 0.2, 0.2, 0.4}, 3, 2);
  const std::vector<double> yv{2.0, 1.5, 0.8}, ym{0.05, -0.1, 0.3}, ys{0.2, 0.6, 0.1, 0.3, 0.4, 0.2};
  const auto d = x.calc.diff.ito_quotient(mu, sx, SeriesItoState{vec(yv), vec(ym), mat(ys, 3, 2)});
  const auto sxv = sx.host<double>();
  for (std::int64_t i = 0; i < 3; ++i) {
    const double X = kX[i], Y = yv[i];
    const double cxy = sxv[2 * i] * ys[2 * i] + sxv[2 * i + 1] * ys[2 * i + 1];
    const double cyy = ys[2 * i] * ys[2 * i] + ys[2 * i + 1] * ys[2 * i + 1];
    CHECK(at(d.drift, i) == Approx(kMu[i] / Y - X * ym[i] / (Y * Y) - cxy / (Y * Y) + X * cyy / (Y * Y * Y)));
    CHECK(at(d.diffusion, 2 * i) == Approx(sxv[2 * i] / Y - X * ys[2 * i] / (Y * Y)));
  }
}

TEST_CASE("a time-dependent function picks up its explicit time derivative", "[stochastic][ito]") {
  const Series x = vec(kX), mu = vec(kMu), sigma = vec(kSigma);
  const auto d = x.calc.diff.ito_at(pluss::make_series(0.7).to(ScalarType::Float64),
                                    [](const Series& t, const Series& s) { return t.mul(s).mul(s); }, mu, sigma);
  for (std::int64_t i = 0; i < 3; ++i)
    CHECK(at(d.drift, i) == Approx(kX[i] * kX[i] + 0.7 * (2 * kX[i] * kMu[i] + kSigma[i] * kSigma[i])));
}

TEST_CASE("Stratonovich differentials follow the ordinary chain rule", "[stochastic][strat]") {
  const Series x = vec(kX), mu = vec(kMu), sigma = vec(kSigma);
  const auto d = x.calc.diff.strat([](const Series& s) { return s.mul(s); }, mu, sigma);
  for (std::int64_t i = 0; i < 3; ++i) {
    CHECK(at(d.drift, i) == Approx(2 * kX[i] * kMu[i]));
    CHECK(at(d.diffusion, i) == Approx(2 * kX[i] * kSigma[i]));
  }
}

TEST_CASE("drift conventions convert and round-trip", "[stochastic][strat]") {
  const Series x = vec({0.5, 1.0, 2.0}), mu = vec({0.1, 0.2, 0.3});
  const auto gbm = [](const Series& s) { return s.mul(0.4); };  // sigma(x) = 0.4 x
  const Series mu_s = x.calc.diff.strat_drift(mu, gbm);
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(mu_s, i) == Approx(at(mu, i) - 0.5 * 0.4 * 0.4 * at(x, i)));
  const Series back = x.calc.diff.ito_drift(mu_s, gbm);
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(back, i) == Approx(at(mu, i)));

  // The two routes to the Ito drift of g = x^2 agree:
  // Ito's lemma, or the ordinary chain rule on the Stratonovich form followed by conversion back.
  const auto ito_route = x.calc.diff.ito([](const Series& s) { return s.mul(s); }, mu, x.mul(0.4));
  const auto strat_route = x.calc.diff.strat([](const Series& s) { return s.mul(s); }, mu_s, x.mul(0.4));
  const Series g = x.mul(x);
  const Series ito_from_strat = g.calc.diff.ito_drift(strat_route.drift, [](const Series& s) { return s.mul(0.8); });
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(ito_from_strat, i) == Approx(at(ito_route.drift, i)));
}

TEST_CASE("jumps: the jump part of the differential and its compensator", "[stochastic][levy]") {
  const Series x = vec(kX), mu = vec(kMu), sigma = vec(kSigma);
  const std::vector<double> z{-0.2, 0.3}, nu{1.0, 0.5};
  const auto sq = [](const Series& s) { return s.mul(s); };
  const auto base = x.calc.diff.ito(sq, mu, sigma);

  SeriesJumpMeasure raw{{vec(z)}, vec(nu), false};
  const auto r = x.calc.diff.ito(sq, mu, sigma, raw);
  REQUIRE(r.jump_sizes.sizes()[0] == 2);
  for (std::int64_t i = 0; i < 3; ++i) {
    CHECK(at(r.drift, i) == Approx(at(base.drift, i)));  // raw jumps leave the continuous drift alone
    for (int k = 0; k < 2; ++k) CHECK(at(r.jump_sizes, 3 * k + i) == Approx(2 * kX[i] * z[k] + z[k] * z[k]));
  }

  SeriesJumpMeasure compensated{{vec(z)}, vec(nu), true};
  const auto c = x.calc.diff.ito(sq, mu, sigma, compensated);
  const double sum_nu_z2 = nu[0] * z[0] * z[0] + nu[1] * z[1] * z[1];
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(c.drift, i) == Approx(at(base.drift, i) + sum_nu_z2));
  CHECK(c.jumps_compensated);

  // jumps in a product: both states jump together
  const SeriesItoState y{vec({2.0, 1.5, 0.8}), vec({0.0, 0.0, 0.0}), vec({0.1, 0.1, 0.1})};
  SeriesJumpMeasure joint{{vec({0.2}), vec({-0.1})}, vec({2.0}), false};
  const auto p = x.calc.diff.ito(
      [](const Series& a, const Series& b) { return a.mul(b); }, mu, sigma, y, joint);
  for (std::int64_t i = 0; i < 3; ++i)
    CHECK(at(p.jump_sizes, i) == Approx((kX[i] + 0.2) * (at(y.x, i) - 0.1) - kX[i] * at(y.x, i)));
}

TEST_CASE("misuse is rejected", "[stochastic][ito]") {
  const Series x = vec(kX), mu = vec(kMu), sigma = vec(kSigma);
  const auto sq = [](const Series& s) { return s.mul(s); };
  SeriesJumpMeasure j{{vec({0.1})}, vec({1.0}), false};
  CHECK_THROWS(x.calc.diff.strat(sq, mu, sigma).value.numel() == 0 ||
               x.calc.diff.ito(sq, mu, vec({1.0, 2.0})).value.numel() == 0);  // sigma shape
  CHECK_THROWS(x.calc.diff.ito([](const Series& s) { return s.sum(); }, mu, sigma));  // not elementwise
  // jumps with a Stratonovich call are impossible by signature; an undersized size list is rejected
  SeriesJumpMeasure bad{{}, vec({1.0}), false};
  CHECK_THROWS(x.calc.diff.ito(sq, mu, sigma, bad));
}

namespace {
Series brownian(std::int64_t steps, std::int64_t paths, double t1, std::uint64_t seed) {
  const double h = t1 / static_cast<double>(steps);
  const Series dw = Series::randn({steps, paths}, ScalarType::Float64, seed).mul(std::sqrt(h));
  return Series::zeros({1, paths}, ScalarType::Float64).unite({dw.cumsum(0)}, 0);
}
}  // namespace

TEST_CASE("Ito and Stratonovich integrals of W against W satisfy the exact discrete identities",
          "[stochastic][integral]") {
  const std::int64_t steps = 400, paths = 3;
  const Series w = brownian(steps, paths, 1.0, 7);
  const auto f = [](const Series&, const Series& s) { return s; };
  const Series ito = w.calc.antidiff.ito(f, 0.0, 1.0);
  const Series strat = w.calc.antidiff.strat(f, 0.0, 1.0);
  const Series back = w.calc.antidiff.theta(f, 1.0, 0.0, 1.0);
  const Series dw = w.diff(1, 0);
  const Series qv = dw.mul(dw).sum(0);
  const Series wt = w.select(0, steps);
  REQUIRE(ito.dim() == 1);
  for (std::int64_t p = 0; p < paths; ++p) {
    const double W = at(wt, p), Q = at(qv, p);
    CHECK(at(ito, p) == Approx(0.5 * (W * W - Q)));
    CHECK(at(strat, p) == Approx(0.5 * W * W));
    CHECK(at(back, p) == Approx(0.5 * (W * W + Q)));
    CHECK(at(strat, p) - at(ito, p) == Approx(0.5 * Q));
    CHECK(Q == Approx(1.0).margin(0.35));  // quadratic variation of Brownian motion over [0, 1]
  }
}

TEST_CASE("cumulative integrals start at zero and end at the total", "[stochastic][integral]") {
  const Series w = brownian(100, 2, 1.0, 3);
  const auto f = [](const Series& t, const Series& s) { return s.mul(t).add(1.0); };
  const Series total = w.calc.antidiff.ito(f, 0.0, 1.0);
  const Series running = w.calc.antidiff.ito_cumulative(f, 0.0, 1.0);
  REQUIRE(running.sizes()[0] == 101);
  for (std::int64_t p = 0; p < 2; ++p) {
    CHECK(at(running, p) == Approx(0.0).margin(1e-15));
    CHECK(at(running, 100 * 2 + p) == Approx(at(total, p)));
  }
}

TEST_CASE("an explicit time grid and another time axis", "[stochastic][integral]") {
  const Series w = brownian(50, 2, 2.0, 11);
  const Series times = Series::linspace(0.0, 2.0, 51, ScalarType::Float64).pow(1.3);  // non-uniform grid
  const auto f = [](const Series& t, const Series&) { return t; };
  const Series a = w.calc.antidiff.ito(f, times);
  const Series dw = w.diff(1, 0);
  const Series expect = times.slice(0, 0, 50, 1).reshape({50, 1}).mul(dw).sum(0);
  for (std::int64_t p = 0; p < 2; ++p) CHECK(at(a, p) == Approx(at(expect, p)));
  const Series wt = w.transpose(0, 1).contiguous();  // [paths, samples]
  const Series b = wt.calc.antidiff.ito(f, times, 1);
  for (std::int64_t p = 0; p < 2; ++p) CHECK(at(b, p) == Approx(at(a, p)));
}

TEST_CASE("a Levy path integrates with the Ito sum and the integral is differentiable", "[stochastic][integral]") {
  // compound Poisson path: jumps at fixed steps
  std::vector<double> path(101, 0.0);
  for (int i = 20; i < 101; ++i) path[i] += 0.5;
  for (int i = 60; i < 101; ++i) path[i] -= 0.8;
  const Series x = vec(path);
  const Series one = x.calc.antidiff.ito([](const Series&, const Series& s) { return s.mul(0.0).add(1.0); }, 0.0, 1.0);
  CHECK(at(one, 0) == Approx(-0.3));  // integral of 1 dX is the total increment

  const Series w = brownian(60, 1, 1.0, 5).squeeze(1);
  const Series a = pluss::make_series(0.7).to(ScalarType::Float64);
  const Series grad = a.calc.grad([&](const Series& aa) {
    return w.calc.antidiff.ito([&](const Series&, const Series& s) { return s.mul(aa); }, 0.0, 1.0);
  });
  const Series dw = w.diff(1, 0);
  const Series expected = w.slice(0, 0, 60, 1).mul(dw).sum();
  CHECK(grad.item<double>() == Approx(expected.item<double>()));
}

TEST_CASE("integral misuse is rejected", "[stochastic][integral]") {
  const Series w = brownian(10, 1, 1.0, 1);
  const auto f = [](const Series&, const Series& s) { return s; };
  CHECK_THROWS(w.calc.antidiff.theta(f, 1.5, 0.0, 1.0));
  CHECK_THROWS(w.calc.antidiff.ito(f, 1.0, 0.0));
  CHECK_THROWS(w.calc.antidiff.ito(f, vec({0.0, 0.1, 0.1, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0})));  // not increasing
  CHECK_THROWS(w.calc.antidiff.ito([](const Series&, const Series&) { return vec({1.0, 2.0, 3.0}); }, 0.0, 1.0));  // not broadcastable
  CHECK_NOTHROW(w.calc.antidiff.ito([](const Series&, const Series&) { return pluss::make_series(2.0).to(ScalarType::Float64); }, 0.0, 1.0));  // constant integrand
}

TEST_CASE("the ordinary derivative and Riemann integral calls are unchanged", "[stochastic][calc]") {
  const Series x = vec({2.0, 3.0});
  const Series d = x.calc.diff([](const Series& s) { return s.pow(3.0).sum(); });
  CHECK(at(d, 0) == Approx(12.0));
  const Series d2 = x.calc.diff([](const Series& s) { return s.pow(3.0); }, 2);
  REQUIRE(d2.numel() == 8);  // full second-derivative tensor, shape [2, 2, 2]
  const Series area = vec({1.0, 2.0}).calc.antidiff([](const Series& s) { return s; }, 0.0);
  CHECK(at(area, 0) == Approx(0.5));
  CHECK(at(area, 1) == Approx(2.0));
}

// ---------------------------------------------------------------------------
// Marcus calculus
// ---------------------------------------------------------------------------
TEST_CASE("the Marcus flow follows dy/du = h(y) z", "[stochastic][marcus]") {
  const Series x = vec({0.5, 1.0, -0.3});
  const Series z = pluss::make_series(0.4).to(ScalarType::Float64);
  const Series lin = x.calc.diff.marcus_flow([](const Series& s) { return s; }, z);
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(lin, i) == Approx(kX[i] * std::exp(0.4)).epsilon(1e-9));
  const Series one = x.calc.diff.marcus_flow([](const Series& s) { return s.mul(0.0).add(1.0); }, z);
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(one, i) == Approx(kX[i] + 0.4).epsilon(1e-12));
  // h = 1 + y^2 has the exact flow tan(atan(x) + z)
  const Series tn = x.calc.diff.marcus_flow([](const Series& s) { return s.mul(s).add(1.0); }, z, 64);
  for (std::int64_t i = 0; i < 3; ++i) CHECK(at(tn, i) == Approx(std::tan(std::atan(kX[i]) + 0.4)).epsilon(1e-9));
}

TEST_CASE("Marcus differential: g(X) jumps by g(flow) - g, so the ordinary chain rule holds", "[stochastic][marcus]") {
  const Series x = vec({0.5, 1.0, 2.0}), mu = vec({0.1, 0.2, 0.3}), sigma = vec({0.3, 0.5, 0.2});
  const std::vector<double> z{0.3, -0.2}, nu{1.0, 0.5};
  SeriesJumpMeasure raw{{vec(z)}, vec(nu), false};
  const auto h = [](const Series& s) { return s; };  // multiplicative jumps
  const auto lg = x.calc.diff.marcus([](const Series& s) { return s.log(); }, mu, sigma, h, raw);
  const auto id = x.calc.diff.marcus([](const Series& s) { return s; }, mu, sigma, h, raw);
  for (std::int64_t i = 0; i < 3; ++i) {
    for (int k = 0; k < 2; ++k) {
      CHECK(at(lg.jump_sizes, 3 * k + i) == Approx(z[k]).epsilon(1e-9));  // log X jumps by z exactly
      CHECK(at(id.jump_sizes, 3 * k + i) == Approx(at(x, i) * (std::exp(z[k]) - 1.0)).epsilon(1e-9));
    }
    CHECK(at(id.drift, i) == Approx(at(mu, i)));                       // Stratonovich: no 1/2 sigma^2 term
    CHECK(at(lg.drift, i) == Approx(at(mu, i) / at(x, i)).epsilon(1e-9));
  }
  SeriesJumpMeasure comp{{vec(z)}, vec(nu), true};
  const auto c = x.calc.diff.marcus([](const Series& s) { return s; }, mu, sigma, h, comp);
  for (std::int64_t i = 0; i < 3; ++i) {
    double extra = 0.0;
    for (int k = 0; k < 2; ++k) extra += nu[k] * at(x, i) * (std::exp(z[k]) - 1.0 - z[k]);
    CHECK(at(c.drift, i) == Approx(at(mu, i) + extra).epsilon(1e-9));
  }
  // additive jumps (h = 1): Marcus and Ito agree on the jump sizes
  const auto add = x.calc.diff.marcus([](const Series& s) { return s.mul(s); }, mu, sigma,
                                      [](const Series& s) { return s.mul(0.0).add(1.0); }, raw);
  const auto ito = x.calc.diff.ito([](const Series& s) { return s.mul(s); }, mu, sigma, raw);
  for (std::int64_t i = 0; i < 6; ++i) CHECK(at(add.jump_sizes, i) == Approx(at(ito.jump_sizes, i)).epsilon(1e-9));
}

TEST_CASE("Marcus integral: Newton-Leibniz holds along a path that jumps", "[stochastic][marcus]") {
  // Brownian steps with two jump steps inserted
  Series w = brownian(60, 3, 1.0, 21);
  std::vector<double> dwv = w.diff(1, 0).host<double>().array().data;
  std::vector<double> mask(60 * 3, 0.0);
  for (int p = 0; p < 3; ++p) {
    dwv[20 * 3 + p] = 0.9 - 0.4 * p;
    mask[20 * 3 + p] = 1.0;
    dwv[41 * 3 + p] = -0.7 + 0.2 * p;
    mask[41 * 3 + p] = 1.0;
  }
  const Series dw = pluss::make_series(dwv, {60, 3}).to(ScalarType::Float64);
  const Series path = Series::zeros({1, 3}, ScalarType::Float64).unite({dw.cumsum(0)}, 0);
  const auto f = [](const Series&, const Series& s) { return s; };
  const Series jm = pluss::make_series(mask, {60, 3}).to(ScalarType::Float64);
  const Series marcus = path.calc.antidiff.marcus(f, jm, 0.0, 1.0);
  const Series ito = path.calc.antidiff.ito(f, 0.0, 1.0);
  const Series wt = path.select(0, 60);
  const Series qv = path.antidiff().quadratic_variation(0);
  for (std::int64_t p = 0; p < 3; ++p) {
    CHECK(at(marcus, p) == Approx(0.5 * at(wt, p) * at(wt, p)).epsilon(1e-9));          // ordinary calculus
    CHECK(at(ito, p) == Approx(0.5 * (at(wt, p) * at(wt, p) - at(qv, p))).epsilon(1e-9)); // Ito pays the quadratic variation
  }
  // the threshold form flags the same steps
  const Series by_threshold = path.calc.antidiff.marcus(f, 0.5, 0.0, 1.0);
  for (std::int64_t p = 0; p < 3; ++p) CHECK(at(by_threshold, p) == Approx(at(marcus, p)));
  // a single jump step integrates f along the segment exactly: (w1^3 - w0^3) / 3
  const Series seg = vec({0.3, 1.1});
  const Series one = seg.calc.antidiff.marcus([](const Series&, const Series& s) { return s.mul(s); }, vec({1.0}), 0.0, 1.0);
  CHECK(at(one, 0) == Approx((1.1 * 1.1 * 1.1 - 0.3 * 0.3 * 0.3) / 3.0).epsilon(1e-12));
}

// ---------------------------------------------------------------------------
// array forms, quadratic variation, covariation
// ---------------------------------------------------------------------------
TEST_CASE("integrals of sampled values match the callable forms", "[stochastic][array]") {
  const Series w = brownian(80, 2, 1.0, 5);  // [81, 2]
  const Series f = w.mul(w).add(0.5);        // an adapted integrand sampled on the grid
  const auto fn = [](const Series&, const Series& s) { return s.mul(s).add(0.5); };
  const Series a = f.antidiff(w).ito(0), b = w.calc.antidiff.ito(fn, 0.0, 1.0);
  const Series c = f.antidiff(w).strat(0), d = w.calc.antidiff.strat(fn, 0.0, 1.0);
  const Series e = f.antidiff(w).theta(1.0, 0), g = w.calc.antidiff.theta(fn, 1.0, 0.0, 1.0);
  for (std::int64_t p = 0; p < 2; ++p) {
    CHECK(at(a, p) == Approx(at(b, p)));
    CHECK(at(c, p) == Approx(at(d, p)));
    CHECK(at(e, p) == Approx(at(g, p)));
  }
  // the Ito form needs only the left endpoints
  const Series left_only = f.slice(0, 0, 80, 1);
  for (std::int64_t p = 0; p < 2; ++p) CHECK(at(left_only.antidiff(w).ito(0), p) == Approx(at(a, p)));
  CHECK_THROWS(left_only.antidiff(w).strat(0));
  const Series run = f.antidiff(w).ito_cumulative(0);
  REQUIRE(run.sizes()[0] == 81);
  for (std::int64_t p = 0; p < 2; ++p) CHECK(at(run, 80 * 2 + p) == Approx(at(a, p)));
}

TEST_CASE("quadratic variation and covariation of sampled paths", "[stochastic][array]") {
  const Series w = brownian(400, 2, 1.0, 9);
  const Series v = brownian(400, 2, 1.0, 10);
  const Series qv = w.antidiff().quadratic_variation(0);
  const Series dw = w.diff(1, 0), dv = v.diff(1, 0);
  const Series cov = w.antidiff(v).covariation(0);
  for (std::int64_t p = 0; p < 2; ++p) {
    CHECK(at(qv, p) == Approx(at(dw.mul(dw).sum(0), p)));
    CHECK(at(qv, p) == Approx(1.0).margin(0.3));
    CHECK(at(cov, p) == Approx(at(dw.mul(dv).sum(0), p)));
    CHECK(std::abs(at(cov, p)) < 0.3);  // independent paths: covariation near zero
  }
  const Series run = w.antidiff().quadratic_variation_cumulative(0);
  REQUIRE(run.sizes()[0] == 401);
  CHECK(at(run, 0) == Approx(0.0).margin(1e-15));
  CHECK(at(run, 400 * 2) == Approx(at(qv, 0)));
  // polarization: [w + v] = [w] + [v] + 2 [w, v]
  const Series s = w.add(v);
  CHECK(at(s.antidiff().quadratic_variation(0), 0) ==
        Approx(at(qv, 0) + at(v.antidiff().quadratic_variation(0), 0) + 2.0 * at(cov, 0)).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Malliavin derivative and Skorokhod integral
// ---------------------------------------------------------------------------
TEST_CASE("discrete Malliavin derivative of functionals of the increments", "[stochastic][malliavin]") {
  const std::int64_t N = 12, paths = 3;
  const double h = 1.0 / N;
  const Series dw = Series::randn({N, paths}, ScalarType::Float64, 77).mul(std::sqrt(h));
  const Series wt = dw.sum(0);
  const Series sq = dw.calc.diff.malliavin([](const Series& d) { return d.sum(0).pow(2.0); }, 0);
  const Series ex = dw.calc.diff.malliavin([](const Series& d) { return d.sum(0).exp(); }, 0);
  const Series qv = dw.calc.diff.malliavin([](const Series& d) { return d.mul(d).sum(0); }, 0);
  REQUIRE(sq.sizes()[0] == N);
  for (std::int64_t i = 0; i < N; ++i)
    for (std::int64_t p = 0; p < paths; ++p) {
      CHECK(at(sq, i * paths + p) == Approx(2.0 * at(wt, p)));          // D_t (W_T)^2 = 2 W_T
      CHECK(at(ex, i * paths + p) == Approx(std::exp(at(wt, p))));      // D_t exp(W_T) = exp(W_T)
      CHECK(at(qv, i * paths + p) == Approx(2.0 * at(dw, i * paths + p)));
    }
  // path form: F = sum_j W_j h; each increment i appears in W_j for j > i
  const Series path = Series::zeros({1, paths}, ScalarType::Float64).unite({dw.cumsum(0)}, 0);
  const Series area = path.calc.diff.malliavin_path([h](const Series& w) { return w.sum(0).mul(h); }, 0);
  REQUIRE(area.sizes()[0] == N);
  for (std::int64_t i = 0; i < N; ++i)
    CHECK(at(area, i * paths) == Approx((N - i) * h));  // increment i enters W_{i+1} .. W_N
}

TEST_CASE("Skorokhod integral: anticipating integrands pay the Malliavin correction", "[stochastic][malliavin]") {
  const std::int64_t N = 20, paths = 2;
  const double h = 0.05, T = N * h;
  const Series dw = Series::randn({N, paths}, ScalarType::Float64, 31).mul(std::sqrt(h));
  const double w0 = at(dw.sum(0), 0);
  // u_i = W_T for every i looks into the future: delta(u) = W_T^2 - T
  const Series ant = dw.calc.diff.skorokhod(
      [N](const Series& d) { return d.sum(0).unsqueeze(0).expand({N, d.size(1)}); }, h, 0);
  CHECK(at(ant, 0) == Approx(w0 * w0 - T));
  // an adapted integrand (u_i = W at the left endpoint) gives the Ito sum, with no correction
  const Series adapted = dw.calc.diff.skorokhod(
      [](const Series& d) { return d.cumsum(0).sub(d); }, h, 0);
  const Series path = Series::zeros({1, paths}, ScalarType::Float64).unite({dw.cumsum(0)}, 0);
  const Series ito = path.calc.antidiff.ito([](const Series&, const Series& s) { return s; }, 0.0, T);
  for (std::int64_t p = 0; p < paths; ++p) CHECK(at(adapted, p) == Approx(at(ito, p)));
}

TEST_CASE("a Malliavin weight reproduces the digital-option delta", "[stochastic][malliavin]") {
  // Delta = E[ 1{S_T > K} * delta(u) ] with u_i = 1 / (S0 sigma T): the pathwise derivative of a digital
  // payoff is zero almost everywhere, the Malliavin weight is not.
  const double S0 = 100.0, K = 100.0, r = 0.02, sg = 0.25, T = 1.0;
  const std::int64_t N = 4, paths = 400000;
  const double h = T / N;
  const Series dw = Series::randn({N, paths}, ScalarType::Float64, 2024).mul(std::sqrt(h));
  const Series weight = dw.calc.diff.skorokhod(
      [&](const Series& d) { return d.mul(0.0).add(1.0 / (S0 * sg * T)); }, h, 0);
  const Series wt = dw.sum(0);
  const Series st = wt.mul(sg).add((r - 0.5 * sg * sg) * T).exp().mul(S0);
  const Series payoff = st.gt(K).to(ScalarType::Float64);
  const double delta_mc = payoff.mul(weight).mean().item<double>();
  const double d2 = (std::log(S0 / K) + (r - 0.5 * sg * sg) * T) / (sg * std::sqrt(T));
  const double delta_exact = std::exp(-0.5 * d2 * d2) / std::sqrt(2.0 * M_PI) / (S0 * sg * std::sqrt(T));
  CHECK(delta_mc == Approx(delta_exact).epsilon(0.03));
  // and D_i S_T = sigma S_T for every increment, which is where the weight comes from
  const Series ds = dw.calc.diff.malliavin(
      [&](const Series& d) { return d.sum(0).mul(sg).add((r - 0.5 * sg * sg) * T).exp().mul(S0); }, 0);
  CHECK(at(ds, 0) == Approx(sg * at(st, 0)));
}
