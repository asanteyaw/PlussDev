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
  // half-wave rectifier: 5 V peak, 500 Hz source, diode, 1 kohm and 1 uF load
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};

  const int samples = 401;
  const double step = 5e-5;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * step);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);

  const std::vector<double> load = flat(run.voltage.select(1, 2));
  const std::vector<double> diode = flat(run.diode_current);
  out.writeln("   ms    source    load   diode current");
  for (int k = 0; k <= 80; k += 5) out.writeln(time[k] * 1e3, wave[k], load[k], diode[k]);
  out.writeln("load peak", *std::max_element(load.begin(), load.end()), " lowest", *std::min_element(load.begin(), load.end()));
  out.writeln("diode_current shape", run.diode_current.sizes(), " (time sample, diode)");
  out.writeln("largest step residual below 1e-8", run.residual.amax().item<double>() < 1e-8);
}
