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
  report([&] { (void)th::wet(vec({20.0}), vec({0.03}), vec({101325.0})); });   // supersaturated: the dew point is above 20
  report([&] { (void)th::wet(vec({150.0}), vec({0.01}), vec({101325.0})); });  // dry bulb outside -50..100
  report([&] { (void)th::wet(vec({20.0}), vec({0.0}), vec({101325.0})); });    // dry air
}
