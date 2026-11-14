#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A leading axis sweeps the conductance of the second link; one call solves every case.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, mat({2.0, 0.5, 2.0, 2.0, 2.0, 8.0}, {3, 2})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto run = th::steady(net);
  out.writeln("temperature shape", run.temperature.sizes(), " (case, node)");
  out.writeln("middle node per case", flat(run.temperature.select(1, 1)));
  out.writeln("heat flow per case", flat(run.link_flow.select(1, 0)));
  out.writeln("closed form", 80.0 / (1.0 / 2.0 + 1.0 / 0.5), 80.0 / (1.0 / 2.0 + 1.0 / 2.0), 80.0 / (1.0 / 2.0 + 1.0 / 8.0));
}
