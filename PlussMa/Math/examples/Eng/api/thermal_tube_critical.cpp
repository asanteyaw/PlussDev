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
  // A thin pipe with a film outside: insulation first lowers the resistance until the outer radius passes k / h.
  const double k = 0.2, h = 10.0, length = 1.0;
  const pluss::Series r_out = vec({0.005, 0.01, 0.02, 0.04, 0.08});
  const pluss::Series wall = th::tube(vec({k}), vec({0.005}), r_out.slice(0, 1, 5), vec({length}));
  const pluss::Series film = th::film(vec({h}), r_out.mul(2.0 * M_PI * length));
  const std::vector<double> w = flat(wall), f = flat(film);
  out.writeln("critical radius k / h =", k / h, "m");
  const double radius[] = {0.01, 0.02, 0.04, 0.08};
  for (int i = 0; i < 4; ++i) out.writeln("outer radius", radius[i], "m  wall", w[i], " film", f[i + 1], " total", w[i] + f[i + 1], "K/W");
}
