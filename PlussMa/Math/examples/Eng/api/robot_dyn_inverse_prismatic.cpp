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
  // a cart on a rail carrying a pendulum: the cart joint slides along x, the pendulum turns about z
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 0};
  links.axis = mat({1, 0, 0, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({1.5, 0.7});
  links.center = mat({0, 0, 0, 0.6, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01}, {2, 3, 3});
  const rb::Robot robot = rb::make(links);
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});
  const double th = 0.9, dth = 1.7, ax = 0.4, ath = -2.2;
  const pluss::Series tau = rb::torques(robot, vec({0.3, th}), vec({-0.8, dth}), vec({ax, ath}), options);
  // closed forms of the cart-pole equations
  const double m = 0.7, M = 1.5, l = 0.6, spin = 0.01;
  out.writeln("force on the cart   ", flat(tau)[0], " closed form", (M + m) * ax - m * l * std::sin(th) * ath - m * l * std::cos(th) * dth * dth);
  out.writeln("torque on the pole  ", flat(tau)[1], " closed form", -m * l * std::sin(th) * ax + (m * l * l + spin) * ath + m * 9.81 * l * std::cos(th));
}
