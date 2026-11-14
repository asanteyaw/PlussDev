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
  const rb::Pose pose = rb::frames(robot, vec({0.3, -0.5, 0.25, 0.8}));
  out.writeln("position shape", pose.position.sizes());
  for (int link = 0; link < 4; ++link) out.writeln("link", link, flat(pose.position.select(0, link)));
  // the wrist (link 3) hangs from the rail (link 2): sliding the rail moves it, rotating the shoulder (link 1) does not
  const rb::Pose slid = rb::frames(robot, vec({0.3, -0.5, 0.75, 0.8}));
  const rb::Pose turned = rb::frames(robot, vec({0.3, 1.5, 0.25, 0.8}));
  out.writeln("wrist moved by sliding the rail", flat(slid.position.select(0, 3).sub(pose.position.select(0, 3))));
  out.writeln("wrist moved by turning the shoulder", flat(turned.position.select(0, 3).sub(pose.position.select(0, 3))));
}
