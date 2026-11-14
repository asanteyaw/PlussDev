#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // one axis and three angles, and three axes with one angle
  const pluss::Series angles = vec({0.0, M_PI / 4, M_PI / 2});
  const pluss::Series turns = rb::rotation(vec({0, 0, 1}), angles);
  out.writeln("angles batch shape", turns.sizes(), " (angle, row, column)");
  out.writeln("R(pi/4) first row", flat(turns.select(0, 1).select(0, 0)));
  const pluss::Series axes = mat({1, 0, 0, 0, 1, 0, 0, 0, 1}, {3, 3});
  const pluss::Series by_axis = rb::rotation(axes, vec({M_PI / 2}).select(0, 0));
  out.writeln("axes batch shape", by_axis.sizes());
  out.writeln("rotated z axis about each axis", flat(by_axis.matmul(vec({0, 0, 1}).unsqueeze(-1))));
}
