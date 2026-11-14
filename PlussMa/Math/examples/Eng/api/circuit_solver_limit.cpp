#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  const std::int64_t needed = ckt::op(circuit).iterations.items<std::int64_t>()[0];
  out.writeln("iterations the default solve needs", needed);
  for (std::int64_t limit : {needed, needed - 1, std::int64_t{3}}) {
    ckt::SolveOptions options;
    options.max_iterations = limit;
    try {
      const auto result = ckt::op(circuit, options);
      out.writeln("limit", limit, " converged in", result.iterations.items<std::int64_t>()[0]);
    } catch (const pluss::Error &e) {
      out.writeln("limit", limit, "failed:", e.message());
    }
  }
}
