#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a block of mass 2 kg free to move in the x-y plane: two prismatic joints (x, then y), the block on the second link
static rb::Robot block() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 1};
  links.axis = mat({1, 0, 0, 0, 1, 0}, {2, 3});
  links.translation = mat({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({0.0, 2.0});
  return rb::make(links);
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}
static rb::Contacts slope(double degrees, double friction) {
  // an incline through the origin rising to the right: the contact point is the block's origin
  const double t = degrees * M_PI / 180.0;
  rb::Contacts contact;
  contact.link = {1};
  contact.point = mat({0, 0, 0}, {1, 3});
  contact.normal = mat({-std::sin(t), std::cos(t), 0}, {1, 3});
  contact.offset = vec({0.0});
  if (friction > 0.0) contact.friction = vec({friction});
  return contact;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  const pluss::Series q = vec({0.0, 0.0}), rest = vec({0.0, 0.0});
  // a block on a 30 degree slope: it slides when tan(30) = 0.577 exceeds the friction coefficient
  out.writeln("friction  acceleration (x, y)         normal force  friction force");
  for (double mu : {0.0, 0.3, 0.5, 0.6, 0.8}) {
    const rb::ConstrainedResult r = rb::constrain(robot, slope(30.0, mu), rb::Pins{}, q, rest, rest, earth_in_plane());
    const std::vector<double> a = flat(r.acceleration), f = flat(r.friction_force);
    out.writeln(mu, a[0], a[1], flat(r.contact_force)[0], std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]));
  }
  out.writeln("weight components: normal m g cos 30 =", 2 * 9.81 * std::cos(M_PI / 6), " tangential m g sin 30 =", 2 * 9.81 * std::sin(M_PI / 6));
}
