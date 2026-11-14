<!-- generated from Contacts.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::Contacts`

Frictionless unilateral contacts of link points with planes.

## Signature

```cpp
robot::Contacts
```

## What It Does

Describes contacts: point `i` is fixed to link `link[i]` at `point[i]` in that link's frame, and it must stay in the half-space `n . x >= offset` with the unit normal `normal[i]` and `offset[i]` in the world. A contact pushes along its normal, never pulls, and acts only while its gap `n . x - offset` is at most the contact threshold.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `link` | The link each contact point is fixed to. Length `m`. | empty |
| `point` | The contact points in their link frames, `[m, 3]`, in metres. | required |
| `normal` | The unit normal of each plane in the world, `[m, 3]`. | required |
| `offset` | The plane offsets `n . x`, `[m]`, in metres. | required |
| `friction` | The Coulomb friction coefficient of each contact, `[m]`, non-negative. | undefined: frictionless |

## Returns

A value; read by `gap` and `constrain`.

## Errors

The functions throw `pluss::Error` for a link outside the robot, a `point` that is not `[m, 3]` or has a row count different from `link`, or a `normal` and `offset` of the wrong shape.

## Interpretation and Use

With `friction` the tangential force is limited to `friction` times the normal force along two tangent directions of the plane (a pyramid approximation of the friction cone, exact when the motion is along a tangent): sticking contacts carry whatever friction they need up to that limit and sliding ones carry the limit against the sliding. A point that arrives with downward velocity keeps it in `constrain`; use `impact` for the collision.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Support, release and free fall.

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

The gap of a point and of a tilted plane.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = slider();
  // the gap is n . x - offset: positive above the plane, negative below
  out.writeln("gap at heights -0.1, 0, 0.3", flat(rb::gap(robot, floor_contact(), mat({-0.1, 0.0, 0.3}, {3, 1}))));
  // a tilted plane x + y >= 1 through the same point: the normal and the offset define it
  rb::Contacts tilted = floor_contact();
  tilted.normal = mat({std::sqrt(0.5), std::sqrt(0.5), 0}, {1, 3});
  tilted.offset = vec({std::sqrt(0.5)});
  out.writeln("gap of the slider at y = 1 against the tilted plane", flat(rb::gap(robot, tilted, vec({1.0}).reshape({1, 1}))));
  // the contact point is in the link frame: a point 0.2 m below the slider's origin touches when the origin is at 0.2
  rb::Contacts foot = floor_contact();
  foot.point = mat({0, -0.2, 0}, {1, 3});
  out.writeln("gap with a foot 0.2 m below", flat(rb::gap(robot, foot, mat({0.2, 0.5}, {2, 1}))));
}
```

Output

```text
gap at heights -0.1, 0, 0.3 [-0.1000, 0.0000, 0.3000]
gap of the slider at y = 1 against the tilted plane [0.0000]
gap with a foot 0.2 m below [0.0000, 0.3000]
```
### Example 3

Friction on a slope: sticking and sliding.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a block of mass 2 kg free to move in the x-y plane: two prismatic joints (x, then y), the block on the second link
static rb::Robot block() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 1};
  links.axis = mat({1, 0, 0, 0, 1, 0}, {2, 3});
  links.translation = mat({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({0.0, 2.0});
  return rb::make(links);
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}
static rb::Contacts slope(double degrees, double friction) {
  // an incline through the origin rising to the right: the contact point is the block's origin
  const double t = degrees * M_PI / 180.0;
  rb::Contacts contact;
  contact.link = {1};
  contact.point = mat({0, 0, 0}, {1, 3});
  contact.normal = mat({-std::sin(t), std::cos(t), 0}, {1, 3});
  contact.offset = vec({0.0});
  if (friction > 0.0) contact.friction = vec({friction});
  return contact;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  const pluss::Series q = vec({0.0, 0.0}), rest = vec({0.0, 0.0});
  // a block on a 30 degree slope: it slides when tan(30) = 0.577 exceeds the friction coefficient
  out.writeln("friction  acceleration (x, y)         normal force  friction force");
  for (double mu : {0.0, 0.3, 0.5, 0.6, 0.8}) {
    const rb::ConstrainedResult r = rb::constrain(robot, slope(30.0, mu), rb::Pins{}, q, rest, rest, earth_in_plane());
    const std::vector<double> a = flat(r.acceleration), f = flat(r.friction_force);
    out.writeln(mu, a[0], a[1], flat(r.contact_force)[0], std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]));
  }
  out.writeln("weight components: normal m g cos 30 =", 2 * 9.81 * std::cos(M_PI / 6), " tangential m g sin 30 =", 2 * 9.81 * std::sin(M_PI / 6));
}
```

Output

```text
friction  acceleration (x, y)         normal force  friction force
0.00000 -4.24785 -2.45250 16.99142 0.00000
0.30000 -2.04060 -1.17814 16.99142 5.09743
0.50000 -0.56910 -0.32857 16.99142 8.49571
0.60000 -0.00000 0.00000 16.99142 9.81000
0.80000 0.00000 0.00000 16.99142 9.81000
weight components: normal m g cos 30 = 16.99142  tangential m g sin 30 = 9.81000
```
### Example 4

Sliding accelerations against the closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a block of mass 2 kg free to move in the x-y plane: two prismatic joints (x, then y), the block on the second link
static rb::Robot block() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 1};
  links.axis = mat({1, 0, 0, 0, 1, 0}, {2, 3});
  links.translation = mat({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({0.0, 2.0});
  return rb::make(links);
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}
static rb::Contacts slope(double degrees, double friction) {
  // an incline through the origin rising to the right: the contact point is the block's origin
  const double t = degrees * M_PI / 180.0;
  rb::Contacts contact;
  contact.link = {1};
  contact.point = mat({0, 0, 0}, {1, 3});
  contact.normal = mat({-std::sin(t), std::cos(t), 0}, {1, 3});
  contact.offset = vec({0.0});
  if (friction > 0.0) contact.friction = vec({friction});
  return contact;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  const pluss::Series rest = vec({0.0, 0.0});
  // sliding: the acceleration down the slope is g (sin t - mu cos t); batches of friction coefficients are separate calls here
  const double t = M_PI / 6;
  for (double mu : {0.1, 0.3, 0.5}) {
    const rb::ConstrainedResult r = rb::constrain(robot, slope(30.0, mu), rb::Pins{}, rest, rest, rest, earth_in_plane());
    const std::vector<double> a = flat(r.acceleration);
    out.writeln("friction", mu, " acceleration along the slope", std::abs(a[0] * std::cos(t) + a[1] * std::sin(t)), " formula", 9.81 * (std::sin(t) - mu * std::cos(t)));
  }
  // the friction opposes the sliding: it points up the slope
  const rb::ConstrainedResult r = rb::constrain(robot, slope(30.0, 0.3), rb::Pins{}, rest, rest, rest, earth_in_plane());
  out.writeln("friction force (x, y, z)", flat(r.friction_force));
}
```

