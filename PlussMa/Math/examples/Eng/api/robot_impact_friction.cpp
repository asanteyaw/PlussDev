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
static rb::Contacts floor_contact(double friction = 0.0) {
  rb::Contacts floor;
  floor.link = {1};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  if (friction > 0.0) floor.friction = vec({friction});
  return floor;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  const pluss::Series q = vec({0.0, 0.0}), v = vec({2.0, -1.0});
  // landing at 2 m/s sideways and 1 m/s down with a plastic impact: friction removes up to mu x the normal impulse of tangential speed
  out.writeln("friction  velocity after (x, y)   normal impulse  friction impulse");
  for (double mu : {0.0, 0.5, 1.0, 2.0}) {
    const rb::ImpactResult hit = rb::impact(robot, floor_contact(mu), rb::Pins{}, q, v, 0.0);
    out.writeln(mu, flat(hit.velocity)[0], flat(hit.velocity)[1], flat(hit.contact_impulse)[0], flat(hit.friction_impulse)[0]);
  }
  out.writeln("sticks when mu >= 2: the sideways speed 2 needs a tangential impulse m v_t = 4 = mu x the normal impulse 2 x 1");
}
