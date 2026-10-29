// Eager Lebesgue integration (range partition) and measure_of, against closed forms.
#include <pluss/ma.h>

#include <cmath>
#include <cstdio>

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

namespace {
int failures = 0;
void check(bool ok, const char *what) {
  if (!ok) { std::printf("FAILED: %s\n", what); ++failures; }
}
double item(const Series &s) { return s.reshape({s.numel()}).host<double>()[0]; }
constexpr double kPi = 3.14159265358979323846;
}  // namespace

int main() {
  integ::LebesgueOptions options;
  options.tolerance = 1e-9;

  // smooth integrand: the bracket contains the value and the value matches the closed form
  {
    const auto r = integ::lebesgue([](const Series &x) { return x.sin(); }, 0.0, kPi, options);
    std::printf("v=%.12f lo=%.12f hi=%.12f err=%g ref=%lld\n", item(r.value), item(r.lower), item(r.upper), r.error_estimate, (long long)r.refinements);
    for (double h : r.history) std::printf("  %.12f\n", h);
    check(r.converged, "sin converged");
    check(std::abs(item(r.value) - 2.0) < 1e-6, "int sin = 2");
    check(item(r.lower) <= item(r.value) && item(r.value) <= item(r.upper), "bracket contains value");
    check(item(r.upper) - item(r.lower) < 1e-8, "bracket narrow");
  }
  // signed integrand
  {
    const auto r = integ::lebesgue([](const Series &x) { return x.sub(0.25); }, 0.0, 1.0, options);
    check(std::abs(item(r.value) - 0.25) < 1e-7, "signed int (x - 0.25) = 0.25");
  }
  // batched integrands share the cells: rows a * x^2
  {
    const Series a = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3, 1});
    const auto r = integ::lebesgue([&](const Series &x) { return a.mul(x.square()); }, 0.0, 1.0, options);
    const auto v = r.value.reshape({3}).host<double>();
    check(std::abs(v[0] - 1.0 / 3.0) < 1e-6 && std::abs(v[1] - 2.0 / 3.0) < 1e-6 && std::abs(v[2] - 1.0) < 1e-6, "batched rows");
  }
  // a.e. invariance: a spike on a null set does not move the Lebesgue value, but Simpson nodes hit it
  {
    const auto spike = [](const Series &x) {
      return x.sub(0.5).abs().lt(1e-12).to(pluss::ScalarType::Float64).mul(1e6).add(1.0);
    };
    const auto r = integ::lebesgue(spike, 0.0, 1.0, options);
    std::printf("spike v=%.9f conv=%d err=%g ref=%lld\n", item(r.value), (int)r.converged, r.error_estimate, (long long)r.refinements);
    check(std::abs(item(r.value) - 1.0) < 1e-6, "spike ignored by lebesgue");
    integ::QuadraOptions q;
    q.rule = integ::QuadraRule::Simpson;
    q.adaptive = false;
    q.nodes = 257;
    const auto riemann = integ::quadra(spike, 0.0, 1.0, q);
    check(std::abs(item(riemann.value) - 1.0) > 1.0, "simpson is fooled by the spike");
  }
  // singular integrand with a graded mesh
  {
    integ::LebesgueOptions graded = options;
    graded.grading = 4.0;
    graded.tolerance = 1e-8;
    const auto r = integ::lebesgue([](const Series &x) { return x.rsqrt(); }, 0.0, 1.0, graded);
    std::printf("sing v=%.9f conv=%d err=%g ref=%lld\n", item(r.value), (int)r.converged, r.error_estimate, (long long)r.refinements);
    check(r.converged && std::abs(item(r.value) - 2.0) < 1e-5, "int x^-1/2 = 2");
  }
  // layer-cake identity: int f = int mu(f > t) dt, f = x on [0,1]
  {
    double total = 0.0;
    const int m = 40;
    for (int j = 0; j <= m; ++j) {
      const double t = static_cast<double>(j) / m;
      const auto mu = integ::measure_of([](const Series &x) { return x; }, 0.0, 1.0, t, options);
      const double w = (j == 0 || j == m) ? 1.0 : (j % 2 ? 4.0 : 2.0);
      total += w * item(mu.value);
      check(std::abs(item(mu.value) - (1.0 - t)) < 1e-6, "mu(x > t) = 1 - t");
    }
    check(std::abs(total / (3.0 * m) - 0.5) < 1e-6, "layer-cake identity");
  }
  // measure of a level set for a nonlinear f: x^2 > t on [0,1] is 1 - sqrt(t)
  {
    const auto mu = integ::measure_of([](const Series &x) { return x.square(); }, 0.0, 1.0, 0.36, options);
    check(std::abs(item(mu.value) - 0.4) < 1e-6, "mu(x^2 > 0.36) = 0.4");
  }
  // monotone convergence: min(x^-1/2, n) increases to x^-1/2, integrals 2 - 1/n
  {
    integ::LebesgueOptions graded = options;
    graded.grading = 4.0;
    graded.tolerance = 1e-8;
    double previous = 0.0;
    for (const double n : {1.0, 2.0, 4.0, 8.0}) {
      const auto r = integ::lebesgue([n](const Series &x) { return x.rsqrt().clamp_max(n); }, 0.0, 1.0, graded);
      check(std::abs(item(r.value) - (2.0 - 1.0 / n)) < 1e-5, "int min(x^-1/2, n)");
      check(item(r.value) > previous, "monotone convergence increases");
      previous = item(r.value);
    }
  }
  std::printf(failures ? "lebesgue_test: %d failure(s)\n" : "lebesgue_test: all passed\n", failures);
  return failures ? 1 : 0;
}
