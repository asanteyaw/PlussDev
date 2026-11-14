#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a trunk (link 0, about z) with a shoulder (link 1, about y), a rail (link 2, a slide along x) and a wrist (link 3, about x) on the rail
static rb::Links tree_links() {
  rb::Links links;
  links.parent = {-1, 0, 0, 2};
  links.prismatic = {0, 0, 1, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0}, {4, 3});
  links.translation = mat({0, 0, 0.3, 0.2, 0, 0.1, -0.2, 0, 0.1, 0, 0, 0.3}, {4, 3});
  links.rpy = mat({0, 0, 0, 0.2, 0, 0.1, 0, -0.1, 0, 0.3, 0.2, 0}, {4, 3});
  return links;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(tree_links());
  const pluss::Series q = vec({0.3, -0.5, 0.25, 0.8});
  const pluss::Series wrist = rb::jac(robot, q, 3);
  const pluss::Series shoulder = rb::jac(robot, q, 1);
  out.writeln("wrist jacobian columns (joint 1, the shoulder branch, is not an ancestor)");
  for (int joint = 0; joint < 4; ++joint) out.writeln("joint", joint, flat(wrist.select(1, joint)));
  out.writeln("shoulder jacobian, joint 2 (the rail) column", flat(shoulder.select(1, 2)));
  // the prismatic column has a linear part only
  out.writeln("rail column of the wrist, angular part", flat(wrist.select(1, 2).slice(0, 3, 6)));
}
