#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(arm_links());
  const pluss::Series q = vec({0.4, -0.8}), qd = vec({1.3, -0.7});
  // released from rest the arm falls: the accelerations are those that gravity produces
  out.writeln("free acceleration", flat(rb::accel(robot, q, vec({0, 0}), vec({0, 0}), in_plane())));
  // the torques that hold the pose give zero acceleration
  const pluss::Series hold = rb::torques(robot, q, vec({0, 0}), vec({0, 0}), in_plane());
  out.writeln("holding torques", flat(hold));
  out.writeln("acceleration under them", flat(rb::accel(robot, q, vec({0, 0}), hold, in_plane())));
  // forward and inverse dynamics undo each other
  const pluss::Series wanted = vec({0.9, -1.4});
  const pluss::Series tau = rb::torques(robot, q, qd, wanted, in_plane());
  out.writeln("recovered acceleration", flat(rb::accel(robot, q, qd, tau, in_plane())));
}
