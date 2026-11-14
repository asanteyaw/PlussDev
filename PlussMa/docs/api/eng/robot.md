<!-- generated from robot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robotics

Rigid-body robots described as a tree of links with revolute and prismatic joints: frames, Jacobians and inverse kinematics, and the dynamics of a motion by recursive Newton-Euler. A robot is described once as `Links`, compiled by `make` into a `Robot`, and every call is vectorized over a batch of configurations.

## Choosing A Surface

| Task | Branch |
|---|---|
| Describe and compile a robot, rotation matrices | [Robot model](robot/model.md) |
| Frames, Jacobians and inverse kinematics | [Robot kinematics](robot/kinematics.md) |
| Torques, inertia, accelerations and energies | [Robot dynamics](robot/dynamics.md) |
| Pins, contacts and constraint forces | [Constraints and contacts](robot/constraints.md) |

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

The torques of a planar arm.

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
  const pluss::Series q = vec({0.4, -0.8}), qd = vec({1.3, -0.7}), qdd = vec({0.9, -1.4});
  const pluss::Series tau = rb::torques(robot, q, qd, qdd, in_plane());
  out.writeln("joint torques", flat(tau));
  // the same torques from the parts: M qdd + bias
  const pluss::Series parts = rb::inertia(robot, q).matmul(qdd.unsqueeze(-1)).squeeze(-1).add(rb::bias(robot, q, qd, in_plane()));
  out.writeln("M qdd + bias   ", flat(parts));
  // holding still: only gravity
  out.writeln("holding the pose", flat(rb::torques(robot, q, vec({0, 0}), vec({0, 0}), in_plane())));
}
```

Output

```text
joint torques [15.24292, 3.44218]
M qdd + bias    [15.24292, 3.44218]
holding the pose [14.90875, 3.61424]
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
:maxdepth: 3
:hidden:

robot/model
robot/kinematics
robot/dynamics
robot/constraints
```
<!-- apiref:toctree:end -->
