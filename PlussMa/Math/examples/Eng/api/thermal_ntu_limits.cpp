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
  // Some arrangements cannot reach every effectiveness; the call says so rather than returning a nonsense NTU.
  const pluss::Series cr = vec({0.5});
  out.writeln("shell-and-tube ceiling at cr = 0.5:", 2.0 / (1.5 + std::sqrt(1.25)));
  out.writeln("parallel-flow ceiling at cr = 0.5:", 1.0 / 1.5);
  report([&] { (void)th::ntu(vec({0.8}), cr, th::Flow::Shell); });
  report([&] { (void)th::ntu(vec({0.7}), cr, th::Flow::Parallel); });
  report([&] { (void)th::ntu(vec({1.0}), cr, th::Flow::Counter); });
  out.writeln("counter flow can approach 1:", flat(th::ntu(vec({0.999}), cr, th::Flow::Counter)), "units");
}
