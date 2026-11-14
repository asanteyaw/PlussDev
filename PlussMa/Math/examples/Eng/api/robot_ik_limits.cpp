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
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series goal = rb::frames(robot, vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2})).position.select(0, 5);
  const pluss::Series start = vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1});
  // joint limits in radians, one pair per joint: the iteration never leaves them
  rb::IkOptions wide;
  wide.lower = vec({-1, -1, -1, -1, -1, -1});
  wide.upper = vec({1, 1, 1, 1, 1, 1});
  const rb::IkResult free = rb::ik(robot, 5, goal, pluss::Series{}, start, wide);
  out.writeln("wide limits: converged", free.converged.item<bool>(), " joint values", flat(free.q));
  rb::IkOptions tight;
  tight.lower = vec({-0.45, -0.45, -0.65, -0.35, -0.75, -0.25});
  tight.upper = vec({0.45, 0.55, 0.65, 0.35, 0.75, 0.25});
  const rb::IkResult boxed = rb::ik(robot, 5, goal, pluss::Series{}, start, tight);
  out.writeln("tight limits: converged", boxed.converged.item<bool>(), " joint values", flat(boxed.q));
  // limits that exclude the target: the nearest reachable configuration, flagged as not converged
  rb::IkOptions blocked;
  blocked.lower = vec({-0.1, -0.1, -0.1, -0.1, -0.1, -0.1});
  blocked.upper = vec({0.1, 0.1, 0.1, 0.1, 0.1, 0.1});
  const rb::IkResult stuck = rb::ik(robot, 5, goal, pluss::Series{}, vec({0, 0, 0, 0, 0, 0}), blocked);
  out.writeln("blocked: converged", stuck.converged.item<bool>(), " distance left", stuck.error.item<double>(), " joint values", flat(stuck.q));
}
