#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const auto base = [] {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    return net;
  };
  auto zero = base();
  zero.diodes.saturation = vec({0.0});
  report([&] { (void)ckt::make(zero); });
  auto missing = base();
  missing.diodes.saturation = ::pluss::Series{};
  report([&] { (void)ckt::make(missing); });
  auto shorted = base();
  shorted.diodes.negative = {2};
  report([&] { (void)ckt::make(shorted); });
  auto outside = base();
  outside.diodes.positive = {9};
  report([&] { (void)ckt::make(outside); });
  auto bad_ideality = base();
  bad_ideality.diodes.ideality = vec({-1.0});
  report([&] { (void)ckt::make(bad_ideality); });
}
