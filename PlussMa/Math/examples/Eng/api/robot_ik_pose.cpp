#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a six-joint arm: a column, then alternating shoulder, elbow and wrist joints
static rb::Links six_links() {
  rb::Links links;
  links.parent = {-1, 0, 1, 2, 3, 4};
  links.prismatic = {0, 0, 0, 0, 0, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0}, {6, 3});
  links.translation = mat({0, 0, 0.2, 0, 0, 0.1, 0, 0, 0.4, 0, 0, 0.3, 0, 0, 0.1, 0, 0, 0.1}, {6, 3});
  return links;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const rb::Pose goal = rb::frames(robot, truth);
  const rb::IkResult solved = rb::ik(robot, 5, goal.position.select(0, 5), goal.rotation.select(0, 5),
                                                      vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  const rb::Pose reached = rb::frames(robot, solved.q);
  out.writeln("converged", solved.converged.item<bool>(), " iterations", solved.iterations.items<std::int64_t>()[0]);
  out.writeln("position error below 1e-6", reached.position.select(0, 5).sub(goal.position.select(0, 5)).abs().amax().item<double>() < 1e-6);
  out.writeln("orientation error below 1e-6", reached.rotation.select(0, 5).sub(goal.rotation.select(0, 5)).abs().amax().item<double>() < 1e-6);
}
