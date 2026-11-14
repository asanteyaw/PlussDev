#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  // iterations records how many Newton iterations each time sample needed
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const int samples = 201;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * 5e-5);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-5));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);
  const std::vector<double> counts = flat(run.iterations.to(pluss::ScalarType::Float64));
  double most = 0.0, total = 0.0;
  for (double c : counts) {
    most = std::max(most, c);
    total += c;
  }
  out.writeln("iterations shape", run.iterations.sizes());
  out.writeln("most iterations in one sample", most, " total", total);
  out.writeln("first sample (the starting state) took", counts[0], " iterations");
}
