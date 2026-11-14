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
  th::Layers layers;
  layers.cells = {3, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  report([&] { (void)th::mesh(layers, vec({-1.0}).reshape({})); });   // negative area
  th::Layers few = layers;
  few.cells = {3};
  report([&] { (void)th::mesh(few, vec({1.0}).reshape({})); });       // one cell count for two layers
  th::Layers none = layers;
  none.cells = {3, 0};
  report([&] { (void)th::mesh(none, vec({1.0}).reshape({})); });      // a layer with no cells
  th::Layers thin = layers;
  thin.thickness = vec({0.2, 0.0});
  report([&] { (void)th::mesh(thin, vec({1.0}).reshape({})); });      // zero thickness
}
