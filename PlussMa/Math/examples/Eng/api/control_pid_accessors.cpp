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
  const auto s = [](double v) { return pluss::make_series(v); };
  const ctl::Pid c = ctl::pid(s(2.0), s(1.0), s(0.5), s(0.1), 0.02, ctl::PidRule::Trapezoid, ctl::PidRule::Backward);
  out.writeln("kp", c.kp().item<double>(), "ki", c.ki().item<double>(), "kd", c.kd().item<double>(), "filter", c.filter().item<double>());
  out.writeln("sample time", c.sample_time(), " discrete:", c.is_disc(), " continuous:", c.is_cont());
  out.writeln("integral rule is Trapezoid:", c.int_rule() == ctl::PidRule::Trapezoid, " derivative rule is Backward:", c.diff_rule() == ctl::PidRule::Backward);
}
