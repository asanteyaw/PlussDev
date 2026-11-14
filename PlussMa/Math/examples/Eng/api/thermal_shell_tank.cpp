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
  // A tank of 1 m radius held at 80 degrees, insulated to 1.1 m (k = 0.04), losing heat to 20 degree air through h = 8 W/(m^2 K).
  const double r1 = 1.0, r2 = 1.1;
  const pluss::Series insulation = th::shell(vec({0.04}), vec({r1}), vec({r2}));
  const pluss::Series film = th::film(vec({8.0}), vec({4.0 * M_PI * r2 * r2}));
  const double total = one(insulation) + one(film);
  out.writeln("insulation", flat(insulation), "K/W  film", flat(film), "K/W");
  out.writeln("heat loss", 60.0 / total, "W");
}
