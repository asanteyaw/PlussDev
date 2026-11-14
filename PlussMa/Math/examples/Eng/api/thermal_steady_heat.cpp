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
  // The same chain with 30 W injected into the middle node: both boundaries absorb it.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  net.heats = {{1}, vec({30.0})};
  const auto run = th::steady(net);
  out.writeln("temperature", flat(run.temperature));
  out.writeln("link flow", flat(run.link_flow));
  out.writeln("boundary delivers", flat(run.fixed_flow));
  out.writeln("injected + delivered =", 30.0 + flat(run.fixed_flow)[0] + flat(run.fixed_flow)[1], " (energy balance)");
}
