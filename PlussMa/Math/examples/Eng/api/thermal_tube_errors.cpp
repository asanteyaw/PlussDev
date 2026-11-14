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
  report([&] { (void)th::tube(vec({0.04}), vec({0.05}), vec({0.025}), vec({10.0})); });  // the outer radius is smaller
  report([&] { (void)th::tube(vec({0.04}), vec({0.025}), vec({0.025}), vec({10.0})); }); // equal radii
  report([&] { (void)th::tube(vec({0.04}), vec({0.025}), vec({0.05}), vec({0.0})); });   // zero length
}
