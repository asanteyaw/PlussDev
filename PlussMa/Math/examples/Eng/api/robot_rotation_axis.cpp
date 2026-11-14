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
  // a quarter turn about z maps the x axis onto the y axis
  const pluss::Series quarter = rb::rotation(vec({0, 0, 1}), vec({M_PI / 2}).select(0, 0));
  out.writeln("rotation shape", quarter.sizes());
  out.writeln("R x", flat(quarter.matmul(vec({1, 0, 0}).unsqueeze(-1))));
  // the axis need not be a unit vector
  out.writeln("same rotation from the axis (0, 0, 5)", flat(rb::rotation(vec({0, 0, 5}), vec({M_PI / 2}).select(0, 0)).matmul(vec({1, 0, 0}).unsqueeze(-1))));
}
