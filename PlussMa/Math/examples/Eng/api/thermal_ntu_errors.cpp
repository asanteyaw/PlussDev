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
  report([&] { (void)th::ntu(vec({-0.1}), vec({0.5}), th::Flow::Counter); });  // negative effectiveness
  report([&] { (void)th::ntu(vec({0.5}), vec({2.0}), th::Flow::Counter); });   // cr above one
  report([&] { (void)th::ntu(vec({1.5}), vec({0.5}), th::Flow::Cross); });     // effectiveness above one
  out.writeln("a boiler needs", flat(th::ntu(vec({0.5}), vec({0.5}), th::Flow::Boil)), "units for 0.5");
}
