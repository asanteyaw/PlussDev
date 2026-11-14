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
  const pluss::Series time = vec({0.0, 1e-3, 2e-3});
  ckt::Drive rows;
  rows.voltage = vec({1.0, 2.0}).reshape({2, 1});
  report([&] { (void)ckt::tran(circuit, time, rows); });
  ckt::Drive columns;
  columns.voltage = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});
  report([&] { (void)ckt::tran(circuit, time, columns); });
  ckt::Drive lanes;
  lanes.voltage = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3, 1});
  report([&] { (void)ckt::tran(circuit, time, lanes); });
  ckt::Drive missing;
  missing.current = vec({1.0, 2.0, 3.0}).reshape({3, 1});
  report([&] { (void)ckt::tran(circuit, time, missing); });
  ckt::Drive not_finite;
  not_finite.voltage = vec({1.0, std::nan(""), 3.0}).reshape({3, 1});
  report([&] { (void)ckt::tran(circuit, time, not_finite); });
}
