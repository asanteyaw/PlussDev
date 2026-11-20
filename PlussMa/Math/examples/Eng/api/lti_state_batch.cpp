#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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
  // A leading axis holds four systems that differ in their damping; the matrix axes stay last.
  std::vector<double> a;
  for (const double damping : {0.0, 0.2, 0.4, 0.8}) for (const double v : {0.0, 1.0, -4.0, -damping}) a.push_back(v);
  const lti::StateSpace batch = lti::state(pluss::make_series(a, {4, 2, 2}), pluss::series::zeros({4, 2, 1}, pluss::ScalarType::Float64).add(mat({0.0, 1.0}, 2, 1)),
                                          pluss::series::zeros({4, 1, 2}, pluss::ScalarType::Float64).add(mat({1.0, 0.0}, 1, 2)), pluss::series::zeros({4, 1, 1}, pluss::ScalarType::Float64));
  out.writeln("A shape", batch.a().size(0), "x", batch.a().size(1), "x", batch.a().size(2), " states", batch.states(), "inputs", batch.inputs(), "outputs", batch.outputs());
}
