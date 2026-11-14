#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = planar_arm();
  // the tip of the forearm is held at rest in the world: three constraint equations, three free force components
  rb::Pins tip;
  tip.link = {1};
  tip.point = mat({0.4, 0.0, 0.0}, {1, 3});
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0.0, 0.0}), tau = vec({1.0, 0.5});
  const rb::ConstrainedResult held = rb::constrain(robot, rb::Contacts{}, tip, q, still, tau, plane_gravity());
  out.writeln("tip position", flat(rb::locate(robot, q, tip.link, tip.point)));
  out.writeln("pin force", flat(held.pin_force), " pin force shape", held.pin_force.sizes());
  out.writeln("joint accelerations", flat(held.acceleration), " against free", flat(rb::accel(robot, q, still, tau, plane_gravity().dynamics)));
  // a tiny step along the accelerations does not move the tip; along the free accelerations it does
  const double eps = 1e-4;
  const std::vector<double> p0 = flat(rb::locate(robot, q, tip.link, tip.point));
  const std::vector<double> p1 = flat(rb::locate(robot, q.add(held.acceleration.mul(eps)), tip.link, tip.point));
  out.writeln("tip displacement along the constrained accelerations below 1e-9", std::abs(p1[0] - p0[0]) < 1e-9 && std::abs(p1[1] - p0[1]) < 1e-9);
}
