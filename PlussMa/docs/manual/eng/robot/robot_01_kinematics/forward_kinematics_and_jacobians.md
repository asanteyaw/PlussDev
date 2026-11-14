<!-- generated from forward_kinematics_and_jacobians.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Forward kinematics and Jacobians

[Back to Robotics 1: Kinematics](../robot_01_kinematics.md)

Given the joint values, **forward kinematics** finds where every link is. Given also the joint rates, the **Jacobian** finds how fast a point on a link moves. This page places the links of the planar arm against closed forms, evaluates a batch of configurations in one call, follows a branching tree with a sliding joint, and builds Jacobians.

## The mental model

`frames(robot, q)` composes the transforms down the tree. For link `i` with parent `p`, the world rotation is `R_i = R_p R_off Rot(axis, q_i)` and the world position of its frame origin is `x_i = x_p + R_p d_i`, where `d_i` is the link's offset in its parent frame (for a prismatic joint, the offset includes the sliding `axis q_i`). The result is a `Pose`: `rotation` `[..., links, 3, 3]` and `position` `[..., links, 3]`.

`jac(robot, q, link)` is the matrix `J` with `v = J qd`: the world linear velocity of the link's origin and its angular velocity for joint rates `qd`. It has shape `[..., 6, links]`. For a revolute joint `j` on the path to the link, column `j` is `[z_j x (p - o_j); z_j]` with `z_j` the joint axis in the world; for a prismatic joint it is `[z_j; 0]`; for every other joint it is zero.

## The planar arm

At shoulder `0.6` and elbow `-0.9` radians the elbow joint is at `(l1 cos q1, l1 sin q1)`, and the forearm is turned by `q1 + q2` about z. The program prints the shapes, the positions, and the closed-form elbow.

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

The elbow is at `(0.41267, 0.28232)`, equal to the closed form, and the forearm frame has cosine `0.95534` and sine `-0.29552`, which are those of `q1 + q2 = -0.3`.

## Many configurations at once

A leading axis on `q` is a batch, and the whole batch is one vectorized call. The program passes three configurations: straight out, a bent pose, and the shoulder raised a quarter turn.

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
  // three configurations at once: a leading axis on q is a batch
  const pluss::Series q = mat({0.0, 0.0, 0.6, -0.9, M_PI / 2, 0.0}, {3, 2});
  const rb::Pose pose = rb::frames(robot, q);
  out.writeln("position shape", pose.position.sizes(), " (configuration, link, xyz)");
  out.writeln("elbow x for each configuration", flat(pose.position.select(1, 1).select(-1, 0)));
  out.writeln("elbow y for each configuration", flat(pose.position.select(1, 1).select(-1, 1)));
}
```

Output

```text
position shape [3, 2, 3]  (configuration, link, xyz)
elbow x for each configuration [0.50000, 0.41267, 0.00000]
elbow y for each configuration [0.00000, 0.28232, 0.50000]
```

The pose has shape `[3, 2, 3]`, configuration then link then coordinate, and the elbow's x coordinates `0.5, 0.41, 0.0` and y coordinates `0.0, 0.28, 0.5` are the three configurations' elbows.

## A tree with a sliding joint

In the branching tree, moving a joint moves every link below it and no other. The program prints the link positions, then slides the rail (link 2) and turns the shoulder (link 1) and reports how far the wrist (link 3, a child of the rail) moves each time.

```cpp
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
```

Output

```text
position shape [4, 3]
link 0 [0.00000, 0.00000, 0.30000]
link 1 [0.19107, 0.05910, 0.40000]
link 2 [0.04657, 0.01441, 0.42496]
link 3 [0.01796, 0.00556, 0.72346]
wrist moved by sliding the rail [0.47528, 0.14702, 0.04992]
wrist moved by turning the shoulder [0.00000, 0.00000, 0.00000]
```

Sliding the rail by `0.5 m` moves the wrist by `(0.475, 0.147, 0.050)`, a vector of length `0.5` along the rail's slide direction, and turning the shoulder, which is on another branch, does not move the wrist at all.

## The Jacobian of the planar arm

For the elbow of the planar arm the shoulder column is the velocity of the elbow per unit shoulder rate: perpendicular to the upper arm and of magnitude `0.5`. The elbow joint's column has no linear part, because the elbow does not move when the elbow joint turns, and both columns carry a unit angular part about z.

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

The shoulder column is `(-0.28232, 0.41267, 0)` with angular part `(0, 0, 1)`, the elbow column is zero except for the angular part, and shoulder rates of `(1, 0)` rad/s move the elbow at `(-0.282, 0.413, 0)` m/s.

## Jacobians of a tree, and of a batch

On the tree the wrist depends only on joints 0, 2 and 3 (the path from the base), so joint 1's column is all zeros, and the prismatic joint's column has a linear part only.

```cpp
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
```

Output

```text
wrist jacobian columns (joint 1, the shoulder branch, is not an ancestor)
joint 0 [-0.00556, 0.01796, 0.00000, 0.00000, 0.00000, 1.00000]
joint 1 [0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000]
joint 2 [0.95056, 0.29404, 0.09983, 0.00000, 0.00000, 0.00000]
joint 3 [0.00000, -0.00000, 0.00000, 0.95056, 0.29404, -0.09983]
shoulder jacobian, joint 2 (the rail) column [0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000]
rail column of the wrist, angular part [0.00000, 0.00000, 0.00000]
```

Joint 1 (the shoulder branch) has a zero column, the rail column is `(0.95, 0.29, 0.10)` with no angular part, and the wrist joint's column has an angular part only because the wrist origin is on its own axis. The Jacobian of a batch has shape `[configurations, 6, links]`.

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
  const pluss::Series q = mat({0.0, 0.0, 0.6, -0.9, M_PI / 2, 0.0}, {3, 2});
  const pluss::Series jac = rb::jac(robot, q, 1);
  out.writeln("jacobian shape", jac.sizes(), " (configuration, row, joint)");
  out.writeln("linear part of joint 0 for each configuration (x then y)");
  out.writeln(flat(jac.select(2, 0).slice(1, 0, 2)));
}
```

