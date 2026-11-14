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
  out.print_defaults(PrintOptions{}.fixed(6));
  // a lossless LC oscillator: 1 mH, 1 uF, the capacitor starts at 1 V
  ckt::Net lc;
  lc.nodes = 2;
  lc.inductors = {{1}, {0}, vec({1e-3})};
  lc.capacitors = {{1}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(lc);
  ckt::Start start;
  start.capacitor_voltage = vec({1.0});
  const double period = 2.0 * M_PI * std::sqrt(1e-3 * 1e-6);
  const std::vector<double> time = grid(period / 40.0, 161);   // four periods at 40 samples per period

  for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
    ckt::TranOptions options;
    options.scheme = scheme;
    const auto run = ckt::tran(circuit, vec(time), {}, start, options);
    const double v = flat(run.voltage.select(1, 1)).back(), i = flat(run.inductor_current).back();
    out.writeln(scheme == ckt::Scheme::Trapezoid ? "Trapezoid" : "Gear2    ", "energy left after four periods", (0.5e-6 * v * v + 0.5e-3 * i * i) / 0.5e-6);
  }
}
