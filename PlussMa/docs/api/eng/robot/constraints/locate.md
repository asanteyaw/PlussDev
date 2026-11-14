<!-- generated from locate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::locate`

The world position of points fixed to links.

## Signature

```cpp
pluss::Series robot::locate(const robot::Robot& robot, const pluss::Series& q, const std::vector<std::int64_t>& link, const pluss::Series& point);
```

## What It Does

Returns the world position of point `i`, fixed to link `link[i]` at `point[i]` in that link's frame.

## Parameters

`robot`
: The compiled robot.

`q`
: The joint values, `[..., links]`.

`link`
: The link of each point, length `m`.

`point`
: The points in their link frames, `[m, 3]`.

## Returns

A real `[..., m, 3]` array of world positions.

## Errors

Throws `pluss::Error` for a link outside the robot, a `point` of the wrong shape, or a `q` that is not a real floating `[..., links]` array.

## Interpretation and Use

Use it to place a pin or a contact point and to check a constraint: the tip of an arm, a foot, a tool point.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Points of a planar arm.

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
### Example 2

The foot of a slider.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConstrainedResult.html">
    <span class="label">Previous</span>
    <span class="title">ConstrainedResult</span>
  </a>
  <a class="next-link" href="gap.html">
    <span class="label">Next</span>
    <span class="title">gap</span>
  </a>
</div>
<!-- apiref:nav:end -->
