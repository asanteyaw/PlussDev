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
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const std::vector<double> time = grid(1e-4, 21);
  // one start voltage per lane, three lanes of a swept circuit
  ckt::Net swept = net;
  swept.resistors.value = vec({1000.0, 1000.0, 1000.0}).reshape({3, 1});
  ckt::Start lanes;
  lanes.capacitor_voltage = vec({1.0, 2.0, 4.0}).reshape({3, 1});
  const auto run = ckt::tran(ckt::make(swept), vec(time), {}, lanes);
  out.writeln("voltage shape", run.voltage.sizes());
  for (int lane = 0; lane < 3; ++lane)
    out.writeln("lane", lane, "start", flat(run.voltage.select(0, lane).select(1, 2))[0], "at 2 ms", flat(run.voltage.select(0, lane).select(1, 2))[20], " closed form", (lane == 0 ? 1.0 : lane == 1 ? 2.0 : 4.0) * std::exp(-2.0));

  // one start voltage shared by every lane
  ckt::Start shared;
  shared.capacitor_voltage = vec({3.0});
  out.writeln("shared start, lane 2 at the first sample", flat(ckt::tran(ckt::make(swept), vec(time), {}, shared).voltage.select(0, 2).select(1, 2))[0]);
}
