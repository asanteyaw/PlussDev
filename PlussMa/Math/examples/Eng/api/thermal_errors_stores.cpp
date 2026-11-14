#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
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
  const pluss::Series time = vec(grid(10.0, 3));
  net.stores = {{0, 0}, vec({10.0, 20.0})};
  out.writeln("two capacities on one node run:", flat(th::tran(net, time).temperature).size(), "values");
  net.stores = {{4}, vec({10.0})};
  report([&] { (void)th::tran(net, time); });                  // a node outside the network
  net.stores = {{0}, vec({0.0})};
  report([&] { (void)th::tran(net, time); });                  // zero capacity
  net.stores = {{0}, vec({10.0, 20.0})};
  report([&] { (void)th::tran(net, time); });                  // one value too many
}
