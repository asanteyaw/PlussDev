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
  // 0.5 qd' M qd, from the mass matrix
  const pluss::Series m = rb::inertia(robot, q);
  out.writeln("kinetic energy  ", rb::ke(robot, q, qd).item<double>());
  out.writeln("0.5 qd' M qd    ", 0.5 * qd.unsqueeze(0).matmul(m).matmul(qd.unsqueeze(-1)).select(0, 0).select(0, 0).item<double>());
  // potential energy m g h summed over the link centres: with the arm in a vertical plane it falls as the arm lowers
  out.writeln("potential energy, arm raised", rb::pe(robot, vec({1.2, 0.3}), in_plane()).item<double>());
  out.writeln("potential energy, arm lowered", rb::pe(robot, vec({-1.2, -0.3}), in_plane()).item<double>());
  const pluss::Series batch = mat({1.2, 0.3, -1.2, -0.3}, {2, 2});
  out.writeln("batched potential energy shape", rb::pe(robot, batch, in_plane()).sizes());
}
