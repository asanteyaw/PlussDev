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
  out.print_defaults(PrintOptions{}.fixed(6));
  const rb::Robot robot = rb::make(arm_links());
  const auto energy = [&](const pluss::Series &q, const pluss::Series &qd) {
    return rb::ke(robot, q, qd).item<double>() + rb::pe(robot, q, in_plane()).item<double>();
  };
  const auto accel = [&](const pluss::Series &q, const pluss::Series &qd) {
    return rb::accel(robot, q, qd, vec({0, 0}), in_plane());
  };
  // the arm is released from rest and swings for one second (classical fourth-order Runge-Kutta, 1 ms steps)
  pluss::Series q = vec({0.3, 0.5}), v = vec({0.0, 0.0});
  const double e0 = energy(q, v);
  const double h = 1e-3;
  out.writeln("  t     q1       q2     total energy");
  for (int k = 0; k <= 1000; ++k) {
    if (k % 250 == 0) out.writeln(k * h, flat(q)[0], flat(q)[1], energy(q, v));
    const pluss::Series k1x = v, k1v = accel(q, v);
    const pluss::Series k2x = v.add(k1v.mul(h / 2)), k2v = accel(q.add(k1x.mul(h / 2)), k2x);
    const pluss::Series k3x = v.add(k2v.mul(h / 2)), k3v = accel(q.add(k2x.mul(h / 2)), k3x);
    const pluss::Series k4x = v.add(k3v.mul(h)), k4v = accel(q.add(k3x.mul(h)), k4x);
    q = q.add(k1x.add(k2x.mul(2)).add(k3x.mul(2)).add(k4x).mul(h / 6));
    v = v.add(k1v.add(k2v.mul(2)).add(k3v.mul(2)).add(k4v).mul(h / 6));
  }
  out.writeln("energy drift over the second below 1e-8", std::abs(energy(q, v) - e0) < 1e-8);
}
