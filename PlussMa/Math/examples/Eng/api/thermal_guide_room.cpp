#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A room: air, a two-layer wall (brick then insulation, 20 m^2), the outdoors with a daily swing, ventilation and a heater.
  th::Layers layers;
  layers.cells = {4, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  th::Slab w = th::mesh(layers, vec({20.0}).reshape({}));
  const std::int64_t air = w.net.nodes, outdoors = w.net.nodes + 1;       // nodes after the slab's surfaces
  w.net.nodes += 2;
  const auto link = [&](std::int64_t a, std::int64_t b, double g) {
    w.net.links.a.push_back(a);
    w.net.links.b.push_back(b);
    w.net.links.conductance = w.net.links.conductance.unite({vec({g})}, -1);
  };
  link(air, w.inner, 8.0 * 20.0);          // film inside
  link(w.outer, outdoors, 25.0 * 20.0);    // film outside
  link(air, outdoors, 15.0);               // ventilation
  w.net.stores.node.push_back(air);
  w.net.stores.capacity = w.net.stores.capacity.unite({vec({6.0e4})}, -1);
  w.net.fixed = {{outdoors}, vec({5.0})};
  w.net.heats = {{air}, vec({400.0})};
  // Mean outdoor temperature 5 degrees, swinging 5 degrees either way over a 24 h cycle; six days in 10 minute steps.
  const double day = 86400.0;
  std::vector<double> outside;
  for (int k = 0; k < 865; ++k) outside.push_back(5.0 + 5.0 * std::sin(2.0 * M_PI * 600.0 * k / day));
  th::Drive drive;
  drive.temperature = mat(outside, {865, 1});
  th::Start start;
  start.temperature = vec(std::vector<double>(7, 15.0));
  const auto run = th::tran(w.net, vec(grid(600.0, 865)), drive, start);
  const std::vector<double> room = flat(run.temperature.select(1, air));
  out.writeln("room temperature at the end of day 1, 2, 4, 6", room[144], room[288], room[576], room[864]);
  double low = 1e9, high = -1e9;
  for (int k = 720; k <= 864; ++k) low = std::min(low, room[k]), high = std::max(high, room[k]);
  out.writeln("last day: lowest", low, " highest", high, " swing", high - low);
  th::Net mean = w.net;
  mean.fixed = {{outdoors}, vec({5.0})};
  out.writeln("steady state at the mean outdoor temperature", flat(th::steady(mean).temperature.select(0, air)));
}
