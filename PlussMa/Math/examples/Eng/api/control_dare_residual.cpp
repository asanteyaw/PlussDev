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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::dare(a, b, q, r);
  const pluss::Series &p = sol.value;
  const pluss::Series s = r + b.transpose(0, 1).matmul(p).matmul(b);
  const pluss::Series residual = a.transpose(0, 1).matmul(p).matmul(a) - a.transpose(0, 1).matmul(p).matmul(b).matmul(b.transpose(0, 1).matmul(p).matmul(a)).div(s.flatten().items<double>()[0]) + q - p;
  out.writeln("P =", p.flatten().items<double>());
  out.writeln("largest entry of A'PA - A'PB (R + B'PB)^-1 B'PA + Q - P =", max_abs(residual));
  out.writeln("K =", sol.gain.flatten().items<double>());
}
