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
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = rb::make(arm_links());
  // a sampled motion q(t) = (0.5 sin t, 0.3 cos t): the torque history in one call
  std::vector<double> q, qd, qdd;
  for (int k = 0; k < 5; ++k) {
    const double t = 0.25 * k;
    q.insert(q.end(), {0.5 * std::sin(t), 0.3 * std::cos(t)});
    qd.insert(qd.end(), {0.5 * std::cos(t), -0.3 * std::sin(t)});
    qdd.insert(qdd.end(), {-0.5 * std::sin(t), -0.3 * std::cos(t)});
  }
  const pluss::Series tau = rb::torques(robot, mat(q, {5, 2}), mat(qd, {5, 2}), mat(qdd, {5, 2}), in_plane());
  out.writeln("torque shape", tau.sizes(), " (time sample, joint)");
  out.writeln("shoulder torque", flat(tau.select(1, 0)));
  out.writeln("elbow torque   ", flat(tau.select(1, 1)));
}
