#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 51)));   // 5 ms in steps of 0.1 ms

  out.writeln("voltage shape", run.voltage.sizes(), " source current shape", run.source_current.sizes(), " residual shape", run.residual.sizes());
  out.writeln("first sample", flat(run.voltage.select(0, 0)));
  const std::vector<double> v = flat(run.voltage.select(1, 2));
  for (int ms : {1, 2, 5}) out.writeln("t =", ms, "ms  v =", v[ms * 10], " closed form", 5.0 * (1.0 - std::exp(-ms * 1e-3 / 1e-3)));
  out.writeln("largest step residual", run.residual.amax().item<double>() < 1e-12 ? "below 1e-12" : "too large");
}
