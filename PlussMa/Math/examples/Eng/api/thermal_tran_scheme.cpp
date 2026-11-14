#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A step much longer than the time constant: the trapezoid rule rings around the answer, the second-order backward rule does not.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({1.0})};
  net.stores = {{0}, vec({1.0})};
  net.fixed = {{1}, vec({10.0})};
  th::Start start;
  start.temperature = vec({0.0});
  const auto trapezoid = th::tran(net, vec(grid(5.0, 6)), {}, start, ckt::Scheme::Trapezoid);
  const auto gear = th::tran(net, vec(grid(5.0, 6)), {}, start, ckt::Scheme::Gear2);
  out.writeln("trapezoid", flat(trapezoid.temperature.select(1, 0)));
  out.writeln("gear2    ", flat(gear.temperature.select(1, 0)));
  std::vector<double> exact;
  for (int k = 0; k <= 5; ++k) exact.push_back(10.0 * (1.0 - std::exp(-5.0 * k)));
  out.writeln("exact    ", exact);
  out.writeln("each step is five time constants long; the exact curve has settled after the first");
}
