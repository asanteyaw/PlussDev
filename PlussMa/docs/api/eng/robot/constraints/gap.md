<!-- generated from gap.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::gap`

The distance of contact points from their planes.

## Signature

```cpp
pluss::Series robot::gap(const robot::Robot& robot, const robot::Contacts& contacts, const pluss::Series& q);
```

## What It Does

Returns `n . x - offset` for every contact: positive above the plane, zero on it, negative below.

## Parameters

`robot`
: The compiled robot.

`contacts`
: The contacts.

`q`
: The joint values, `[..., links]`.

## Returns

A real `[..., m]` array in metres.

## Errors

Throws `pluss::Error` for contacts of the wrong shape, a link outside the robot, or a malformed `q`.

## Interpretation and Use

The gap tells whether a contact is in range; `constrain` returns it as `gap`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Heights above a floor and a tilted plane.

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
### Example 2

The gap of three sliders.

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
### Example 3

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
  <a class="prev-link" href="locate.html">
    <span class="label">Previous</span>
    <span class="title">locate</span>
  </a>
  <a class="next-link" href="constrain.html">
    <span class="label">Next</span>
    <span class="title">constrain</span>
  </a>
</div>
<!-- apiref:nav:end -->
