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
  const pluss::Series b = pluss::make_series({0.5, 0.5});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  pluss::Series state;
  std::vector<double> all;
  for (int block = 0; block < 3; ++block) {
    const pluss::Series chunk = pluss::make_series({1.0 + block * 4, 2.0 + block * 4, 3.0 + block * 4, 4.0 + block * 4});
    const sig::FilterRun run = block == 0 ? sig::filter(chunk, b, a) : sig::filter(chunk, b, a, state);
    state = run.state;
    for (const double v : run.values.items<double>()) all.push_back(v);
  }
  const pluss::Series whole = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0});
  const auto once = sig::filter(whole, b, a).values.items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < once.size(); ++i) worst = std::max(worst, std::abs(once[i] - all[i]));
  out.writeln("three blocks of four samples against one pass of twelve, largest difference =", worst);
}
