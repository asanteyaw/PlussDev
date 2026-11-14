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
  report([&] { (void)th::fin(vec({25.0}), vec({200.0}), vec({0.016}), vec({1.6e-5}), vec({0.1}), vec({20.0}), vec({20.0})); }); // base equals ambient
  report([&] { (void)th::fin(vec({25.0}), vec({200.0}), vec({0.016}), vec({1.6e-5}), vec({0.0}), vec({100.0}), vec({20.0})); });  // zero length
  report([&] { (void)th::fin(vec({0.0}), vec({200.0}), vec({0.016}), vec({1.6e-5}), vec({0.1}), vec({100.0}), vec({20.0})); });  // zero coefficient
}
