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
    net.nodes = 5;
    net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0, 5.0})};
    net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
    net.transistors.collector = {2};
    net.transistors.base = {4};
    net.transistors.emitter = {0};
    net.transistors.polarity = {1};
    net.transistors.saturation = vec({1e-14});
    net.transistors.forward_gain = vec({100.0});
    return net;
  };
  auto polarity = base();
  polarity.transistors.polarity = {2};
  report([&] { (void)ckt::make(polarity); });
  auto tied = base();
  tied.transistors.emitter = {2};
  report([&] { (void)ckt::make(tied); });
  auto lengths = base();
  lengths.transistors.polarity = {1, 1};
  report([&] { (void)ckt::make(lengths); });
  auto no_gain = base();
  no_gain.transistors.forward_gain = ::pluss::Series{};
  report([&] { (void)ckt::make(no_gain); });
  auto negative_gain = base();
  negative_gain.transistors.reverse_gain = vec({-1.0});
  report([&] { (void)ckt::make(negative_gain); });
}
