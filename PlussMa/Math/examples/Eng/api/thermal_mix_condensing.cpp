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
  // Mixing two saturated streams makes supersaturated air: the straight-line mix lies above the saturation curve and fog forms.
  const double p = 101325.0;
  const pluss::Series w1 = th::ratio(vec({5.0}), vec({1.0}), vec({p})), w2 = th::ratio(vec({30.0}), vec({1.0}), vec({p}));
  const auto m = th::mix(vec({5.0}), w1, vec({1.0}), vec({30.0}), w2, vec({1.0}));
  out.writeln("mixed temperature", flat(m.temperature), " w", flat(m.w));
  out.writeln("saturation w at that temperature", flat(th::ratio(m.temperature, vec({1.0}), vec({p}))), "-> supersaturated, relative humidity", flat(th::rh(m.temperature, m.w, vec({p}))));
}
