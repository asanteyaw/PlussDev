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
  report([&] { ckt::Net net; net.nodes = 1; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {1}, vec({1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {5}, vec({1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {0}, vec({-1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1, 2}, {0, 0}, vec({1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {0, 2}, vec({1.0})}; (void)ckt::make(net); });
  report([&] {
    ckt::Net net;
    net.nodes = 3;
    net.resistors = {{1}, {0}, vec({1.0})};
    net.capacitors = {{2}, {0}, pluss::series::make(std::vector<float>{1.0f})};
    (void)ckt::make(net);
  });
}
