#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

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
  net.nodes = 3;
  net.links = {{0}, {1}, vec({2.0})};
  net.fixed = {{0}, vec({100.0})};
  report([&] { (void)th::steady(net); });             // node 2 has no link
  th::Net open = net;
  open.fixed = {};
  report([&] { (void)th::steady(open); });            // nothing fixes the temperature
  th::Net far = net;
  far.links.b = {7};
  report([&] { (void)th::steady(far); });             // a node index outside the network
  th::Net loop = net;
  loop.links.b = {0};
  report([&] { (void)th::steady(loop); });            // a link from a node to itself
  th::Net dead = net;
  dead.links.conductance = vec({0.0});
  report([&] { (void)th::steady(dead); });            // a conductance that is not positive
}
