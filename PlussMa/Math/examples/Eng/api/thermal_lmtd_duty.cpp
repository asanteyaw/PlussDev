#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Duty = UA x LMTD agrees with the effectiveness method for the same exchanger.
  const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), th::Flow::Counter);
  const pluss::Series lm = th::lmtd(vec({90.0}), r.hot_out, vec({20.0}), r.cold_out, th::Flow::Counter);
  out.writeln("LMTD", flat(lm), " UA x LMTD", 1500.0 * one(lm), " duty from NTU", flat(r.duty));
}
