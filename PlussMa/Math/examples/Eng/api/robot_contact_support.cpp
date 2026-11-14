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
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = slider();
  // the floor carries what the pull does not: force = max(0, weight - pull); it never pulls the slider down
  std::vector<double> pulls{0, 5, 10, 19.62, 25, 40};
  const pluss::Series tau = vec(pulls).reshape({6, 1});
  const pluss::Series q = pluss::series::zeros({6, 1}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const rb::ConstrainedResult result = rb::constrain(robot, floor_contact(), rb::Pins{}, q, q, tau, earth_in_plane());
  const std::vector<double> force = flat(result.contact_force), acceleration = flat(result.acceleration);
  out.writeln("  pull    floor force   acceleration");
  for (int k = 0; k < 6; ++k) out.writeln(pulls[k], force[k], acceleration[k]);
}
