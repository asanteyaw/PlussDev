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
  // a sinusoidal voltage source through an RC low-pass at its corner frequency
  ckt::Net rc;
  rc.nodes = 3;
  rc.voltage_sources = {{1}, {0}, vec({0.0})};
  rc.resistors = {{1}, {2}, vec({1000.0})};
  rc.capacitors = {{2}, {0}, vec({1e-6})};
  const std::vector<double> time = grid(1e-4, 401);
  std::vector<double> sine;
  for (double t : time) sine.push_back(std::sin(1000.0 * t));
  ckt::Drive voltage_drive;
  voltage_drive.voltage = vec(sine).reshape({401, 1});
  const std::vector<double> v = flat(ckt::tran(ckt::make(rc), vec(time), voltage_drive).voltage.select(1, 2));
  out.writeln("sinusoid: output at 10, 20, 40 ms", v[100], v[200], v[400]);

  // a current ramp of 1 A/s into a parallel RC: v = R a (t - tau + tau exp(-t/tau))
  ckt::Net parallel;
  parallel.nodes = 2;
  parallel.current_sources = {{0}, {1}, vec({0.0})};
  parallel.resistors = {{1}, {0}, vec({1000.0})};
  parallel.capacitors = {{1}, {0}, vec({1e-6})};
  std::vector<double> ramp;
  for (int k = 0; k < 51; ++k) ramp.push_back(k * 1e-4);
  ckt::Drive current_drive;
  current_drive.current = vec(ramp).reshape({51, 1});
  const std::vector<double> u = flat(ckt::tran(ckt::make(parallel), vec(grid(1e-4, 51)), current_drive).voltage.select(1, 1));
  out.writeln("ramp: voltage at 5 ms", u[50], " closed form", 1000.0 * (5e-3 - 1e-3 + 1e-3 * std::exp(-5.0)));
}
