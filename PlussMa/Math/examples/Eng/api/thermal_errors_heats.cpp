#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.fixed = {{1}, vec({20.0})};
  net.heats = {{0}, vec({10.0, 5.0})};
  report([&] { (void)th::steady(net); });                      // two powers for one node
  net.heats = {{9}, vec({10.0})};
  report([&] { (void)th::steady(net); });                      // a node outside the network
  net.heats = {{0}, vec({-10.0})};
  const auto cooling = th::steady(net);
  out.writeln("a negative power withdraws heat:", flat(cooling.temperature));
}
