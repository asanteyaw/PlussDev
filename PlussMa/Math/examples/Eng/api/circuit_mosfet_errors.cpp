#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
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
  auto polarity = common_source(10.0, 3.0, 500.0);
  polarity.mosfets.polarity = {2};
  report([&] { (void)ckt::make(polarity); });
  auto no_gain = common_source(10.0, 3.0, 500.0);
  no_gain.mosfets.transconductance = vec({0.0});
  report([&] { (void)ckt::make(no_gain); });
  auto no_threshold = common_source(10.0, 3.0, 500.0);
  no_threshold.mosfets.threshold = pluss::Series{};
  report([&] { (void)ckt::make(no_threshold); });
  auto tied = common_source(10.0, 3.0, 500.0);
  tied.mosfets.source = {2};
  report([&] { (void)ckt::make(tied); });
  auto negative_cap = common_source(10.0, 3.0, 500.0);
  negative_cap.mosfets.gate_source_capacitance = vec({-1e-9});
  report([&] { (void)ckt::make(negative_cap); });
  auto lengths = common_source(10.0, 3.0, 500.0);
  lengths.mosfets.gate = {3, 3};
  report([&] { (void)ckt::make(lengths); });
}
