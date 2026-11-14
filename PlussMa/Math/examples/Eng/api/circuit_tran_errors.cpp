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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const ckt::Circuit circuit = ckt::make(net);
  report([&] { (void)ckt::tran(circuit, vec({0.0})); });
  report([&] { (void)ckt::tran(circuit, vec({0.0, 1e-3, 3e-3})); });
  report([&] { (void)ckt::tran(circuit, vec({1e-3, 0.0})); });
  report([&] { (void)ckt::tran(circuit, pluss::series::zeros({2, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });

  // a capacitor directly across the ideal source cannot start at rest
  ckt::Net across;
  across.nodes = 2;
  across.voltage_sources = {{1}, {0}, vec({5.0})};
  across.resistors = {{1}, {0}, vec({1000.0})};
  across.capacitors = {{1}, {0}, vec({1e-6})};
  report([&] { (void)ckt::tran(ckt::make(across), vec({0.0, 1e-3})); });
}
