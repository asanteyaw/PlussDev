#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  // a robot without mass properties is a kinematic model: kinematics work and dynamics are refused
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {0};
  links.axis = mat({0, 0, 1}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  const rb::Robot robot = rb::make(links);
  out.writeln("has mass", robot.has_mass());
  out.writeln("pose of the only link", robot.links());
  try {
    (void)rb::inertia(robot, vec({0.1}));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
