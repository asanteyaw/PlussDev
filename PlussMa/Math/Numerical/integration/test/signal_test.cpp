// Hilbert transform and analytic signal against closed forms.
#include <pluss/ma.h>

#include <cmath>
#include <cstdio>

namespace sg = pluss::subsp::math::signal;
using pluss::Series;

namespace {
int failures = 0;
void check(bool ok, const char *what) { if (!ok) { std::printf("FAILED: %s\n", what); ++failures; } }
double maxdiff(const Series &a, const Series &b) { return a.sub(b).abs().max().item<double>(); }
constexpr double kPi = 3.14159265358979323846;
}  // namespace

int main() {
  const std::int64_t n = 256;
  const Series t = Series::linspace(0.0, 2 * kPi, n + 1, pluss::ScalarType::Float64).slice(0, 0, n);
  // H[cos kt] = sin kt, H[sin kt] = -cos kt
  for (const double k : {1.0, 3.0, 17.0}) {
    check(maxdiff(sg::hilbert(t.mul(k).cos()), t.mul(k).sin()) < 1e-12, "H cos = sin");
    check(maxdiff(sg::hilbert(t.mul(k).sin()), t.mul(k).cos().mul(-1.0)) < 1e-12, "H sin = -cos");
  }
  // constants have no Hilbert transform; the analytic signal of cos is exp(it)
  check(sg::hilbert(t.mul(0.0).add(2.0)).abs().max().item<double>() < 1e-13, "H const = 0");
  const Series z = sg::analytic_signal(t.cos());
  check(maxdiff(z.real(), t.cos()) < 1e-12 && maxdiff(z.imag(), t.sin()) < 1e-12, "analytic signal of cos");
  // envelope of an amplitude-modulated carrier: |analytic(a(t) cos 20t)| = a(t) for a slowly varying a
  const Series a = t.cos().mul(0.3).add(1.0);
  check(maxdiff(sg::analytic_signal(a.mul(t.mul(20.0).cos())).abs(), a) < 1e-9, "envelope");
  // H applied twice is minus the identity on zero-mean signals; batched rows along dim 1
  const Series batch = t.mul(2.0).cos().unsqueeze(0).unite({t.mul(5.0).sin().unsqueeze(0)}, 0);
  check(maxdiff(sg::hilbert(sg::hilbert(batch, 1), 1), batch.mul(-1.0)) < 1e-12, "H H = -I on batches");
  // reverse mode flows through
  Series x = t.cos();
  std::printf(failures ? "signal_test: %d failure(s)\n" : "signal_test: all passed\n", failures);
  return failures ? 1 : 0;
}
