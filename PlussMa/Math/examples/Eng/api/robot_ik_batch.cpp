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
  // two targets and two starting configurations solved together
  const pluss::Series truths = mat({0.4, 0.5, -0.6, 0.3, 0.7, -0.2, -0.3, 0.6, -0.4, 0.2, 0.5, 0.1}, {2, 6});
  const pluss::Series goals = rb::frames(robot, truths).position.select(1, 5);
  const pluss::Series starts = mat({0.3, 0.4, -0.5, 0.2, 0.6, -0.1, -0.2, 0.5, -0.3, 0.1, 0.4, 0.0}, {2, 6});
  const rb::IkResult solved = rb::ik(robot, 5, goals, pluss::Series{}, starts);
  out.writeln("q shape", solved.q.sizes(), " error shape", solved.error.sizes(), " converged shape", solved.converged.sizes());
  out.writeln("converged", flat(solved.converged.to(pluss::ScalarType::Float64)));
  out.writeln("iterations used by the batch", solved.iterations.items<std::int64_t>()[0]);
}
