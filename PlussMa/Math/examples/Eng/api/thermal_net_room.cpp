#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A room: air (node 0) and a wall (node 1) against the outside (node 2) at -5 degrees; 1 kW heats the air.
  //   air -- 40 W/K -- wall -- 25 W/K -- outside,   plus 30 W/K of ventilation from air straight to outside
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1, 0}, {1, 2, 2}, vec({40.0, 25.0, 30.0})};
  net.stores = {{0, 1}, vec({5.0e4, 4.0e6})};
  net.fixed = {{2}, vec({-5.0})};
  net.heats = {{0}, vec({1000.0})};
  const auto run = th::steady(net);
  out.writeln("air, wall", flat(run.temperature));
  out.writeln("flows (air-wall, wall-outside, air-outside)", flat(run.link_flow));
  out.writeln("the outside absorbs", flat(run.fixed_flow), "W of the 1000 W");
  const double rise = 1000.0 / (30.0 + 1.0 / (1.0 / 40.0 + 1.0 / 25.0));
  out.writeln("closed form air rise over the outside", rise, " air", -5.0 + rise);
}
