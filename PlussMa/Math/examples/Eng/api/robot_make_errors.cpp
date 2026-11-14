#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
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
  auto self = arm_links();
  self.parent = {-1, 1};
  report([&] { (void)rb::make(self); });
  auto no_axis = arm_links();
  no_axis.axis = mat({0, 0, 1, 0, 0, 0}, {2, 3});
  report([&] { (void)rb::make(no_axis); });
  auto kind = arm_links();
  kind.prismatic = {0, 2};
  report([&] { (void)rb::make(kind); });
  auto negative = arm_links();
  negative.mass = vec({1.0, -2.0});
  report([&] { (void)rb::make(negative); });
  auto shape = arm_links();
  shape.translation = mat({0, 0, 0}, {1, 3});
  report([&] { (void)rb::make(shape); });
  auto asymmetric = arm_links();
  asymmetric.inertia = mat({0, 1, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  report([&] { (void)rb::make(asymmetric); });
  rb::Links lonely;
  lonely.parent = {-1};
  lonely.prismatic = {0};
  lonely.axis = mat({0, 0, 1}, {1, 3});
  lonely.translation = mat({0, 0, 0}, {1, 3});
  lonely.center = mat({0, 0, 0}, {1, 3});
  report([&] { (void)rb::make(lonely); });
}
