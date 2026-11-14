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
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.stores = {{1}, vec({50.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto steady = th::steady(net);
  out.writeln("steady: temperature", steady.temperature.sizes(), " link_flow", steady.link_flow.sizes(), " fixed_flow", steady.fixed_flow.sizes(), " residual", steady.residual.sizes());
  const auto run = th::tran(net, vec(grid(10.0, 4)));
  out.writeln("tran:   temperature", run.temperature.sizes(), " link_flow", run.link_flow.sizes(), " fixed_flow", run.fixed_flow.sizes(), " residual", run.residual.sizes());
  out.writeln("link flow at the first sample", flat(run.link_flow.select(0, 0)), "W: the store starts at zero, so heat flows into it");
}
