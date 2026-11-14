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
  // points fixed to links, in the link frames: the forearm tip and the middle of the upper arm
  const pluss::Series points = mat({0.4, 0.0, 0.0, 0.25, 0.0, 0.0}, {2, 3});
  const pluss::Series where = rb::locate(robot, vec({0.6, -0.9}), {1, 0}, points);
  out.writeln("shape", where.sizes(), " (point, xyz)");
  out.writeln("forearm tip", flat(where.select(0, 0)), " closed form", 0.5 * std::cos(0.6) + 0.4 * std::cos(-0.3), 0.5 * std::sin(0.6) + 0.4 * std::sin(-0.3));
  out.writeln("middle of the upper arm", flat(where.select(0, 1)));
  // a batch of configurations
  const pluss::Series batch = rb::locate(robot, mat({0, 0, 0.6, -0.9}, {2, 2}), {1}, mat({0.4, 0, 0}, {1, 3}));
  out.writeln("batched shape", batch.sizes(), " (configuration, point, xyz)");
}
