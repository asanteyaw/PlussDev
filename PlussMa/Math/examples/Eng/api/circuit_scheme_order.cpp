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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
    ckt::TranOptions options;
    options.scheme = scheme;
    out.writeln(scheme == ckt::Scheme::Trapezoid ? "Trapezoid" : "Gear2    ", "largest error at steps of 0.2, 0.1, 0.05 ms:");
    for (double step : {2e-4, 1e-4, 5e-5}) {
      const std::vector<double> time = grid(step, static_cast<int>(std::round(5e-3 / step)) + 1);
      const std::vector<double> v = flat(ckt::tran(circuit, vec(time), {}, {}, options).voltage.select(1, 2));
      double worst = 0.0;
      for (std::size_t k = 0; k < time.size(); ++k) worst = std::max(worst, std::abs(v[k] - 5.0 * (1.0 - std::exp(-time[k] / 1e-3))));
      out.writeln("  step", step * 1e3, "ms  error", worst);
    }
  }
}
