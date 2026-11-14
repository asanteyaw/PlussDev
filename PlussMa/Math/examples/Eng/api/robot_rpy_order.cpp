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
  // roll about x, then pitch about y, then yaw about z, all about the fixed axes: R = Rz Ry Rx
  const pluss::Series angles = vec({0.4, -0.7, 1.3});
  const pluss::Series product = rb::rotation(vec({0, 0, 1}), angles.select(0, 2))
                                    .matmul(rb::rotation(vec({0, 1, 0}), angles.select(0, 1)))
                                    .matmul(rb::rotation(vec({1, 0, 0}), angles.select(0, 0)));
  const pluss::Series direct = rb::rpy(angles);
  out.writeln("rpy first row   ", flat(direct.select(0, 0)));
  out.writeln("Rz Ry Rx row    ", flat(product.select(0, 0)));
  out.writeln("largest difference below 1e-12", direct.sub(product).abs().amax().item<double>() < 1e-12);
  // pure yaw is a rotation about z
  out.writeln("yaw 90 degrees, R x", flat(rb::rpy(vec({0, 0, M_PI / 2})).matmul(vec({1, 0, 0}).unsqueeze(-1))));
}
