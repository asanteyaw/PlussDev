<!-- generated from DynamicsOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::DynamicsOptions`

Controls of the dynamics functions.

## Signature

```cpp
robot::DynamicsOptions
```

## What It Does

Holds the gravity vector that the dynamics functions use.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `gravity` | The gravitational acceleration as a vector in the base frame, a real floating array of three components, in metres per second squared. Undefined means `(0, 0, -9.81)`. | `(0, 0, -9.81)` |

## Returns

A value; read by `torques`, `bias`, `accel` and `pe`.

## Errors

The functions throw `pluss::Error` for a gravity that is not a real floating vector of three components.

## Interpretation and Use

The default points along the negative z axis. A robot that moves in the x-y plane, as in the guide, needs gravity along negative y. Zero gravity isolates the velocity forces.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Earth, the moon and no gravity.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = rb::make(arm_links());
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0, 0});
  out.writeln("earth", flat(rb::bias(robot, q, still, in_plane())));
  rb::DynamicsOptions moon;
  moon.gravity = vec({0.0, -1.62, 0.0});
  out.writeln("moon ", flat(rb::bias(robot, q, still, moon)));
  rb::DynamicsOptions none;
  none.gravity = vec({0.0, 0.0, 0.0});
  out.writeln("none ", flat(rb::bias(robot, q, still, none)));
  // the default is gravity along -z: for a robot moving in the x-y plane it does nothing
  out.writeln("default gravity (0, 0, -9.81) on the planar arm", flat(rb::bias(robot, q, still)));
}
```

Output

```text
earth [14.9088, 3.6142]
moon  [2.4620, 0.5968]
none  [0.0000, 0.0000]
default gravity (0, 0, -9.81) on the planar arm [0.0000, 0.0000]
```
### Example 2

The gravity torque of a planar arm.

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
  const pluss::Series q = vec({0.4, -0.8});
  // at rest only gravity remains; with joint rates the Coriolis and centrifugal forces add
  out.writeln("bias at rest   ", flat(rb::bias(robot, q, vec({0, 0}), in_plane())));
  out.writeln("bias moving    ", flat(rb::bias(robot, q, vec({1.3, -0.7}), in_plane())));
  // without gravity the bias is velocity forces alone, and doubling the rates quadruples them
  rb::DynamicsOptions none;
  none.gravity = vec({0.0, 0.0, 0.0});
  out.writeln("velocity forces", flat(rb::bias(robot, q, vec({1.3, -0.7}), none)));
  out.writeln("at twice the rates", flat(rb::bias(robot, q, vec({2.6, -1.4}), none)));
}
```

Output

```text
bias at rest    [14.90875, 3.61424]
bias moving     [14.71794, 3.37178]
velocity forces [-0.19082, -0.24247]
at twice the rates [-0.76327, -0.96987]
```
### Example 3

Holding torques.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="torques.html">
    <span class="label">Next</span>
    <span class="title">torques</span>
  </a>
</div>
<!-- apiref:nav:end -->
