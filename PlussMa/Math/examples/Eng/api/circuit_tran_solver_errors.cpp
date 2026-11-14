#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const int samples = 41;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * 5e-5);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-5));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  ckt::TranOptions few;
  few.solver.max_iterations = 1;
  report([&] { (void)ckt::tran(circuit, vec(time), drive, {}, few); });
  ckt::TranOptions negative;
  negative.solver.tolerance = -1.0;
  report([&] { (void)ckt::tran(circuit, vec(time), drive, {}, negative); });
  ckt::TranOptions leaky;
  leaky.solver.gmin = -1.0;
  report([&] { (void)ckt::tran(circuit, vec(time), drive, {}, leaky); });
}
