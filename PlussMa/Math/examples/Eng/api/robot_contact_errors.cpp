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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const rb::Robot robot = slider();
  const pluss::Series q = vec({0.0}).reshape({1, 1});
  report([&] { (void)rb::gap(robot, rb::Contacts{{3}, mat({0, 0, 0}, {1, 3}), mat({0, 1, 0}, {1, 3}), vec({0.0})}, q); });
  report([&] { (void)rb::gap(robot, rb::Contacts{{0}, mat({0, 0}, {1, 2}), mat({0, 1, 0}, {1, 3}), vec({0.0})}, q); });
  report([&] { (void)rb::gap(robot, rb::Contacts{{0}, mat({0, 0, 0}, {1, 3}), mat({0, 1}, {1, 2}), vec({0.0})}, q); });
  rb::ConstraintOptions bad = earth_in_plane();
  bad.contact_threshold = -1.0;
  report([&] { (void)rb::constrain(robot, floor_contact(), rb::Pins{}, q, q, q, bad); });
  const rb::Robot arm = planar_arm();
  rb::Links massless;
  massless.parent = {-1};
  massless.prismatic = {0};
  massless.axis = mat({0, 0, 1}, {1, 3});
  massless.translation = mat({0, 0, 0}, {1, 3});
  report([&] { (void)rb::constrain(rb::make(massless), rb::Contacts{}, rb::Pins{}, q, q, q); });
}
