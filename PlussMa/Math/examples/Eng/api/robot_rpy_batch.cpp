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
  const pluss::Series angles = mat({0, 0, 0, 0, 0, M_PI / 2, M_PI / 2, 0, 0, 0.3, 0.2, 0.1}, {4, 3});
  const pluss::Series turns = rb::rpy(angles);
  out.writeln("rpy shape", turns.sizes(), " (batch, row, column)");
  out.writeln("trace of each rotation", flat(turns.select(-1, 0).select(-1, 0).add(turns.select(-2, 1).select(-1, 1)).add(turns.select(-2, 2).select(-1, 2))));
}
