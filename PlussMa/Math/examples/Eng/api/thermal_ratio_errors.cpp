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
  report([&] { (void)th::ratio(vec({25.0}), vec({1.5}), vec({101325.0})); });    // relative humidity above 1
  report([&] { (void)th::ratio(vec({25.0}), vec({-0.1}), vec({101325.0})); });   // negative
  report([&] { (void)th::ratio(vec({25.0}), vec({0.5}), vec({0.0})); });         // zero pressure
  report([&] { (void)th::ratio(vec({120.0}), vec({0.9}), vec({50000.0})); });    // vapour pressure reaches the total
}
