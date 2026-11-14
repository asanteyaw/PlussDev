<!-- generated from kinematics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robot kinematics

This branch places the links of a robot. `frames` gives the world frame of every link, `jac` the velocity map of one link, and `ik` the joint values that bring a link to a target. `Pose`, `IkOptions` and `IkResult` carry the inputs and results.

## Choosing A Surface

Use `frames` when the joint values are known, `jac` for velocities and forces at a configuration, and `ik` when the target is known and the joint values are wanted.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

The frames of a planar arm.

```cpp
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
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(arm_links());
  const pluss::Series q = vec({0.6, -0.9});
  const rb::Pose pose = rb::frames(robot, q);
  out.writeln("position shape", pose.position.sizes(), " rotation shape", pose.rotation.sizes());
  out.writeln("shoulder, elbow positions", flat(pose.position));
  out.writeln("elbow closed form (l1 cos q1, l1 sin q1)", 0.5 * std::cos(0.6), 0.5 * std::sin(0.6));
  out.writeln("forearm rotation about z is q1 + q2 =", 0.6 - 0.9, " cos and sin", flat(pose.rotation.select(0, 1))[0], flat(pose.rotation.select(0, 1))[3]);
}
```

Output

```text
position shape [2, 3]  rotation shape [2, 3, 3]
shoulder, elbow positions [0.00000, 0.00000, 0.00000, 0.41267, 0.28232, 0.00000]
elbow closed form (l1 cos q1, l1 sin q1) 0.41267 0.28232
forearm rotation about z is q1 + q2 = -0.30000  cos and sin 0.95534 -0.29552
```
### Example 2

The Jacobian of the elbow.

```cpp
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
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(arm_links());
  const pluss::Series q = vec({0.6, -0.9});
  const pluss::Series jac = rb::jac(robot, q, 1);
  out.writeln("jacobian shape", jac.sizes(), " (linear xyz then angular xyz, joint)");
  out.writeln("column of joint 0 (the elbow turns about the shoulder at distance 0.5)", flat(jac.select(1, 0)));
  out.writeln("column of joint 1 (the elbow does not move when the elbow joint turns)", flat(jac.select(1, 1)));
  // joint rates (1, 0) rad/s move the elbow at 0.5 m/s, perpendicular to the upper arm
  out.writeln("elbow velocity for rates (1, 0)", flat(jac.slice(0, 0, 3).matmul(vec({1.0, 0.0}).unsqueeze(-1))));
}
```

Output

```text
jacobian shape [6, 2]  (linear xyz then angular xyz, joint)
column of joint 0 (the elbow turns about the shoulder at distance 0.5) [-0.28232, 0.41267, 0.00000, 0.00000, 0.00000, 1.00000]
column of joint 1 (the elbow does not move when the elbow joint turns) [0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 1.00000]
elbow velocity for rates (1, 0) [-0.28232, 0.41267, 0.00000]
```
### Example 3

A target reached by inverse kinematics.

```cpp
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
  // choose a reachable target by running the forward kinematics on a known configuration
  const pluss::Series truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const pluss::Series goal = rb::frames(robot, truth).position.select(0, 5);
  const rb::IkResult solved = rb::ik(robot, 5, goal, pluss::Series{}, vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  out.writeln("converged", solved.converged.item<bool>(), " iterations", solved.iterations.items<std::int64_t>()[0]);
  out.writeln("target       ", flat(goal));
  out.writeln("reached      ", flat(rb::frames(robot, solved.q).position.select(0, 5)));
  out.writeln("remaining error below 1e-8", solved.error.item<double>() < 1e-8);
}
```

Output

```text
converged true  iterations 4
target        [0.212893, 0.033385, 1.123725]
reached       [0.212893, 0.033385, 1.123725]
remaining error below 1e-8 true
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

kinematics/Pose
kinematics/frames
kinematics/jac
kinematics/IkOptions
kinematics/IkResult
kinematics/ik
```
<!-- apiref:toctree:end -->
