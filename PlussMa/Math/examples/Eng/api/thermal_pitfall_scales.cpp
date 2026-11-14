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
  // A network is blind to the temperature scale, so mixing scales gives a wrong answer without an error:
  // the boundary is at 20 degrees Celsius, but the body was started at 293.15 (kelvin).
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  th::Start mixed, consistent;
  mixed.temperature = vec({293.15});
  consistent.temperature = vec({20.0});
  const pluss::Series time = vec(grid(200.0, 4));
  out.writeln("mixed scales   ", flat(th::tran(net, time, {}, mixed).temperature.select(1, 0)), " the body appears to cool to 20 from 293");
  out.writeln("one scale      ", flat(th::tran(net, time, {}, consistent).temperature.select(1, 0)), " the body is already at the boundary");
}
