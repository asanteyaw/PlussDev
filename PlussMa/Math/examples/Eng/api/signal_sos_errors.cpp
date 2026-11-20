#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  refused(out, "sections of the wrong width", [] { sig::sos(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 0.0, 0.0, 1.0, 0.0}).reshape({1, 5})); });
  refused(out, "a zero leading denominator coefficient", [] { sig::sos(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0}).reshape({1, 6})); });
  refused(out, "no sections", [] { sig::sos(pluss::make_series({1.0, 2.0}), pluss::series::zeros({0, 6}, pluss::ScalarType::Float64)); });
}
