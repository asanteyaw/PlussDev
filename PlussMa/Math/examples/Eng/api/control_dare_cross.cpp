#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series a = mat({1.0, 0.1, 0.0, 1.0}, 2, 2), b = mat({0.005, 0.1}, 2, 1);
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1), n = mat({0.05, 0.02}, 2, 1);
  const ctl::RiccatiResult plain = ctl::dare(a, b, q, r);
  const ctl::RiccatiResult coupled = ctl::dare(a, b, q, r, n);
  out.writeln("gain without a cross term =", plain.gain.flatten().items<double>());
  out.writeln("gain with the cross term  =", coupled.gain.flatten().items<double>());
}
