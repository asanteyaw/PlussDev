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
  const pluss::Series time = vec(grid(1e-4, 21));
  ckt::TranOptions trapezoid;
  trapezoid.scheme = ckt::Scheme::Trapezoid;
  ckt::TranOptions gear;
  gear.scheme = ckt::Scheme::Gear2;
  const auto omitted = flat(ckt::tran(circuit, time).voltage.select(1, 2));
  const auto explicit_trapezoid = flat(ckt::tran(circuit, time, {}, {}, trapezoid).voltage.select(1, 2));
  const auto explicit_gear = flat(ckt::tran(circuit, time, {}, {}, gear).voltage.select(1, 2));
  double same = 0.0, differs = 0.0;
  for (std::size_t k = 0; k < omitted.size(); ++k) {
    same = std::max(same, std::abs(omitted[k] - explicit_trapezoid[k]));
    differs = std::max(differs, std::abs(omitted[k] - explicit_gear[k]));
  }
  out.writeln("options omitted vs Trapezoid, largest difference", same);
  out.writeln("options omitted vs Gear2,     largest difference", differs);
}
