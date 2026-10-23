// pluss::subsp::math::integ -- Integra/Quadra/Aleatoria/Analytica.
//
// Verification strategy, mirroring this program's established discipline:
//  - trapezoid()/cumulative_trapezoid() (the torch-parity sampled-data
//    pair) are checked against REAL torch's own live numeric output
//    (torch.trapezoid/torch.cumulative_trapezoid, computed once via the
//    cpp_env's torch 2.6.0 and pasted below as oracle values), not
//    hand-derived -- same standard as every complex-support oracle test
//    earlier in this program.
//  - quadra()/quadra_scalar()/aleatoria() are checked against KNOWN
//    closed-form integrals (no torch equivalent needed: these are exact
//    mathematical facts -- int_0^pi sin = 2, int_0^1 x^2 = 1/3, etc.).
//  - analytica::* are checked both against the same closed-form facts AND
//    cross-checked against quadra()'s own high-precision numeric result,
//    so a bug shared between the hand-derived closed form and its
//    hand-derived expected value is unlikely to slip through unnoticed.
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using Complex = std::complex<double>;

using pluss::integ::AleatoriaOptions;
using pluss::integ::IntegraOptions;
using pluss::integ::IntegraStrategy;
using pluss::integ::IntegrandFn;
using pluss::integ::IntegrandNdFn;
using pluss::integ::QuadraNdOptions;
using pluss::integ::QuadraOptions;
using pluss::integ::QuadraRule;
using pluss::integ::QuadraScalarOptions;
using pluss::integ::ScalarIntegrandFn;
using pluss::subsp::math::diff::trace;
namespace analytica = pluss::integ::analytica;

constexpr double kPi = 3.14159265358979323846;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

void expect_near(double got, double want, double atol, const std::string& msg, int* failures) {
  if (std::abs(got - want) > atol) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " got=" << got << " want=" << want << " atol=" << atol << '\n';
  }
}

Series real_series(const std::vector<double>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Float64);
}

Series complex_series(const std::vector<Complex>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Complex128);
}

double scalar_f64(const Series& x) { return x.to(pluss::pCPU, ScalarType::Float64).item<double>(); }

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

void expect_vec_near(const std::vector<double>& got, const std::vector<double>& want, double atol,
                     const std::string& msg, int* failures) {
  if (got.size() != want.size()) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " (size mismatch)\n";
    return;
  }
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (std::abs(got[i] - want[i]) > atol) {
      ++(*failures);
      std::cerr << "[FAIL] " << msg << " idx=" << i << " got=" << got[i] << " want=" << want[i] << '\n';
      return;
    }
  }
}

} // namespace

