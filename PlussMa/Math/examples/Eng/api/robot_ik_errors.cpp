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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series start = vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1});
  report([&] { (void)rb::ik(robot, 6, vec({0.5, 0.0, 0.5}), pluss::Series{}, start); });
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0}), pluss::Series{}, start); });
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0, 0.5}), vec({1.0, 0.0, 0.0}), start); });
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0, 0.5}), pluss::Series{}, vec({0.1, 0.2})); });
  rb::IkOptions options;
  options.damping = 0.0;
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0, 0.5}), pluss::Series{}, start, options); });
}
