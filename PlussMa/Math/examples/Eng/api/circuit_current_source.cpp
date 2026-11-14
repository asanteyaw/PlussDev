#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  ckt::Net into;
  into.nodes = 2;
  into.current_sources = {{0}, {1}, vec({0.002})};
  into.resistors = {{1}, {0}, vec({1000.0})};
  ckt::Net out_of = into;
  out_of.current_sources = {{1}, {0}, vec({0.002})};

  out.writeln("positive = 0, negative = 1: V1 =", flat(ckt::op(ckt::make(into)).voltage)[1]);
  out.writeln("positive = 1, negative = 0: V1 =", flat(ckt::op(ckt::make(out_of)).voltage)[1]);
}
