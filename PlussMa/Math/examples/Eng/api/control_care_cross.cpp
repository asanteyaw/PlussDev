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
  // A cross-weight N couples state and input in the cost x'Qx + 2 x'N u + u'Ru.
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  const pluss::Series r = mat({1.0}, 1, 1);
  const pluss::Series n = mat({0.2, 0.1}, 2, 1);
  const ctl::RiccatiResult plain = ctl::care(A2(), B2(), q, r);
  const ctl::RiccatiResult coupled = ctl::care(A2(), B2(), q, r, n);
  out.writeln("gain without a cross term =", plain.gain.flatten().items<double>());
  out.writeln("gain with the cross term  =", coupled.gain.flatten().items<double>());
  const pluss::Series &p = coupled.value;
  const pluss::Series residual = A2().transpose(0, 1).matmul(p) + p.matmul(A2()) - (p.matmul(B2()) + n).matmul(B2().transpose(0, 1).matmul(p) + n.transpose(0, 1)) + q;
  out.writeln("largest entry of the cross-term Riccati residual =", max_abs(residual));
}
