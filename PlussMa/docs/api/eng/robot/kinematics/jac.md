<!-- generated from jac.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::jac`

The geometric Jacobian of a link.

## Signature

```cpp
pluss::Series robot::jac(const robot::Robot& robot, const pluss::Series& q, std::int64_t link);
```

## What It Does

Returns, for the origin of `link`'s frame, how its world velocity depends on the joint rates. Column `j` is `[z_j x (p - o_j); z_j]` for a revolute joint `j` that is an ancestor of the link (or the link itself), and `[z_j; 0]` for a prismatic one, where `z_j` is the joint axis in the world, `o_j` the joint's origin and `p` the link's origin. A joint that is not on the path from the base to the link has a zero column.

## Parameters

`robot`
: The compiled robot.

`q`
: The joint values, a real floating `[..., links]` array.

`link`
: The index of the link, in `[0, links)`.

## Returns

A real `[..., 6, links]` array: three rows of linear velocity then three of angular velocity per unit joint rate, in the world frame.

## Errors

Throws `pluss::Error` for a `link` outside the robot, or a `q` that is not a real floating `[..., links]` array.

## Interpretation and Use

Multiply by joint rates to get the link's spatial velocity, by the transpose with a force to get the joint torques that balance it, and use it in inverse kinematics. Every row is expressed in the world frame, so a column for a joint of a branch not leading to `link` is zero.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The Jacobian of the elbow of a planar arm.

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
### Example 2

A tree with a prismatic joint and a non-ancestor column.

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
### Example 3

A batch of configurations.

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
### Example 4

Rejected links and configurations.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frames.html">
    <span class="label">Previous</span>
    <span class="title">frames</span>
  </a>
  <a class="next-link" href="IkOptions.html">
    <span class="label">Next</span>
    <span class="title">IkOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
