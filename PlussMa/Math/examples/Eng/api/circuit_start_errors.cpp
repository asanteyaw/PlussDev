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
  const pluss::Series time = vec({0.0, 1e-3});
  ckt::Start two;
  two.capacitor_voltage = vec({1.0, 2.0});
  report([&] { (void)ckt::tran(circuit, time, {}, two); });
  ckt::Start no_inductor;
  no_inductor.inductor_current = vec({0.1});
  report([&] { (void)ckt::tran(circuit, time, {}, no_inductor); });
  ckt::Start not_finite;
  not_finite.capacitor_voltage = vec({std::nan("")});
  report([&] { (void)ckt::tran(circuit, time, {}, not_finite); });
  ckt::Net across;
  across.nodes = 2;
  across.voltage_sources = {{1}, {0}, vec({5.0})};
  across.resistors = {{1}, {0}, vec({1000.0})};
  across.capacitors = {{1}, {0}, vec({1e-6})};
  ckt::Start wrong;
  wrong.capacitor_voltage = vec({2.0});
  report([&] { (void)ckt::tran(ckt::make(across), time, {}, wrong); });
}
