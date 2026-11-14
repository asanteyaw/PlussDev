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
  // the Early voltage makes the collector current grow with the collector voltage: the supply moves the current
  for (double supply : {5.0, 10.0, 20.0}) {
    ckt::Net net;
    net.nodes = 5;
    net.voltage_sources = {{1, 3}, {0, 0}, vec({supply, 5.0})};
    net.resistors = {{1, 3}, {2, 4}, vec({100.0, 100000.0})};
    net.transistors.collector = {2};
    net.transistors.base = {4};
    net.transistors.emitter = {0};
    net.transistors.polarity = {1};
    net.transistors.saturation = vec({1e-14});
    net.transistors.forward_gain = vec({100.0});
    const auto flat_model = ckt::op(ckt::make(net));
    net.transistors.early_voltage = vec({50.0});
    const auto early = ckt::op(ckt::make(net));
    out.writeln("supply", supply, " collector current without and with the Early effect", flat(flat_model.collector_current)[0], flat(early.collector_current)[0]);
  }
}
