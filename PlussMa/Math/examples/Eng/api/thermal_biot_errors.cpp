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
  report([&] { (void)th::biot(vec({0.0}), vec({15.0}), vec({0.02})); });   // zero coefficient
  report([&] { (void)th::biot(vec({50.0}), vec({0.0}), vec({0.02})); });   // zero conductivity
  report([&] { (void)th::biot(vec({50.0}), vec({15.0}), vec({-0.02})); }); // negative length
}
