#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net follower(double gamma, bool bulk_grounded) {
  // nodes: 1 drain supply, 2 gate drive, 3 source; the source resistor goes to ground
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 2}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{3}, {0}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {3};
  net.mosfets.polarity = {1};
  if (bulk_grounded) net.mosfets.bulk = {0};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (gamma > 0.0) net.mosfets.body_effect = vec({gamma});
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
  auto lengths = follower(0.5, true);
  lengths.mosfets.bulk = {0, 0};
  report([&] { (void)ckt::make(lengths); });
  auto negative = follower(0.5, true);
  negative.mosfets.body_effect = vec({-0.1});
  report([&] { (void)ckt::make(negative); });
  auto potential = follower(0.5, true);
  potential.mosfets.surface_potential = vec({0.0});
  report([&] { (void)ckt::make(potential); });
  auto outside = follower(0.5, true);
  outside.mosfets.bulk = {9};
  report([&] { (void)ckt::make(outside); });
}
