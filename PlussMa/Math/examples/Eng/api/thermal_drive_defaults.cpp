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
  // An undefined Drive member keeps the stored values; a defined one replaces them sample by sample.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  th::Start warm;
  warm.temperature = vec({20.0});
  const pluss::Series time = vec(grid(100.0, 4));
  const auto held = th::tran(net, time, {}, warm);
  th::Drive hot;
  hot.temperature = mat({20, 60, 60, 60}, {4, 1});
  const auto driven = th::tran(net, time, hot, warm);
  out.writeln("stored ambient   ", flat(held.temperature.select(1, 0)));
  out.writeln("ambient steps up ", flat(driven.temperature.select(1, 0)));
}