Output

```text
friction 0.10000  acceleration along the slope 4.05543  formula 4.05543
friction 0.30000  acceleration along the slope 2.35629  formula 2.35629
friction 0.50000  acceleration along the slope 0.65715  formula 0.65715
friction force (x, y, z) [4.41450, 2.54871, 0.00000]
```
### Example 5

Rejected friction values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a block of mass 2 kg free to move in the x-y plane: two prismatic joints (x, then y), the block on the second link
static rb::Robot block() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 1};
  links.axis = mat({1, 0, 0, 0, 1, 0}, {2, 3});
  links.translation = mat({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({0.0, 2.0});
  return rb::make(links);
}
static rb::ConstraintOptions earth_in_plane() {
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  return options;
}
static rb::Contacts slope(double degrees, double friction) {
  // an incline through the origin rising to the right: the contact point is the block's origin
  const double t = degrees * M_PI / 180.0;
  rb::Contacts contact;
  contact.link = {1};
  contact.point = mat({0, 0, 0}, {1, 3});
  contact.normal = mat({-std::sin(t), std::cos(t), 0}, {1, 3});
  contact.offset = vec({0.0});
  if (friction > 0.0) contact.friction = vec({friction});
  return contact;
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
  const rb::Robot robot = block();
  const pluss::Series rest = vec({0.0, 0.0});
  rb::Contacts negative = slope(30.0, 0.3);
  negative.friction = vec({-0.1});
  report([&] { (void)rb::constrain(robot, negative, rb::Pins{}, rest, rest, rest, earth_in_plane()); });
  rb::Contacts shape = slope(30.0, 0.3);
  shape.friction = vec({0.1, 0.2});
  report([&] { (void)rb::constrain(robot, shape, rb::Pins{}, rest, rest, rest, earth_in_plane()); });
  rb::ConstraintOptions rounds = earth_in_plane();
  rounds.friction_iterations = 0;
  report([&] { (void)rb::constrain(robot, slope(30.0, 0.3), rb::Pins{}, rest, rest, rest, rounds); });
}
```

Output

```text
eng::robot::constrain: friction must be a non-negative real floating vector with one value per contact
eng::robot::constrain: friction must be a non-negative real floating vector with one value per contact
eng::robot::constrain: contact_threshold must be non-negative and friction_iterations at least 1
```
### Example 6

Rejected contacts.

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
  const rb::Robot robot = slider();
  const pluss::Series q = vec({0.0}).reshape({1, 1});
  report([&] { (void)rb::gap(robot, rb::Contacts{{3}, mat({0, 0, 0}, {1, 3}), mat({0, 1, 0}, {1, 3}), vec({0.0})}, q); });
  report([&] { (void)rb::gap(robot, rb::Contacts{{0}, mat({0, 0}, {1, 2}), mat({0, 1, 0}, {1, 3}), vec({0.0})}, q); });
  report([&] { (void)rb::gap(robot, rb::Contacts{{0}, mat({0, 0, 0}, {1, 3}), mat({0, 1}, {1, 2}), vec({0.0})}, q); });
  rb::ConstraintOptions bad = earth_in_plane();
  bad.contact_threshold = -1.0;
  report([&] { (void)rb::constrain(robot, floor_contact(), rb::Pins{}, q, q, q, bad); });
  const rb::Robot arm = planar_arm();
  rb::Links massless;
  massless.parent = {-1};
  massless.prismatic = {0};
  massless.axis = mat({0, 0, 1}, {1, 3});
  massless.translation = mat({0, 0, 0}, {1, 3});
  report([&] { (void)rb::constrain(rb::make(massless), rb::Contacts{}, rb::Pins{}, q, q, q); });
}
```

Output

```text
eng::robot::locate: link is outside the robot
eng::robot::gap: point must be a real floating [m, 3] array with one row per link entry
eng::robot::gap: normal must be [m, 3] and offset [m]
eng::robot::constrain: contact_threshold must be non-negative and friction_iterations at least 1
eng::robot::constrain: the robot has no mass properties; set mass (and optionally center and inertia) in Links
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Pins.html">
    <span class="label">Next</span>
    <span class="title">Pins</span>
  </a>
</div>
<!-- apiref:nav:end -->
