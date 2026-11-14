#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const int samples = 81;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * 5e-5);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-5));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  // a looser tolerance gives Newton fewer iterations per step
  for (double tolerance : {1e-4, 1e-8, 1e-12}) {
    ckt::TranOptions options;
    options.solver.tolerance = tolerance;
    const auto run = ckt::tran(circuit, vec(time), drive, {}, options);
    std::int64_t total = 0;
    for (std::int64_t count : run.iterations.items<std::int64_t>()) total += count;
    out.writeln("tolerance", tolerance, " total Newton iterations", total, " highest load voltage", run.voltage.select(1, 2).amax().item<double>());
  }
}
