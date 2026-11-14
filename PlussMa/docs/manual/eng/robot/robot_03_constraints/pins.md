<!-- generated from pins.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pins and closed chains

[Back to Robotics 3: Constraints and Contacts](../robot_03_constraints.md)

A pin holds a point of a link at rest in the world: the tip of an arm bolted to a wall, a joint to the ground. It removes three degrees of freedom and carries a force that the dynamics returns. This page pins the tip of the planar arm, shows that its accelerations no longer move the tip, reads the force the pin carries as the joint torque changes, and locates points with `locate`.

## The mental model

A pin on link `l` at `point` (in the link frame) is the constraint that the world acceleration of that point is zero: `J qdd + J' qd = 0`, where `J` is the point Jacobian and `J' qd` the bias acceleration (the acceleration of the point when the joint accelerations are zero). Its force is free in all three components. With the Delassus matrix of the pins the constraint force is solved together with the accelerations: `qdd = M^-1 (tau - bias + J^T f)`.

Because a pin constrains the acceleration, it holds the point where it is only if the point starts at rest in the world.

## Where the points are

`locate` gives the world position of points fixed to links, for any batch of configurations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = planar_arm();
  // points fixed to links, in the link frames: the forearm tip and the middle of the upper arm
  const pluss::Series points = mat({0.4, 0.0, 0.0, 0.25, 0.0, 0.0}, {2, 3});
  const pluss::Series where = rb::locate(robot, vec({0.6, -0.9}), {1, 0}, points);
  out.writeln("shape", where.sizes(), " (point, xyz)");
  out.writeln("forearm tip", flat(where.select(0, 0)), " closed form", 0.5 * std::cos(0.6) + 0.4 * std::cos(-0.3), 0.5 * std::sin(0.6) + 0.4 * std::sin(-0.3));
  out.writeln("middle of the upper arm", flat(where.select(0, 1)));
  // a batch of configurations
  const pluss::Series batch = rb::locate(robot, mat({0, 0, 0.6, -0.9}, {2, 2}), {1}, mat({0.4, 0, 0}, {1, 3}));
  out.writeln("batched shape", batch.sizes(), " (configuration, point, xyz)");
}
```

Output

```text
shape [2, 3]  (point, xyz)
forearm tip [0.79480, 0.16411, 0.00000]  closed form 0.79480 0.16411
middle of the upper arm [0.20633, 0.14116, 0.00000]
batched shape [2, 1, 3]  (configuration, point, xyz)
```

## A pinned tip

The tip of the forearm (`0.4 m` along the second link) is pinned. The program computes the accelerations with and without the pin, reads the force on the pin, and checks that a small step along the constrained accelerations does not move the tip.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = planar_arm();
  // the tip of the forearm is held at rest in the world: three constraint equations, three free force components
  rb::Pins tip;
  tip.link = {1};
  tip.point = mat({0.4, 0.0, 0.0}, {1, 3});
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0.0, 0.0}), tau = vec({1.0, 0.5});
  const rb::ConstrainedResult held = rb::constrain(robot, rb::Contacts{}, tip, q, still, tau, plane_gravity());
  out.writeln("tip position", flat(rb::locate(robot, q, tip.link, tip.point)));
  out.writeln("pin force", flat(held.pin_force), " pin force shape", held.pin_force.sizes());
  out.writeln("joint accelerations", flat(held.acceleration), " against free", flat(rb::accel(robot, q, still, tau, plane_gravity().dynamics)));
  // a tiny step along the accelerations does not move the tip; along the free accelerations it does
  const double eps = 1e-4;
  const std::vector<double> p0 = flat(rb::locate(robot, q, tip.link, tip.point));
  const std::vector<double> p1 = flat(rb::locate(robot, q.add(held.acceleration.mul(eps)), tip.link, tip.point));
  out.writeln("tip displacement along the constrained accelerations below 1e-9", std::abs(p1[0] - p0[0]) < 1e-9 && std::abs(p1[1] - p0[1]) < 1e-9);
}
```

Output

```text
tip position [0.82895, 0.03894, 0.00000]
pin force [-17.72312, 15.94608, 0.00000]  pin force shape [1, 3]
joint accelerations [-0.00000, 0.00000]  against free [-16.87126, 9.93144]
tip displacement along the constrained accelerations below 1e-9 true
```

Without the pin the joint accelerations are `(-16.87, 9.93)` rad/s^2; with it they are zero, since the pinned arm at rest cannot move under these torques. The pin carries `(-17.72, 15.95, 0) N`, and a step along the constrained accelerations moves the tip by less than `1e-9 m`.

## Sharing the load