Output

```text
jacobian shape [3, 6, 2]  (configuration, row, joint)
linear part of joint 0 for each configuration (x then y)
[0.00000, 0.50000, -0.28232, 0.41267, -0.50000, 0.00000]
```

## Errors

A configuration of the wrong width or dtype, and a link that is not in the robot, are rejected with messages that name the argument.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const rb::Robot robot = rb::make(arm_links());
  report([&] { (void)rb::frames(robot, vec({0.1, 0.2, 0.3})); });
  report([&] { (void)rb::frames(robot, pluss::series::make(std::vector<std::int64_t>{1, 2})); });
  report([&] { (void)rb::frames(robot, pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
}
```

Output

```text
eng::robot::frames: q must be a real floating [..., 2] array
eng::robot::frames: q must be a real floating [..., 2] array
eng::robot::frames: q must be a real floating [..., 2] array
```

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const rb::Robot robot = rb::make(arm_links());
  report([&] { (void)rb::jac(robot, vec({0.1, 0.2}), 2); });
  report([&] { (void)rb::jac(robot, vec({0.1, 0.2}), -1); });
  report([&] { (void)rb::jac(robot, vec({0.1}), 1); });
}
```

Output

```text
eng::robot::jac: link is outside the robot
eng::robot::jac: link is outside the robot
eng::robot::jac: q must be a real floating [..., 2] array
```

## Decisions

- **Positions and orientations of every link:** `frames`; select one link with `select(-3, link)` on `rotation` and `select(-2, link)` on `position`.
- **Velocities, forces at the tip, singularities:** `jac`; its rank drops at a singularity.
- **A trajectory of poses:** pass the whole trajectory as one batched `q`.
- **Sensitivities of a pose to a link length or an angle:** differentiate through the call; the robot's compiled values and `q` carry reverse sensitivity.

## Pitfalls

The program makes three mistakes with the planar arm: giving the joint values in the wrong order, applying the Jacobian to rates in degrees per second, and expecting a Jacobian column for a joint that is not on the path to the link asked about.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Links arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  return links;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = rb::make(arm());
  const pluss::Series q = vec({0.6, -0.9});
  const pluss::Series jac = rb::jac(robot, q, 1);

  // 1. configurations are in link order: swapping the entries turns the other joint
  out.writeln("elbow with q = (0.6, -0.9)", flat(rb::frames(robot, q).position.select(0, 1)));
  out.writeln("elbow with q = (-0.9, 0.6)", flat(rb::frames(robot, vec({-0.9, 0.6})).position.select(0, 1)));

  // 2. the Jacobian maps radians per second: 10 degrees per second is 0.1745 rad/s
  const double degrees = 10.0, radians = degrees * M_PI / 180.0;
  out.writeln("elbow speed for 10 as if rad/s", flat(jac.slice(0, 0, 3).matmul(vec({degrees, 0.0}).unsqueeze(-1)))[0], flat(jac.slice(0, 0, 3).matmul(vec({degrees, 0.0}).unsqueeze(-1)))[1]);
  out.writeln("elbow speed for 10 degrees per second  ", flat(jac.slice(0, 0, 3).matmul(vec({radians, 0.0}).unsqueeze(-1)))[0], flat(jac.slice(0, 0, 3).matmul(vec({radians, 0.0}).unsqueeze(-1)))[1]);

  // 3. the Jacobian of link 0 only sees joint 0: the forearm joint is not on the path to the shoulder frame
  out.writeln("Jacobian of link 0, column of joint 1", flat(rb::jac(robot, q, 0).select(1, 1)));
}
```

Output

```text
elbow with q = (0.6, -0.9) [0.4127, 0.2823, 0.0000]
elbow with q = (-0.9, 0.6) [0.3108, -0.3917, 0.0000]
elbow speed for 10 as if rad/s -2.8232 4.1267
elbow speed for 10 degrees per second   -0.0493 0.0720
Jacobian of link 0, column of joint 1 [0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000]
```

Swapping the two entries of `q` puts the elbow at `(0.311, -0.392)` instead of `(0.413, 0.282)`. Ten degrees per second typed as `10` moves the elbow at `(-2.82, 4.13)` m/s, sixty times too fast; as `0.1745` rad/s it moves at `(-0.049, 0.072)` m/s. And the Jacobian of link 0 has an all-zero column for joint 1, because the forearm joint is not on the path from the base to the shoulder frame.

## Where next

[Inverse kinematics](ik.md) solves the opposite problem. Reference pages: [`frames`](../../../../api/eng/robot/kinematics/frames.md), [`jac`](../../../../api/eng/robot/kinematics/jac.md), [`Pose`](../../../../api/eng/robot/kinematics/Pose.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describing_a_robot.html">
    <span class="label">Previous</span>
    <span class="title">Describing a robot</span>
  </a>
  <a class="next-link" href="ik.html">
    <span class="label">Next</span>
    <span class="title">Inverse kinematics</span>
  </a>
</div>
<!-- apiref:nav:end -->
