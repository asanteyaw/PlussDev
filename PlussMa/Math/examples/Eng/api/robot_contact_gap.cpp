#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a vertical slider of mass 2 kg (a prismatic joint along y) with a point contact on the floor y >= 0
static rb::Robot slider() {
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {1};
  links.axis = mat({0, 1, 0}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  links.mass = vec({2.0});
  return rb::make(links);
}
static rb::Contacts floor_contact() {
  rb::Contacts floor;
  floor.link = {0};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  return floor;
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = slider();
  // the gap is n . x - offset: positive above the plane, negative below
  out.writeln("gap at heights -0.1, 0, 0.3", flat(rb::gap(robot, floor_contact(), mat({-0.1, 0.0, 0.3}, {3, 1}))));
  // a tilted plane x + y >= 1 through the same point: the normal and the offset define it
  rb::Contacts tilted = floor_contact();
  tilted.normal = mat({std::sqrt(0.5), std::sqrt(0.5), 0}, {1, 3});
  tilted.offset = vec({std::sqrt(0.5)});
  out.writeln("gap of the slider at y = 1 against the tilted plane", flat(rb::gap(robot, tilted, vec({1.0}).reshape({1, 1}))));
  // the contact point is in the link frame: a point 0.2 m below the slider's origin touches when the origin is at 0.2
  rb::Contacts foot = floor_contact();
  foot.point = mat({0, -0.2, 0}, {1, 3});
  out.writeln("gap with a foot 0.2 m below", flat(rb::gap(robot, foot, mat({0.2, 0.5}, {2, 1}))));
}
