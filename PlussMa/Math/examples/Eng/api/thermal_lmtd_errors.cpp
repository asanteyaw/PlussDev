#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] { (void)th::lmtd(vec({50.0}), vec({30.0}), vec({40.0}), vec({45.0}), th::Flow::Counter); });   // the streams cross
  report([&] { (void)th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({60.0}), th::Flow::Cross); });     // no plain log mean for cross flow
  report([&] { (void)th::lmtd(vec({90.0}), vec({50.0}), vec({std::nan("")}), vec({60.0}), th::Flow::Counter); });  // not finite
}
