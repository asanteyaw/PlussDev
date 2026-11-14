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
  report([&] { (void)th::props(th::Fluid::Water, vec({400.0})); });            // above the validity range of the liquid
  report([&] { (void)th::props(th::Fluid::Air, vec({100.0})); });              // below the validity range of the air tables
  report([&] { (void)th::props(th::Fluid::Air, vec({300.0}), vec({-1.0})); }); // non-positive pressure
  report([&] { (void)th::props(th::Fluid::Water, vec({std::nan("")})); });
}
