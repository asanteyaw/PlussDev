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
  // a sweep of the resistor: tau = 0.5, 1 and 2 ms
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({500.0, 1000.0, 2000.0}).reshape({3, 1})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 51)));
  out.writeln("voltage shape", run.voltage.sizes(), " residual shape", run.residual.sizes());
  const double taus[3] = {0.5e-3, 1e-3, 2e-3};
  for (int lane = 0; lane < 3; ++lane)
    out.writeln("lane", lane, "output at 2 ms", flat(run.voltage.select(0, lane).select(1, 2))[20], " closed form", 5.0 * (1.0 - std::exp(-2e-3 / taus[lane])));

  // single precision circuits and time vectors stay single precision
  ckt::Net single;
  single.nodes = 3;
  const auto f32 = pluss::ScalarType::Float32;
  single.voltage_sources = {{1}, {0}, vec({5.0}).to(f32)};
  single.resistors = {{1}, {2}, vec({1000.0}).to(f32)};
  single.capacitors = {{2}, {0}, vec({1e-6}).to(f32)};
  const auto fast = ckt::tran(ckt::make(single), vec(grid(1e-4, 51)).to(f32));
  out.writeln("single precision run has element type", fast.voltage.dtype().name, " output at 5 ms", flat(fast.voltage.select(1, 2))[50]);
}
