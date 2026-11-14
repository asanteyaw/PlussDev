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
  // the same arm with the forearm mass doubled: the gravity torque at the shoulder grows
  const pluss::Series q = vec({0.3, 0.5});
  const rb::Robot light = rb::make(arm_links());
  auto heavy_links = arm_links();
  heavy_links.mass = vec({1.0, 4.0});
  const rb::Robot heavy = rb::make(heavy_links);
  out.writeln("gravity torque, forearm 2 kg", flat(rb::bias(light, q, vec({0, 0}), in_plane())));
  out.writeln("gravity torque, forearm 4 kg", flat(rb::bias(heavy, q, vec({0, 0}), in_plane())));
  // omitted center and inertia: a point mass at the link origin
  auto point = arm_links();
  point.center = pluss::Series{};
  point.inertia = pluss::Series{};
  out.writeln("gravity torque, point masses at the joints", flat(rb::bias(rb::make(point), q, vec({0, 0}), in_plane())));
}
