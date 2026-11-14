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
  report([&] { (void)th::wall(vec({0.0}), vec({0.1}), vec({1.0})); });     // zero conductivity
  report([&] { (void)th::wall(vec({1.4}), vec({-0.1}), vec({1.0})); });    // negative thickness
  report([&] { (void)th::wall(vec({1.4}), vec({0.1}), vec({std::nan("")})); }); // area not finite
  report([&] { (void)th::wall(pluss::series::make(std::vector<int>{1}), vec({0.1}), vec({1.0})); }); // not a real floating array
}
