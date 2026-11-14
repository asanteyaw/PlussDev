#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // a half-wave rectifier sampled four times per cycle: the step is subdivided where the diode switches
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-5})};
  const auto circuit = ckt::make(net);
  for (double step : {5e-5, 5e-4}) {
    const int samples = static_cast<int>(std::lround(8e-3 / step)) + 1;
    std::vector<double> time, wave;
    for (int k = 0; k < samples; ++k) {
      time.push_back(k * step);
      wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
    }
    ckt::Drive drive;
    drive.voltage = vec(wave).reshape({samples, 1});
    const auto run = ckt::tran(circuit, vec(time), drive);
    std::int64_t refined = 0;
    for (std::int64_t count : run.refinements.items<std::int64_t>()) refined += count;
    out.writeln("step", step * 1e6, "us  highest load voltage", run.voltage.select(1, 2).amax().item<double>(), " refined sub-steps", refined);
  }
  // with refinement off a long step rings above the source peak
  ckt::TranOptions off;
  off.solver.max_refinement = 0;
  const double step = 5e-4;
  std::vector<double> time, wave;
  for (int k = 0; k <= 16; ++k) {
    time.push_back(k * step);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({17, 1});
  out.writeln("500 us, max_refinement 0, highest load voltage", ckt::tran(circuit, vec(time), drive, {}, off).voltage.select(1, 2).amax().item<double>(), " (the source peak is 5)");
}
