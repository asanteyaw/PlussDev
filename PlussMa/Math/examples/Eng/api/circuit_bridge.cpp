#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net bridge;
  bridge.nodes = 4;
  bridge.voltage_sources = {{1}, {0}, vec({10.0})};
  bridge.resistors = {{1, 2, 1, 3, 2}, {2, 0, 3, 0, 3}, vec({100.0, 200.0, 300.0, 600.0, 50.0})};
  const auto balanced = flat(ckt::op(ckt::make(bridge)).voltage);
  out.writeln("balanced:   V2", balanced[2], " V3", balanced[3], " difference", balanced[2] - balanced[3]);

  bridge.resistors.value = vec({100.0, 200.0, 300.0, 500.0, 50.0});
  const auto skewed = flat(ckt::op(ckt::make(bridge)).voltage);
  out.writeln("unbalanced: V2", skewed[2], " V3", skewed[3], " difference", skewed[2] - skewed[3]);
}
