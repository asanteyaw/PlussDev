#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const th::Origin s = th::source(th::Fluid::Water);
  out.writeln("name:", s.name);
  out.writeln("range:", s.t_min, "to", s.t_max, "K");
  out.writeln("state:", s.state);
  out.writeln("reference:", s.reference);
}
