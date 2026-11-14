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
  report([&] { (void)th::eff(vec({-1.0}), vec({0.5}), th::Flow::Counter); });   // negative NTU
  report([&] { (void)th::eff(vec({2.0}), vec({1.5}), th::Flow::Counter); });    // cr above one: it is Cmin / Cmax
  report([&] { (void)th::eff(vec({2.0}), vec({-0.1}), th::Flow::Counter); });   // negative cr
  report([&] { (void)th::eff(vec({std::nan("")}), vec({0.5}), th::Flow::Cross); }); // not finite
}
