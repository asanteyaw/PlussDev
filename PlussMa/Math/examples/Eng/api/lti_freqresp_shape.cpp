#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
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

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0});
  const pluss::Series siso = lti::freqresp(oscillator(), omega);
  out.writeln("one input, one output, three frequencies: shape", siso.size(0), "x", siso.size(1), "x", siso.size(2));
  const lti::StateSpace mimo = lti::state(pluss::series::eye(3, pluss::ScalarType::Float64).mul(-1.0), pluss::series::ones({3, 2}, pluss::ScalarType::Float64), pluss::series::ones({2, 3}, pluss::ScalarType::Float64), pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  const pluss::Series m = lti::freqresp(mimo, omega);
  out.writeln("two inputs, two outputs, three frequencies: shape", m.size(0), "x", m.size(1), "x", m.size(2));
}