At rest, the joint torques and the pin together carry the weight. As the shoulder torque grows from `0` to `10 N m` the pin force changes from `(-17.4, 17.2) N` through `(-4.6, 11.7) N` to `(8.3, 6.3) N`: the arm stays at rest because the constraint force adjusts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = planar_arm();
  // the arm at rest with its tip pinned: the pin carries part of the weight, the joint torques the rest.
  // The pin force is the world force the ground exerts on the point (positive y holds the arm up).
  rb::Pins tip;
  tip.link = {1};
  tip.point = mat({0.4, 0.0, 0.0}, {1, 3});
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0.0, 0.0});
  for (double pull : {0.0, 5.0, 10.0}) {
    const rb::ConstrainedResult held = rb::constrain(robot, rb::Contacts{}, tip, q, still, vec({pull, 0.0}), plane_gravity());
    out.writeln("shoulder torque", pull, " pin force", flat(held.pin_force), " acceleration", flat(held.acceleration));
  }
}
```

Output

```text
shoulder torque 0.00000  pin force [-17.40212, 17.16750, 0.00000]  acceleration [-0.00000, 0.00000]
shoulder torque 5.00000  pin force [-4.56246, 11.73898, 0.00000]  acceleration [-0.00000, 0.00000]
shoulder torque 10.00000  pin force [8.27720, 6.31046, 0.00000]  acceleration [-0.00000, 0.00000]
```

## Without constraints

With no pins and no contacts the call is plain forward dynamics, and the force arrays are empty.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = planar_arm();
  // with no contacts and no pins the call is plain forward dynamics, with empty force arrays
  const pluss::Series q = vec({0.4, -0.8}), qd = vec({1.3, -0.7}), tau = vec({1.0, 0.5});
  const rb::ConstrainedResult result = rb::constrain(robot, rb::Contacts{}, rb::Pins{}, q, qd, tau, plane_gravity());
  out.writeln("acceleration", flat(result.acceleration), " forward dynamics", flat(rb::accel(robot, q, qd, tau, plane_gravity().dynamics)));
  out.writeln("contact force shape", result.contact_force.sizes(), " pin force shape", result.pin_force.sizes());
}
```

Output

```text
acceleration [-17.75500, 14.13889]  forward dynamics [-17.75500, 14.13889]
contact force shape [0]  pin force shape [0, 3]
```

## Errors

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
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
  const rb::Robot robot = planar_arm();
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0.0, 0.0});
  report([&] { (void)rb::constrain(robot, rb::Contacts{}, rb::Pins{{5}, mat({0, 0, 0}, {1, 3})}, q, still, still); });
  report([&] { (void)rb::constrain(robot, rb::Contacts{}, rb::Pins{{1}, mat({0, 0}, {1, 2})}, q, still, still); });
  report([&] { (void)rb::constrain(robot, rb::Contacts{}, rb::Pins{{1, 0}, mat({0, 0, 0}, {1, 3})}, q, still, still); });
  report([&] { (void)rb::constrain(robot, rb::Contacts{}, rb::Pins{{1}, mat({0, 0, 0}, {1, 3})}, vec({0.4}), still, still); });
}
```

Output

```text
eng::robot: a constraint refers to a link outside the robot
eng::robot::constrain: point must be a real floating [m, 3] array with one row per link entry
eng::robot::constrain: point must be a real floating [m, 3] array with one row per link entry
eng::robot::constrain: q must be a real floating [..., 2] array
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

static rb::Robot planar_arm() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return rb::make(links);
}
static rb::ConstraintOptions plane_gravity() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
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
  const rb::Robot robot = planar_arm();
  const pluss::Series q = vec({0.4, -0.8});
  report([&] { (void)rb::locate(robot, q, {2}, mat({0, 0, 0}, {1, 3})); });
  report([&] { (void)rb::locate(robot, q, {1}, mat({0, 0}, {1, 2})); });
  report([&] { (void)rb::locate(robot, vec({0.4}), {1}, mat({0, 0, 0}, {1, 3})); });
}
```

Output

```text
eng::robot::locate: link is outside the robot
eng::robot::locate: point must be a real floating [m, 3] array with one row per link entry
eng::robot::locate: q must be a real floating [..., 2] array
```

## Decisions

- **A point that must not move:** a pin; start it at rest in the world.
- **A closed chain or a mechanism bolted down:** one pin per closing point, on a robot described as a tree.
- **Where a point is:** `locate`; place pins and contact points from it.
- **The force on the support:** `pin_force` in the world frame, newtons.

## Where next

Reference pages: [`Pins`](../../../../api/eng/robot/constraints/Pins.md), [`locate`](../../../../api/eng/robot/constraints/locate.md), [`ConstrainedResult`](../../../../api/eng/robot/constraints/ConstrainedResult.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contacts_and_support.html">
    <span class="label">Previous</span>
    <span class="title">Contacts and support</span>
  </a>
  <a class="next-link" href="friction_and_impacts.html">
    <span class="label">Next</span>
    <span class="title">Friction and impacts</span>
  </a>
</div>
<!-- apiref:nav:end -->
