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
  report([&] { (void)th::dew(vec({0.0}), vec({101325.0})); });        // dry air has no dew point
  report([&] { (void)th::dew(vec({0.5}), vec({101325.0})); });        // so humid the dew point is above 150 degrees
  report([&] { (void)th::dew(vec({0.01}), vec({-1.0})); });
}
