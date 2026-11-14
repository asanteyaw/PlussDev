<!-- generated from Pins.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::Pins`

Points held at rest in the world.

## Signature

```cpp
robot::Pins
```

## What It Does

Describes bilateral constraints: point `i` is fixed to link `link[i]` at `point[i]` in that link's frame and its world acceleration is held at zero (a pin to the ground, or a point held fixed). Each pin removes three degrees of freedom and carries a free three-component force.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `link` | The link of each pinned point. Length `p`. | empty |
| `point` | The pinned points in their link frames, `[p, 3]`, in metres. | required |

## Returns

A value; read by `constrain`.

## Errors

`constrain` throws `pluss::Error` for a link outside the robot or a `point` that is not `[p, 3]` with one row per link entry.

## Interpretation and Use

A pin keeps the point's acceleration zero, which holds the point where it is only if its velocity is zero too: start the robot with the pinned point at rest in the world. Pinning more than the robot has degrees of freedom is allowed but only the consistent part can be satisfied.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A pinned tip.

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
### Example 2

The pin carries the weight the joints do not.

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
### Example 3

Rejected pins.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Contacts.html">
    <span class="label">Previous</span>
    <span class="title">Contacts</span>
  </a>
  <a class="next-link" href="ConstraintOptions.html">
    <span class="label">Next</span>
    <span class="title">ConstraintOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