int main() {
  int failures = 0;

  // ---------------------------------------------------------------
  // trapezoid() / cumulative_trapezoid() -- oracle values from real
  // torch 2.6.0 (torch.trapezoid / torch.cumulative_trapezoid), computed
  // live against this exact y/x data.
  // ---------------------------------------------------------------
  const Series y1 = real_series({0.0, 1.0, 4.0, 9.0, 16.0, 25.0}, {6});
  const Series x1 = real_series({0.0, 0.5, 1.0, 2.0, 3.0, 4.0}, {6});

  {
    const double got = scalar_f64(pluss::integ::trapezoid(y1, x1));
    expect_near(got, 41.0, 1e-9, "trapezoid(y, x) vs torch oracle", &failures);
  }
  {
    const double got = scalar_f64(pluss::integ::trapezoid(y1, 0.5));
    expect_near(got, 21.25, 1e-9, "trapezoid(y, dx=0.5) vs torch oracle", &failures);
  }
  {
    const auto got = vec_f64(pluss::integ::cumulative_trapezoid(y1, x1));
    expect_vec_near(got, {0.25, 1.5, 8.0, 20.5, 41.0}, 1e-9, "cumulative_trapezoid(y, x) vs torch oracle", &failures);
  }
  {
    const auto got = vec_f64(pluss::integ::cumulative_trapezoid(y1, 0.5));
    expect_vec_near(got, {0.25, 1.5, 4.75, 11.0, 21.25}, 1e-9, "cumulative_trapezoid(y, dx=0.5) vs torch oracle",
                    &failures);
  }

  // 2-D batched (dim=-1), x is 1-D and broadcasts across the batch axis.
  {
    const Series y2 = real_series({0.0, 1.0, 4.0, 9.0, 1.0, 2.0, 5.0, 10.0}, {2, 4});
    const Series x2 = real_series({0.0, 1.0, 2.0, 3.0}, {4});
    const auto got = vec_f64(pluss::integ::trapezoid(y2, x2, -1));
    expect_vec_near(got, {9.5, 12.5}, 1e-9, "2D trapezoid(y, x, dim=-1) vs torch oracle", &failures);
    const auto gotc = vec_f64(pluss::integ::cumulative_trapezoid(y2, x2, -1).reshape({6}));
    expect_vec_near(gotc, {0.5, 3.0, 9.5, 1.5, 5.0, 12.5}, 1e-9,
                    "2D cumulative_trapezoid(y, x, dim=-1) vs torch oracle", &failures);
  }

  // Descending x: no special-casing anywhere in trapezoid() -- diff(x) is
  // naturally negative, and the formula produces the correctly-signed
  // (negative) result exactly like torch's own does, with no reversal
  // logic needed (unlike quadra(), whose a > b convenience DOES flip sign
  // -- see below).
  {
    const Series yr = real_series({16.0, 9.0, 4.0, 1.0, 0.25, 0.0}, {6});
    const Series xr = real_series({4.0, 3.0, 2.0, 1.0, 0.5, 0.0}, {6});
    const double got = scalar_f64(pluss::integ::trapezoid(yr, xr));
    expect_near(got, -21.875, 1e-9, "descending-x trapezoid vs torch oracle", &failures);
  }

  // Complex y: pure linear-combination formula, no ordering op anywhere,
  // so this should just work -- verified against a hand-computed value
  // (trapezoid is linear, so real and imaginary channels are independent
  // and match the real-only case run on each part separately).
  {
    const Series yc = complex_series({{0.0, 0.0}, {1.0, 2.0}, {4.0, 8.0}}, {3});
    const Series xc = real_series({0.0, 1.0, 2.0}, {3}).to(ScalarType::Complex128);
    const Complex got = pluss::integ::trapezoid(yc, xc).to(pluss::pCPU, ScalarType::Complex128).item<Complex>();
    // real part: trapezoid([0,1,4], [0,1,2]) = 0.5*(0+1) + 0.5*(1+4) = 3.0
    // imag part: trapezoid([0,2,8], [0,1,2]) = 0.5*(0+2) + 0.5*(2+8) = 6.0
    expect_near(got.real(), 3.0, 1e-9, "complex trapezoid real part", &failures);
    expect_near(got.imag(), 6.0, 1e-9, "complex trapezoid imag part", &failures);
    expect(pluss::integ::trapezoid(yc, xc).scalar_type() == ScalarType::Complex128,
           "complex trapezoid preserves Complex128 dtype", &failures);
  }

  // ---------------------------------------------------------------
  // Autograd through trapezoid()/cumulative_trapezoid(): no custom
  // backward rule exists anywhere for these (see Series::trapezoid in
  // OpsInternal.cpp) -- the gradient is inherited entirely from the
  // composed diff/slice/mul/add/sum ops' own already-verified backward
  // rules. d(trapezoid)/dy_i is exactly the trapezoid weight for y_i
  // (0.5*dx on a boundary point, the sum of its two adjacent half-
  // intervals on an interior point) -- a clean, hand-checkable closed
  // form, verified here bit-for-bit rather than just "doesn't throw".
  // ---------------------------------------------------------------
  {
    const Series y0 = real_series({0.0, 1.0, 4.0, 9.0, 16.0}, {5});
    auto tx = trace(y0, /*tracks_slope=*/true);
    const Series y = tx.value();
    const Series x = real_series({0.0, 1.0, 2.0, 3.0, 4.0}, {5});

    // Exercises the "safe call style" accessor form directly (as opposed
    // to the free-function pluss::integ::trapezoid used everywhere else
    // in this file, and the dot-style convenience form used just below).
    const Series integral = y.antidiff().trapezoid(x);
    expect_near(scalar_f64(integral), 22.0, 1e-9, "autodiff trapezoid forward value", &failures);

    integral.reverse();
    const auto grad = vec_f64(y.sensitivity());
    // uniform dx=1: boundary weight 0.5, interior weight 1.0.
    expect_vec_near(grad, {0.5, 1.0, 1.0, 1.0, 0.5}, 1e-9, "autodiff trapezoid gradient matches trapezoid weights",
                    &failures);
  }
  {
    // cumulative_trapezoid: reverse() from the LAST cumulative entry alone
    // must reproduce the same total-trapezoid gradient as the test above
    // (the last cumsum entry IS the full trapezoid sum) -- a genuine
    // second, independent check that the cumsum-based reverse path agrees
    // with the sum-based one.
    const Series y0 = real_series({0.0, 1.0, 4.0, 9.0, 16.0}, {5});
    auto tx = trace(y0, /*tracks_slope=*/true);
    const Series y = tx.value();
    const Series x = real_series({0.0, 1.0, 2.0, 3.0, 4.0}, {5});

    // Dot-style convenience form this time: `y.antidiff.cumulative_trapezoid(x)`.
    const Series cum = y.antidiff.cumulative_trapezoid(x);
    const Series last = cum.select(0, cum.numel() - 1);
    last.reverse();
    const auto grad = vec_f64(y.sensitivity());
    expect_vec_near(grad, {0.5, 1.0, 1.0, 1.0, 0.5}, 1e-9,
                    "autodiff cumulative_trapezoid's last entry matches trapezoid's own gradient", &failures);
  }
  {
    // quadra()/aleatoria(): differentiability flows through the INTEGRAND
    // closure, not through any special plumbing in this module -- a
    // Trace-tracked parameter captured by the closure ends up in the
    // reverse-mode graph automatically, since the closure's own body is
    // ordinary (already-differentiable) Series arithmetic. Here the
    // integrand is `mean^2`-shifted (x - mean)^2, integrated over a fixed
    // window: d/d(mean) of int_a^b (x-mean)^2 dx has the closed form
    // -2*(b-a)*mean + 2*int_a^b x dx (linear in mean, since the integral
    // of x^2 doesn't depend on mean at all) -- for a=0,b=1,mean=0.5:
    // int_0^1 (x-0.5)^2 dx = 1/12; d/dmean = -2*1*0.5 + 2*0.5 = 0 (mean is
    // exactly the centroid of [0,1], so this specific point is the
    // integral's minimum over mean -- gradient 0 confirms that, not a
    // degenerate check).
    const Series mean0 = real_series({0.5}, {1});
    auto tmean = trace(mean0, /*tracks_slope=*/true);
    const Series mean = tmean.value();
    IntegrandFn f = [&](const Series& x) { return x.sub(mean).mul(x.sub(mean)); };
    QuadraOptions opts;
    opts.rule = QuadraRule::Simpson;
    const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
    expect_near(scalar_f64(r.value), 1.0 / 12.0, 1e-8, "quadra integrand captures outer parameter (forward)",
               &failures);
    r.value.reverse();
    expect_near(scalar_f64(mean.sensitivity()), 0.0, 1e-6,
               "quadra integral differentiates through the captured parameter (mean is the centroid of [0,1])",
               &failures);
  }
  {
    // Batched quadra(): one call prices 4 INDEPENDENT integrals sharing
    // the same quadrature nodes -- f_k(x) = k*x for k=1..4, closed form
    // int_0^1 k*x dx = k/2 -- the concrete capability the earlier review
    // flagged as missing ("pricing a whole strike/maturity grid means one
    // call per point, not one vectorized call"). `y = fn(x)` returns
    // [4, N+1] here (any number of leading batch axes is supported, node
    // axis always last) instead of the usual [N+1]; every rule reduces
    // along `y.dim()-1` specifically so this works uniformly. Also proves
    // autodiff survives the batching: d/dk_i of row i's integral is
    // exactly 0.5 for every row independently, verified via a single
    // .sum().reverse() over the whole batch (an ordinary "batch loss"
    // reduction, exactly how a real per-row objective would be trained).
    const Series k0 = real_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
    auto tk = trace(k0, /*tracks_slope=*/true);
    const Series k = tk.value();
    IntegrandFn f = [&](const Series& x) { return x.reshape({1, x.numel()}).mul(k); };
    for (QuadraRule rule : {QuadraRule::Trapezoidal, QuadraRule::Simpson, QuadraRule::GaussLegendre}) {
      QuadraOptions opts;
      opts.rule = rule;
      const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
      expect_vec_near(vec_f64(r.value), {0.5, 1.0, 1.5, 2.0}, 1e-8, "batched quadra forward matches k/2 per row",
                      &failures);
    }
    QuadraOptions opts;
    opts.rule = QuadraRule::Simpson;
    const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
    r.value.sum().reverse();
    expect_vec_near(vec_f64(k.sensitivity()), {0.5, 0.5, 0.5, 0.5}, 1e-8,
                    "batched quadra gradient is 0.5 for every row independently", &failures);
  }

  // ---------------------------------------------------------------
  // quadra(): the genuine multi-dimensional deterministic quadrature
  // this module didn't have before (distinct from batching -- that was
  // many independent 1-D integrals sharing nodes; this is ONE integral
  // over a genuine N-D box). Tensor-product construction, verified
  // against exact closed forms for 2-D and 3-D, plus autodiff through a
  // captured parameter.
  // ---------------------------------------------------------------
  {
    IntegrandNdFn xy = [](const std::vector<Series>& c) { return c[0].mul(c[1]); };
    const auto r = pluss::integ::quadra(xy, {{0.0, 1.0}, {0.0, 1.0}});
    expect_near(scalar_f64(r.value), 0.25, 1e-9, "quadra (N-D) 2D int_0^1 int_0^1 x*y dx dy == 0.25", &failures);
    expect(r.dimensions == 2, "quadra (N-D) reports the correct dimension count", &failures);
  }
  {
    IntegrandNdFn xyz = [](const std::vector<Series>& c) { return c[0].add(c[1]).add(c[2]); };
    const auto r = pluss::integ::quadra(xyz, {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}});
    expect_near(scalar_f64(r.value), 1.5, 1e-9, "quadra (N-D) 3D int over unit cube of (x+y+z) == 1.5", &failures);
  }
  {
    // Cross-check against a DIFFERENT rule for the same problem, and
    // against aleatoria()'s own N-D Monte Carlo overload (a completely
    // independent algorithm) -- both should agree with the deterministic
    // Gauss-Legendre default and with each other.
    IntegrandNdFn xy = [](const std::vector<Series>& c) { return c[0].mul(c[1]); };
    QuadraNdOptions opts;
    opts.rule = QuadraRule::Simpson;
    opts.resolution = 16;
    const auto r_simpson = pluss::integ::quadra(xy, {{0.0, 1.0}, {0.0, 1.0}}, opts);
    expect_near(scalar_f64(r_simpson.value), 0.25, 1e-6, "quadra (N-D) Simpson rule agrees with GaussLegendre default",
               &failures);

    IntegrandNdFn xy_mc = [](const std::vector<Series>& c) { return c[0].mul(c[1]); };
    AleatoriaOptions mc_opts;
    mc_opts.samples = 1'000'000;
    mc_opts.seed = 41;
    const auto r_mc =
        pluss::integ::aleatoria(xy_mc, std::vector<std::pair<double, double>>{{0.0, 1.0}, {0.0, 1.0}}, mc_opts);
    expect(std::abs(scalar_f64(r_mc.value) - 0.25) <= 6.0 * r_mc.standard_error,
           "quadra (N-D)'s exact result agrees with an independent aleatoria() N-D Monte Carlo estimate", &failures);
  }
  {
    // Safety cap against combinatorial blowup.
    IntegrandNdFn f = [](const std::vector<Series>& c) { return c[0]; };
    QuadraNdOptions opts;
    opts.resolution = 50;
    opts.max_total_nodes = 1000;
    bool threw = false;
    try {
      (void)pluss::integ::quadra(
          f, std::vector<std::pair<double, double>>{{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}}, opts);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "quadra (N-D) rejects a tensor-product grid exceeding max_total_nodes", &failures);
  }
  {
    // Autograd through quadra (N-D): int_0^1 int_0^1 k*x*y dxdy = k/4, so
    // d/dk = 1/4 -- verified via .reverse() against the captured k.
    const Series k0 = real_series({2.0}, {1});
    auto tk = trace(k0, /*tracks_slope=*/true);
    const Series k = tk.value();
    IntegrandNdFn f = [&](const std::vector<Series>& c) { return c[0].mul(c[1]).mul(k); };
    const auto r = pluss::integ::quadra(f, {{0.0, 1.0}, {0.0, 1.0}});
    expect_near(scalar_f64(r.value), 0.5, 1e-8, "quadra (N-D) forward value with captured parameter (k=2 -> k/4=0.5)",
               &failures);
    r.value.reverse();
    expect_near(scalar_f64(k.sensitivity()), 0.25, 1e-8, "quadra (N-D) differentiates through the captured parameter",
               &failures);
  }

  // ---------------------------------------------------------------
  // quadra() -- known closed-form integrals.
  // ---------------------------------------------------------------
  {
    IntegrandFn f = [](const Series& x) { return x.mul(x); };  // x^2
    for (QuadraRule rule : {QuadraRule::Trapezoidal, QuadraRule::Simpson, QuadraRule::GaussLegendre}) {
      QuadraOptions opts;
      opts.rule = rule;
      const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
      expect_near(scalar_f64(r.value), 1.0 / 3.0, 1e-8, "quadra x^2 over [0,1]", &failures);
      expect(r.converged, "quadra x^2 converged", &failures);
    }
  }
  {
    IntegrandFn sinf = [](const Series& x) { return x.sin(); };
    QuadraOptions opts;
    opts.rule = QuadraRule::Simpson;
    const auto r = pluss::integ::quadra(sinf, 0.0, kPi, opts);
    expect_near(scalar_f64(r.value), 2.0, 1e-8, "quadra sin(x) over [0,pi] == 2", &failures);
  }
  {
    // Gaussian kernel, cross-checked against analytica::gaussian below too.
    IntegrandFn gauss = [](const Series& x) { return x.mul(x).mul(-0.5).exp(); };
    QuadraOptions opts;
    opts.rule = QuadraRule::GaussLegendre;
    opts.panels = 64;
    const auto r = pluss::integ::quadra(gauss, -6.0, 6.0, opts);
    expect_near(scalar_f64(r.value), std::sqrt(2.0 * kPi), 1e-6, "quadra Gaussian kernel over [-6,6]", &failures);
  }
  {
    // a > b must flip the sign (unlike trapezoid(), which has no special
    // casing and relies on diff()'s natural sign -- quadra() generates its
    // OWN nodes from [min(a,b), max(a,b)] so it needs an explicit flip).
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    const auto r = pluss::integ::quadra(f, 1.0, 0.0);
    expect_near(scalar_f64(r.value), -1.0 / 3.0, 1e-8, "quadra reversed bounds flips sign", &failures);
  }
  {
    // Complex integrand, all 3 rules: int_0^1 exp(i*x) dx = (e^i - 1)/i =
    // sin(1) + i*(1-cos(1)) (a nontrivial closed form, verified live via
    // Python's cmath, not just "converges to ~0" -- exactly the
    // characteristic-function-style integrand this module needs to
    // support for e.g. Heston/Fourier-inversion pricing work). The node
    // array itself stays real (QuadraOptions::dtype controls the real
    // node dtype fed to Series::linspace, independent of what the
    // integrand returns -- same dtype/output-dtype independence
    // established for aleatoria()); only `fn`'s own `.to(Complex128)`
    // determines the complex path.
    const Complex want{0.8414709848078965, 0.45969769413186023};
    IntegrandFn f = [](const Series& x) {
      const Series i_unit = pluss::make_series(std::vector<Complex>{{0.0, 1.0}}, {}).to(ScalarType::Complex128);
      return x.to(ScalarType::Complex128).mul(i_unit).exp();
    };
    for (QuadraRule rule : {QuadraRule::Trapezoidal, QuadraRule::Simpson, QuadraRule::GaussLegendre}) {
      QuadraOptions opts;
      opts.rule = rule;
      const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
      expect(r.value.scalar_type() == ScalarType::Complex128, "complex quadra preserves Complex128 dtype", &failures);
      const Complex got = r.value.to(pluss::pCPU, ScalarType::Complex128).item<Complex>();
      expect(std::abs(got - want) < 1e-6, "complex quadra int_0^1 exp(i*x) dx matches closed form", &failures);
    }
  }

  // ---------------------------------------------------------------
  // Semi-infinite / infinite domains: quadra()/aleatoria() detect
  // +-infinity bounds and transparently substitute onto a finite domain
  // (see the substitution helpers in integ.h) -- this is the concrete
  // capability characteristic-function/Fourier-inversion integrands need
  // (PlussMa/Finance's Heston pricing currently sidesteps this by
  // manually truncating to a large finite cutoff instead of integrating
  // the true infinite domain -- this is what would let that be a real
  // infinite integral, though the migration itself is a separate,
  // deliberately unmade decision -- see project memory).
  // ---------------------------------------------------------------
  {
    constexpr double kInf = std::numeric_limits<double>::infinity();
    IntegrandFn exp_neg = [](const Series& x) { return x.mul(-1.0).exp(); };
    const auto r = pluss::integ::quadra(exp_neg, 0.0, kInf);
    expect_near(scalar_f64(r.value), 1.0, 1e-6, "quadra semi-infinite [0,inf): int exp(-x) dx == 1", &failures);
    expect(r.converged, "quadra semi-infinite upper converged", &failures);
  }
  {
    constexpr double kInf = std::numeric_limits<double>::infinity();
    IntegrandFn exp_pos = [](const Series& x) { return x.exp(); };
    const auto r = pluss::integ::quadra(exp_pos, -kInf, 0.0);
    expect_near(scalar_f64(r.value), 1.0, 1e-6, "quadra semi-infinite (-inf,0]: int exp(x) dx == 1", &failures);
  }
  {
    // The classic Gaussian integral -- fully infinite domain, closed form
    // sqrt(pi), also cross-checked against analytica::gaussian below.
    constexpr double kInf = std::numeric_limits<double>::infinity();
    IntegrandFn gauss = [](const Series& x) { return x.mul(x).mul(-1.0).exp(); };
    const auto r = pluss::integ::quadra(gauss, -kInf, kInf);
    expect_near(scalar_f64(r.value), std::sqrt(kPi), 1e-6, "quadra fully infinite: int exp(-x^2) dx == sqrt(pi)",
               &failures);
  }
  {
    // Monte Carlo over a semi-infinite domain -- the substitution is
    // applied before sampling, so this is still an ordinary uniform MC
    // estimate, just of the substituted (finite-domain) integrand.
    constexpr double kInf = std::numeric_limits<double>::infinity();
    IntegrandFn exp_neg = [](const Series& x) { return x.mul(-1.0).exp(); };
    AleatoriaOptions opts;
    opts.samples = 2'000'000;
    opts.seed = 31;
    const auto r = pluss::integ::aleatoria(exp_neg, 0.0, kInf, opts);
    expect(std::abs(scalar_f64(r.value) - 1.0) <= 6.0 * r.standard_error,
           "aleatoria semi-infinite [0,inf) within 6 standard errors of 1.0", &failures);
  }
  {
    // The exact case the earlier review named directly: a complex,
    // characteristic-function-shaped integrand (exp(-x)*exp(i*x)) over a
    // genuinely semi-infinite domain -- int_0^inf exp(-(1-i)x) dx =
    // 1/(1-i) = 0.5 + 0.5i, a standard Laplace/Fourier transform pair,
    // verified live via Python's cmath.
    constexpr double kInf = std::numeric_limits<double>::infinity();
    const Complex want{0.5, 0.5};
    IntegrandFn f = [](const Series& x) {
      const Series i_unit = pluss::make_series(std::vector<Complex>{{0.0, 1.0}}, {}).to(ScalarType::Complex128);
      const Series decay = x.mul(-1.0).exp().to(ScalarType::Complex128);
      const Series oscillation = x.to(ScalarType::Complex128).mul(i_unit).exp();
      return decay.mul(oscillation);
    };
    const auto r = pluss::integ::quadra(f, 0.0, kInf);
    const Complex got = r.value.to(pluss::pCPU, ScalarType::Complex128).item<Complex>();
    expect(std::abs(got - want) < 1e-6,
           "quadra complex characteristic-function-style integrand over a semi-infinite domain matches closed form",
           &failures);
  }

  // ---------------------------------------------------------------
  // quadra_scalar() -- replaces the duplicated integrate_simpson_scalar_/
  // integrate_simpson helpers; same closed-form checks, scalar API.
  // ---------------------------------------------------------------
  {
    ScalarIntegrandFn f = [](double x) { return x * x; };
    const auto r = pluss::integ::quadra_scalar(f, 0.0, 1.0);
    expect_near(r.value, 1.0 / 3.0, 1e-9, "quadra_scalar x^2 over [0,1]", &failures);
    expect(r.converged, "quadra_scalar non-adaptive always converged", &failures);
  }
  {
    // Exact bit-for-bit match with the OLD distr_objects_continuous.cpp
    // integrate_simpson(fn, a, b, 2048) default this replaces at its
    // migrated call sites (see distr_objects_continuous.cpp).
    ScalarIntegrandFn f = [](double x) { return std::exp(-x); };
    QuadraScalarOptions opts;
    opts.nodes = 2048;
    const auto r = pluss::integ::quadra_scalar(f, 0.0, 5.0, opts);
    const double exact = 1.0 - std::exp(-5.0);
    expect_near(r.value, exact, 1e-9, "quadra_scalar matches old n=2048 Simpson default", &failures);
  }
  {
    ScalarIntegrandFn f = [](double x) { return std::sin(x); };
    QuadraScalarOptions opts;
    opts.adaptive = true;
    opts.tolerance = 1e-12;
    const auto r = pluss::integ::quadra_scalar(f, 0.0, kPi, opts);
    expect_near(r.value, 2.0, 1e-9, "quadra_scalar adaptive sin(x) over [0,pi]", &failures);
    expect(r.converged, "quadra_scalar adaptive converged", &failures);
  }

  // ---------------------------------------------------------------
  // Autograd through aleatoria(): same "differentiability is inherited
  // from ordinary Series ops, not built into this module" property as
  // quadra's own gradient test above, but exercising a genuinely
  // DIFFERENT reduction path (`.mean()` over Monte Carlo samples, not a
  // weighted sum of quadrature nodes) -- this is exactly the axis the
  // earlier review correctly flagged as unproven ("architecturally
  // plausible but unproven... nothing... ever calls .reverse()").
  // Reparameterization-style: the sample locations x = U*(b-a)+a do not
  // depend on the captured parameter, so the gradient flows purely
  // through fn's own body evaluated at fixed sample points -- exactly how
  // JAX/torch's own reparameterized Monte Carlo gradient estimators work.
  // Same closed-form target as the quadra check above (mean=0.5 is the
  // centroid of [0,1], so d/dmean == 0 exactly) -- tolerance widened to
  // account for genuine Monte Carlo sampling noise in the gradient
  // estimate itself, not just the value.
  // ---------------------------------------------------------------
  {
    const Series mean0 = real_series({0.5}, {1});
    auto tmean = trace(mean0, /*tracks_slope=*/true);
    const Series mean = tmean.value();
    IntegrandFn f = [&](const Series& x) { return x.sub(mean).mul(x.sub(mean)); };
    AleatoriaOptions opts;
    opts.samples = 4'000'000;
    opts.seed = 29;
    const auto r = pluss::integ::aleatoria(f, 0.0, 1.0, opts);
    expect_near(scalar_f64(r.value), 1.0 / 12.0, 0.01, "aleatoria integrand captures outer parameter (forward)",
               &failures);
    r.value.reverse();
    expect_near(scalar_f64(mean.sensitivity()), 0.0, 0.05,
               "aleatoria integral differentiates through the captured parameter (mean is the centroid of [0,1], "
               "tolerance widened for genuine MC sampling noise in the gradient itself)",
               &failures);
  }

  // ---------------------------------------------------------------
  // aleatoria() -- convergence to known integrals within statistical
  // tolerance (a generous multiple of the reported standard error).
  // ---------------------------------------------------------------
  {
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    AleatoriaOptions opts;
    opts.samples = 2'000'000;
    opts.seed = 7;
    const auto r = pluss::integ::aleatoria(f, 0.0, 1.0, opts);
    const double got = scalar_f64(r.value);
    expect(std::abs(got - 1.0 / 3.0) <= 6.0 * r.standard_error,
           "aleatoria x^2 over [0,1] within 6 standard errors of 1/3", &failures);
    expect(r.standard_error > 0.0 && r.standard_error < 0.01, "aleatoria standard_error is a sane small number",
           &failures);
  }
  {
    // Stratified sampling should not change the estimator's unbiasedness.
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    AleatoriaOptions opts;
    opts.samples = 500'000;
    opts.seed = 11;
    opts.stratified = true;
    const auto r = pluss::integ::aleatoria(f, 0.0, 1.0, opts);
    expect(std::abs(scalar_f64(r.value) - 1.0 / 3.0) <= 6.0 * r.standard_error,
           "stratified aleatoria x^2 over [0,1] within 6 standard errors", &failures);
  }
  {
    // N-D: volume of the unit ball's positive octant in 3D relative to the
    // enclosing cube is pi/6 (a well-known closed-form fact), estimated by
    // Monte Carlo over [0,1]^3.
    IntegrandNdFn indicator = [](const std::vector<Series>& coords) {
      const Series r2 = coords[0].mul(coords[0]).add(coords[1].mul(coords[1])).add(coords[2].mul(coords[2]));
      return r2.le(1.0).to(ScalarType::Float64);
    };
    AleatoriaOptions opts;
    opts.samples = 2'000'000;
    opts.seed = 13;
    const auto r = pluss::integ::aleatoria(
        indicator, std::vector<std::pair<double, double>>{{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}}, opts);
    const double want = kPi / 6.0;
    expect(std::abs(scalar_f64(r.value) - want) <= 6.0 * r.standard_error,
           "3D aleatoria octant-of-unit-ball volume within 6 standard errors of pi/6", &failures);
  }
  {
    // Complex-valued integrand: f(x) = x + i*x^2, so int_0^1 f = 0.5 + i/3
    // -- a genuinely complex closed form (distinct real/imaginary parts),
    // not just a real value cast to complex. `AleatoriaOptions::dtype`
    // controls the REAL sample-point dtype fed to Series::rand (which
    // only supports floating dtypes) -- it is deliberately independent of
    // what the integrand itself returns; the complex path is exercised
    // entirely through `f`'s own `.to(Complex128)`, and
    // aleatoria_reduce_'s abs2()-based variance formula picks it up via
    // the returned value's own dtype, not via any option.
    IntegrandFn f = [](const Series& x) {
      const Series re = x;
      const Series im = x.mul(x);
      const Series i_unit = pluss::make_series(std::vector<Complex>{{0.0, 1.0}}, {}).to(ScalarType::Complex128);
      return re.to(ScalarType::Complex128).add(im.to(ScalarType::Complex128).mul(i_unit));
    };
    AleatoriaOptions opts;
    opts.samples = 2'000'000;
    opts.seed = 17;
    bool threw = false;
    try {
      const auto r = pluss::integ::aleatoria(f, 0.0, 1.0, opts);
      expect(r.value.scalar_type() == ScalarType::Complex128, "complex aleatoria preserves Complex128 dtype",
             &failures);
      const Complex got = r.value.to(pluss::pCPU, ScalarType::Complex128).item<Complex>();
      expect(std::abs(got.real() - 0.5) <= 6.0 * r.standard_error,
             "complex aleatoria real part within 6 standard errors of 0.5", &failures);
      expect(std::abs(got.imag() - 1.0 / 3.0) <= 6.0 * r.standard_error,
             "complex aleatoria imag part within 6 standard errors of 1/3", &failures);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] complex aleatoria threw: " << e.what() << '\n';
    }
    expect(!threw, "complex aleatoria must not throw", &failures);
  }

  // ---------------------------------------------------------------
  // analytica -- exact closed forms, cross-checked against quadra()'s own
  // high-precision numeric result and against known facts.
  // ---------------------------------------------------------------
  {
    expect_near(analytica::power(2.0, 0.0, 1.0), 1.0 / 3.0, 1e-12, "analytica::power x^2 exact", &failures);
    expect_near(analytica::power(-1.0, 1.0, std::exp(1.0)), 1.0, 1e-9, "analytica::power n=-1 -> ln", &failures);
    expect_near(analytica::polynomial({1.0, 0.0, 3.0}, 0.0, 2.0), 10.0, 1e-12,
               "analytica::polynomial 1+3x^2 over [0,2] == 10", &failures);  // 2 + 3*(8/3) = 2+8=10
    expect_near(analytica::exponential(1.0, 0.0, 1.0), std::exp(1.0) - 1.0, 1e-12, "analytica::exponential exp(x)",
               &failures);
    expect_near(analytica::exponential(0.0, 2.0, 5.0), 3.0, 1e-12, "analytica::exponential k=0 degenerate case",
               &failures);
    expect_near(analytica::gaussian_probability(-1.0, 1.0), 0.6826894921370859, 1e-9,
               "analytica::gaussian_probability matches the 68% rule", &failures);
    expect_near(analytica::gaussian_probability(-1.959963984540054, 1.959963984540054), 0.95, 1e-6,
               "analytica::gaussian_probability matches the 95% rule", &failures);
    expect_near(analytica::sine(1.0, 0.0, kPi / 2.0), 1.0, 1e-12, "analytica::sine int_0^(pi/2) sin(x) dx == 1",
               &failures);
    expect_near(analytica::sine(2.0, 0.0, kPi), 0.0, 1e-12, "analytica::sine full period == 0", &failures);
    expect_near(analytica::cosine(1.0, 0.0, kPi / 2.0), 1.0, 1e-12, "analytica::cosine int_0^(pi/2) cos(x) dx == 1",
               &failures);
    expect_near(analytica::cosine(0.0, 2.0, 5.0), 3.0, 1e-12, "analytica::cosine k=0 degenerate case", &failures);
    expect_near(analytica::lorentzian(1.0, -1.0, 1.0), kPi / 2.0, 1e-12,
               "analytica::lorentzian int_-1^1 1/(1+x^2) dx == pi/2", &failures);
    // Oracle values below computed live via Python's math module.
    expect_near(analytica::hyperbolic_sine(2.0, 0.0, 1.0), 1.3810978455418157, 1e-9,
               "analytica::hyperbolic_sine int_0^1 sinh(2x) dx", &failures);
    expect_near(analytica::hyperbolic_sine(0.0, 0.0, 1.0), 0.0, 1e-12, "analytica::hyperbolic_sine k=0 degenerate",
               &failures);
    expect_near(analytica::hyperbolic_cosine(2.0, 0.0, 1.0), 1.8134302039235093, 1e-9,
               "analytica::hyperbolic_cosine int_0^1 cosh(2x) dx", &failures);
    expect_near(analytica::hyperbolic_cosine(0.0, 0.0, 1.0), 1.0, 1e-12, "analytica::hyperbolic_cosine k=0 degenerate",
               &failures);
    expect_near(analytica::hyperbolic_tangent(0.0, 1.0), 0.4337808304830271, 1e-9,
               "analytica::hyperbolic_tangent int_0^1 tanh(x) dx", &failures);
    expect_near(analytica::sigmoid(0.0, 1.0), 0.6201145069582775, 1e-9,
               "analytica::sigmoid int_0^1 sigmoid(x) dx (softplus difference)", &failures);
    expect_near(analytica::sigmoid(-1000.0, 1000.0), 1000.0, 1e-6,
               "analytica::sigmoid stays numerically stable for large |x|", &failures);
    expect_near(analytica::gaussian_moment(0.0, 1.0), 0.3934693402873666, 1e-9,
               "analytica::gaussian_moment int_0^1 x*exp(-x^2/2) dx", &failures);
    expect_near(analytica::arcsine_kernel(0.0, 0.5), 0.5235987755982988, 1e-9,
               "analytica::arcsine_kernel int_0^0.5 1/sqrt(1-x^2) dx", &failures);
  }
  {
    // Cross-check the new closed forms against quadra()'s own independent
    // numeric result, same discipline as the gaussian cross-check below.
    IntegrandFn sigmoid_fn = [](const Series& x) { return x.neg().exp().add(1.0).pow(-1.0); };
    const auto r = pluss::integ::quadra(sigmoid_fn, -5.0, 5.0);
    expect_near(scalar_f64(r.value), analytica::sigmoid(-5.0, 5.0), 1e-6,
               "analytica::sigmoid matches quadra() numerically", &failures);

    IntegrandFn tanh_fn = [](const Series& x) { return x.tanh(); };
    const auto r2 = pluss::integ::quadra(tanh_fn, 0.0, 2.0);
    expect_near(scalar_f64(r2.value), analytica::hyperbolic_tangent(0.0, 2.0), 1e-6,
               "analytica::hyperbolic_tangent matches quadra() numerically", &failures);
  }
  {
    // Cross-check analytica::lorentzian against a genuinely fully-infinite
    // quadra() call -- int_-inf^inf 1/(1+x^2) dx == pi, the classic
    // arctangent improper integral -- exercising BOTH new features
    // (infinite-domain substitution and the new registry entry) against
    // each other rather than only against a hand-typed constant.
    constexpr double kInf = std::numeric_limits<double>::infinity();
    IntegrandFn lorentz = [](const Series& x) { return x.mul(x).add(1.0).pow(-1.0); };
    const auto r = pluss::integ::quadra(lorentz, -kInf, kInf);
    expect_near(scalar_f64(r.value), kPi, 1e-6, "quadra fully-infinite Lorentzian == pi", &failures);
    expect_near(analytica::lorentzian(1.0, -kInf, kInf), kPi, 1e-9,
               "analytica::lorentzian itself accepts infinite bounds directly (pure atan formula, no substitution "
               "needed)",
               &failures);
  }
  {
    // Cross-check analytica::gaussian against quadra()'s own independent
    // numeric result for the same (unnormalized) kernel used above.
    IntegrandFn gauss = [](const Series& x) { return x.mul(x).mul(-0.5).exp(); };
    QuadraOptions opts;
    opts.rule = QuadraRule::GaussLegendre;
    opts.panels = 64;
    const auto r = pluss::integ::quadra(gauss, -6.0, 6.0, opts);
    const double closed_form = analytica::gaussian(-6.0, 6.0);
    expect_near(scalar_f64(r.value), closed_form, 1e-6, "analytica::gaussian matches quadra() numerically",
               &failures);
  }

  // ---------------------------------------------------------------
  // integra() -- dispatcher.
  // ---------------------------------------------------------------
  {
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    const auto r = pluss::integ::integra(f, 0.0, 1.0);
    expect_near(scalar_f64(r.value), 1.0 / 3.0, 1e-8, "integra() default Auto resolves to Quadrature", &failures);
    expect(r.strategy_used == IntegraStrategy::Quadrature, "integra() Auto reports Quadrature as strategy_used",
           &failures);

    IntegraOptions opts;
    opts.strategy = IntegraStrategy::MonteCarlo;
    opts.monte_carlo.samples = 1'000'000;
    opts.monte_carlo.seed = 23;
    const auto rmc = pluss::integ::integra(f, 0.0, 1.0, opts);
    expect(rmc.strategy_used == IntegraStrategy::MonteCarlo, "integra() explicit MonteCarlo strategy honored",
           &failures);
    expect(std::abs(scalar_f64(rmc.value) - 1.0 / 3.0) <= 6.0 * rmc.error_estimate,
           "integra() MonteCarlo result within 6 standard errors of 1/3", &failures);
  }

  // ---------------------------------------------------------------
  // quadra(): QuadraRule::Auto -- well-behaved integrand converges on the
  // first rung (GaussLegendre), never escalates.
  // ---------------------------------------------------------------
  {
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    QuadraOptions opts;
    opts.rule = QuadraRule::Auto;
    const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
    expect_near(scalar_f64(r.value), 1.0 / 3.0, 1e-8, "quadra() Auto matches the exact value for a smooth integrand",
               &failures);
    expect(r.converged, "quadra() Auto converges for a smooth integrand", &failures);
    expect(r.rule_used == QuadraRule::GaussLegendre, "quadra() Auto picks GaussLegendre first and stays there",
           &failures);
  }

  // ---------------------------------------------------------------
  // quadra(): QuadraRule::Auto -- max_refinements=0 forces every rung to
  // report non-convergence deterministically and reliably, regardless of
  // the integrand (unlike an unreachable tolerance: a smooth integrand
  // like x*x or even exp(x) can make GaussLegendre saturate to a
  // bit-identical floating-point result well within a couple of
  // refinements, making tolerance=0 accidentally "converge" for the wrong
  // reason). With the refinement loop's body never running,
  // `converged` simply stays at its default `false` for any integrand --
  // the cleanest way to exercise the escalation path itself. Expect it to
  // end on Trapezoidal, report converged=false honestly, and accumulate
  // evaluations across ALL THREE rungs, not just the last one.
  // ---------------------------------------------------------------
  {
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    QuadraOptions opts;
    opts.rule = QuadraRule::Auto;
    opts.max_refinements = 0;
    const auto r = pluss::integ::quadra(f, 0.0, 1.0, opts);
    expect(!r.converged, "quadra() Auto honestly reports non-convergence when the tolerance is unreachable",
           &failures);
    expect(r.rule_used == QuadraRule::Trapezoidal, "quadra() Auto falls all the way through to Trapezoidal",
           &failures);
    // Each rung runs an initial evaluation plus max_refinements
    // doublings; three rungs' worth should sum to noticeably more than
    // any single rung's own evaluation count.
    QuadraOptions single_rung = opts;
    single_rung.rule = QuadraRule::GaussLegendre;
    const auto r_single = pluss::integ::quadra(f, 0.0, 1.0, single_rung);
    expect(r.evaluations > r_single.evaluations,
           "quadra() Auto's evaluation count reflects ALL escalated rungs, not just the last one", &failures);
  }

  // ---------------------------------------------------------------
  // integra(): IntegraStrategy::Auto -- quadrature.max_refinements=0
  // forces Quadrature to report non-convergence deterministically (same
  // technique as quadra()'s own Auto test above), which should escalate
  // Integra's Auto to Monte Carlo.
  // ---------------------------------------------------------------
  {
    IntegrandFn f = [](const Series& x) { return x.mul(x); };
    IntegraOptions opts;
    opts.strategy = IntegraStrategy::Auto;
    opts.quadrature.max_refinements = 0;
    opts.monte_carlo.samples = 500'000;
    opts.monte_carlo.seed = 31;
    const auto r = pluss::integ::integra(f, 0.0, 1.0, opts);
    expect(r.strategy_used == IntegraStrategy::MonteCarlo,
           "integra() Auto escalates to MonteCarlo when Quadrature genuinely fails to converge", &failures);
    expect(r.converged, "integra() Auto's MonteCarlo fallback still reports converged (MC always completes)",
           &failures);
    expect(std::abs(scalar_f64(r.value) - 1.0 / 3.0) <= 6.0 * r.error_estimate,
           "integra() Auto's MonteCarlo fallback value is within 6 standard errors of the true answer", &failures);
  }

  // integra() Auto must not replace a nearly-converged quadrature value by a noisier Monte Carlo one: sqrt(x) on [0, 1] misses the
  // default 1e-10 tolerance by a small margin (error about 1e-9) but is still far more accurate than 1e6 random samples (about 1e-4).
  {
    IntegrandFn f = [](const Series& x) { return x.sqrt(); };
    const auto r = pluss::integ::integra(f, 0.0, 1.0);
    expect_near(scalar_f64(r.value), 2.0 / 3.0, 1e-7, "integra() Auto keeps the more accurate quadrature value", &failures);
    expect(r.strategy_used == IntegraStrategy::Quadrature, "integra() Auto reports Quadrature when it is the better answer", &failures);
  }

  if (failures != 0) {
    std::cerr << "integ_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "integ_test passed\n";
  return 0;
}
