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
  out.print_defaults(PrintOptions{}.fixed(6));
  const rb::Robot robot = slider();
  // a slider resting on the floor stays there: simulate a second of semi-implicit Euler under gravity and a small push up
  pluss::Series q = vec({0.0}).reshape({1}), v = q;
  const rb::Contacts floor = floor_contact();
  const double h = 1e-3;
  for (int k = 0; k <= 1000; ++k) {
    const rb::ConstrainedResult step = rb::constrain(robot, floor, rb::Pins{}, q, v, vec({5.0}).reshape({1}), earth_in_plane());
    if (k % 250 == 0) out.writeln(k * h, "s  height", flat(q)[0], " speed", flat(v)[0], " floor force", flat(step.contact_force)[0]);
    v = v.add(step.acceleration.mul(h));
    q = q.add(v.mul(h));
  }
}
