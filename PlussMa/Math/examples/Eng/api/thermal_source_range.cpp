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
  // The range in the provenance is the range props accepts: check before calling, or take the error.
  for (const auto fluid : {th::Fluid::Water, th::Fluid::Air}) {
    const th::Origin s = th::source(fluid);
    out.writeln(s.name, "accepts", s.t_min, "to", s.t_max, "K");
    out.writeln("  at the limits", flat(th::props(fluid, vec({s.t_min, s.t_max})).density));
  }
}
