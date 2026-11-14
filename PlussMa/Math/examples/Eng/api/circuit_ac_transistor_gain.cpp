#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static ckt::Circuit stage(double supply, double drive) {
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({supply, drive})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 10000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {1};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});
  return ckt::make(net);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto bias = ckt::op(stage(10.0, 1.2));
  const double gm = flat(bias.collector_current)[0] / 0.02585;
  const double rpi = 100.0 / gm;
  const auto gain = flat(ckt::ac(stage(0.0, 1.0), vec({0.0}), bias).voltage.select(-1, 2).real())[0];
  out.writeln("computed gain", gain, " formula -gm Rc rpi / (Rb + rpi)", -gm * 1000.0 * rpi / (10000.0 + rpi));
}
