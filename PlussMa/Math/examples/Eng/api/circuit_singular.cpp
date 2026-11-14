#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] {
    ckt::Net floating;
    floating.nodes = 3;
    floating.resistors = {{1}, {0}, vec({1.0})};   // node 2 is connected to nothing
    (void)ckt::op(ckt::make(floating));
  });
  report([&] {
    ckt::Net clash;
    clash.nodes = 3;
    clash.voltage_sources = {{1, 1}, {0, 0}, vec({1.0, 2.0})};   // 1 V and 2 V across the same nodes
    clash.resistors = {{1}, {2}, vec({1.0})};
    (void)ckt::op(ckt::make(clash));
  });
}
