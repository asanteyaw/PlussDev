<!-- generated from constrain.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::constrain`

Joint accelerations under pins and contacts.

## Signature

```cpp
robot::ConstrainedResult robot::constrain(const robot::Robot& robot, const robot::Contacts& contacts, const robot::Pins& pins, const pluss::Series& q, const pluss::Series& qd, const pluss::Series& tau, const robot::ConstraintOptions& options = {});
```

## What It Does

Finds the joint accelerations of the motion `(q, qd)` under joint forces `tau` that also satisfy the constraints. Pin rows are equations `J qdd + J' qd = 0` with free forces. A contact row is the inequality `n . (J qdd + J' qd) >= 0` with a force `lambda >= 0` along the normal and complementarity `lambda (a_n) = 0`, active while the gap is at most the threshold. With the Delassus matrix `W = J M^-1 J^T` the forces solve the linear complementarity problem `W lambda + a0 >= 0`, `lambda >= 0`, which is the optimality system of a box-constrained quadratic program and is solved by `math::qp` for the whole batch.

## Parameters

`robot`
: The compiled robot, with mass properties.

`contacts`
: The contacts; may be empty.

`pins`
: The pins; may be empty.

`q`
: The joint values, `[..., links]`.

`qd`
: The joint rates.

`tau`
: The applied joint forces.

`options`
: Gravity and the contact threshold; see `ConstraintOptions`.

## Returns

A `ConstrainedResult` with the accelerations, contact forces, pin forces and gaps.

## Errors

Throws `pluss::Error` when the robot has no mass properties, for a malformed argument, or for a contact threshold that is negative.

## Interpretation and Use

The forces are accurate to the tolerance of the quadratic-program solver (about `1e-6` in the examples). Contacts with `friction` add two tangent rows each, whose forces are limited to the friction coefficient times the normal force (solved by a short fixed-point iteration over the quadratic program). A contact removes the acceleration into the plane but not an incoming velocity: use `impact` for that. Redundant constraints are tolerated; inconsistent ones return the least-violating forces.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A slider on a floor: support, release and free fall.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a vertical slider of mass 2 kg (a prismatic joint along y) with a point contact on the floor y >= 0
static rb::Robot slider() {
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {1};
  links.axis = mat({0, 1, 0}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  links.mass = vec({2.0});
  return rb::make(links);
}
static rb::Contacts floor_contact() {
  rb::Contacts floor;
  floor.link = {0};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  return floor;
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = slider();
  // three sliders at once: on the floor with no pull, on the floor pulled up hard, and 0.5 m in the air
  const pluss::Series q = mat({0.0, 0.0, 0.5}, {3, 1}), qd = mat({0.0, 0.0, 0.0}, {3, 1}), tau = mat({0.0, 40.0, 0.0}, {3, 1});
  const rb::ConstrainedResult result = rb::constrain(robot, floor_contact(), rb::Pins{}, q, qd, tau, earth_in_plane());
  out.writeln("gap", flat(result.gap));
  out.writeln("contact force", flat(result.contact_force), " (the weight is 2 x 9.81 = 19.62 N)");
  out.writeln("acceleration ", flat(result.acceleration), " (resting, pulled free, falling)");
  out.writeln("force shape", result.contact_force.sizes(), " pin force shape", result.pin_force.sizes());
}
```

Output

```text
gap [0.00000, 0.00000, 0.50000]
contact force [19.62000, -0.00000, 0.00000]  (the weight is 2 x 9.81 = 19.62 N)
acceleration  [0.00000, 10.19000, -9.81000]  (resting, pulled free, falling)
force shape [3, 1]  pin force shape [3, 0, 3]
```
### Example 2

Support as a function of the pull.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a vertical slider of mass 2 kg (a prismatic joint along y) with a point contact on the floor y >= 0
static rb::Robot slider() {
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {1};
  links.axis = mat({0, 1, 0}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  links.mass = vec({2.0});
  return rb::make(links);
}
static rb::Contacts floor_contact() {
  rb::Contacts floor;
  floor.link = {0};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  return floor;
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = slider();
  // the floor carries what the pull does not: force = max(0, weight - pull); it never pulls the slider down
  std::vector<double> pulls{0, 5, 10, 19.62, 25, 40};
  const pluss::Series tau = vec(pulls).reshape({6, 1});
  const pluss::Series q = pluss::series::zeros({6, 1}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const rb::ConstrainedResult result = rb::constrain(robot, floor_contact(), rb::Pins{}, q, q, tau, earth_in_plane());
  const std::vector<double> force = flat(result.contact_force), acceleration = flat(result.acceleration);
  out.writeln("  pull    floor force   acceleration");
  for (int k = 0; k < 6; ++k) out.writeln(pulls[k], force[k], acceleration[k]);
}
```

Output

```text
  pull    floor force   acceleration
0.00000 19.62000 0.00000
5.00000 14.62000 -0.00000
10.00000 9.62000 0.00000
19.62000 0.00000 0.00000
25.00000 -0.00000 2.69000
40.00000 -0.00000 10.19000
```
### Example 3

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
### Example 4

A resting simulation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a vertical slider of mass 2 kg (a prismatic joint along y) with a point contact on the floor y >= 0
static rb::Robot slider() {
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {1};
  links.axis = mat({0, 1, 0}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  links.mass = vec({2.0});
  return rb::make(links);
}
static rb::Contacts floor_contact() {
  rb::Contacts floor;
  floor.link = {0};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  return floor;
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rb::Robot robot = slider();
  // a slider resting on the floor stays there: simulate a second of semi-implicit Euler under gravity and a small push up
  pluss::Series q = vec({0.0}).reshape({1}), v = q;
  const rb::Contacts floor = floor_contact();
  const double h = 1e-3;
  for (int k = 0; k <= 1000; ++k) {
    const rb::ConstrainedResult step = rb::constrain(robot, floor, rb::Pins{}, q, v, vec({5.0}).reshape({1}), earth_in_plane());
    if (k % 250 == 0) out.writeln(k * h, "s  height", flat(q)[0], " speed", flat(v)[0], " floor force", flat(step.contact_force)[0]);
    v = v.add(step.acceleration.mul(h));
    q = q.add(v.mul(h));
  }
}
```

Output

```text
0.000000 s  height 0.000000  speed 0.000000  floor force 14.620000
0.250000 s  height 0.000000  speed 0.000000  floor force 14.620000
0.500000 s  height 0.000000  speed 0.000000  floor force 14.620000
0.750000 s  height 0.000000  speed 0.000000  floor force 14.620000
1.000000 s  height 0.000000  speed 0.000000  floor force 14.620000
```
### Example 5

No constraints.

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
### Example 6

Rejected arguments.

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
  <a class="prev-link" href="gap.html">
    <span class="label">Previous</span>
    <span class="title">gap</span>
  </a>
  <a class="next-link" href="ImpactResult.html">
    <span class="label">Next</span>
    <span class="title">ImpactResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
