#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({10.0})};
  net.inductors = {{2}, {3}, vec({1e-3})};
  net.capacitors = {{3}, {0}, vec({1e-6})};
  const double w0 = 1.0 / std::sqrt(1e-3 * 1e-6);
  const double quality = std::sqrt(1e-3 / 1e-6) / 10.0;

  const auto result = ckt::ac(ckt::make(net), vec({0.1 * w0, w0, 10.0 * w0}));
  out.writeln("resonant frequency", w0, " quality factor", quality);
  out.writeln("source current magnitude  ", flat(result.source_current.abs()), " (0.1 A at resonance)");
  out.writeln("capacitor voltage magnitude", flat(result.voltage.select(1, 3).abs()), " (Q at resonance)");
  out.writeln("inductor current magnitude ", flat(result.inductor_current.abs()));
}
