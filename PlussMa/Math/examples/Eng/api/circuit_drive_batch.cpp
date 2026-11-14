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
  net.voltage_sources = {{1}, {0}, vec({1.0, 2.0, 4.0}).reshape({3, 1})};   // the stored values only give the batch shape
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const std::vector<double> time = grid(1e-4, 101);
  std::vector<double> waves;
  for (double amplitude : {1.0, 2.0, 4.0})
    for (double t : time) waves.push_back(amplitude * std::sin(1000.0 * t));
  ckt::Drive drive;
  drive.voltage = vec(waves).reshape({3, 101, 1});          // one waveform per lane
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);
  out.writeln("voltage shape", run.voltage.sizes());
  for (int lane = 0; lane < 3; ++lane) out.writeln("lane", lane, "output at 10 ms", flat(run.voltage.select(0, lane).select(1, 2))[100]);

  // one waveform shared by every lane of a swept circuit
  ckt::Net swept = net;
  swept.voltage_sources.value = vec({1.0});
  swept.resistors.value = vec({500.0, 1000.0, 2000.0}).reshape({3, 1});
  ckt::Drive shared;
  shared.voltage = vec(std::vector<double>(waves.begin(), waves.begin() + 101)).reshape({101, 1});   // no lane axis
  const auto swept_run = ckt::tran(ckt::make(swept), vec(time), shared);
  out.writeln("shared drive, voltage shape", swept_run.voltage.sizes());
}
