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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const ckt::Circuit circuit = ckt::make(net);
  report([&] { (void)ckt::ac(circuit, vec({-1.0})); });
  report([&] { (void)ckt::ac(circuit, pluss::series::zeros({0}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
  report([&] { (void)ckt::ac(circuit, pluss::series::zeros({2, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
}
