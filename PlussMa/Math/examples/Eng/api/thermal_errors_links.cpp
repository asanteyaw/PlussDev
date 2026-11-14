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
  net.nodes = 2;
  net.fixed = {{1}, vec({20.0})};
  net.links = {{0}, {1}, vec({1.0, 2.0})};
  report([&] { (void)th::steady(net); });                      // two conductances for one link
  net.links = {{0, 1}, {1}, vec({1.0, 2.0})};
  report([&] { (void)th::steady(net); });                      // links.a and links.b of different lengths
}
