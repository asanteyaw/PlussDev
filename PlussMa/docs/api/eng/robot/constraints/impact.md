<!-- generated from impact.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::impact`

Joint velocities after an impact.

## Signature

```cpp
robot::ImpactResult robot::impact(const robot::Robot& robot, const robot::Contacts& contacts, const robot::Pins& pins, const pluss::Series& q, const pluss::Series& qd, double restitution, const robot::ConstraintOptions& options = {});
```

## What It Does

Applies Newton's impact law to every contact that is in range (gap at most the threshold) and approaching its plane (normal velocity below zero): it receives a non-negative normal impulse chosen so that the normal velocity after the impact is `restitution` times the approach speed, with complementarity (no impulse if the contact is already separating after). Pins stop their points, and friction (when set) limits the tangential impulse of a contact to the friction coefficient times its normal impulse. The impulses solve the linear complementarity problem of the Delassus matrix `J M^-1 J^T`, as a quadratic program by `math::qp`, for the whole batch.

## Parameters

`robot`
: The compiled robot, with mass properties.

`contacts`
: The contacts; may be empty.

`pins`
: The pins; may be empty.

`q`
: The joint values at the impact, `[..., links]`.

`qd`
: The joint velocities just before it.

`restitution`
: The coefficient of restitution in `[0, 1]`: `0` a plastic impact, `1` an elastic one.

`options`
: The contact threshold and friction iterations; see `ConstraintOptions`.

## Returns

An `ImpactResult` with the velocities after, the contact impulses, the pin impulses and the friction impulses.

## Errors

Throws `pluss::Error` when the robot has no mass properties, for a `restitution` outside `[0, 1]`, for a malformed argument, or for options out of range.

## Interpretation and Use

The velocity after is `qd + M^-1 J^T impulse`: every joint coupled to a contact point shares the change through the inertia. Use `impact` to reset the velocity when a simulation step brings a point to a plane, then `constrain` for the contact that follows.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Plastic, partly elastic and elastic impacts.

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
static rb::Contacts floor_contact(double friction = 0.0) {
  rb::Contacts floor;
  floor.link = {1};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  if (friction > 0.0) floor.friction = vec({friction});
  return floor;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  const pluss::Series q = vec({0.0, 0.0});
  // a 2 kg block falls onto the floor at 3 m/s: the rebound is restitution times the approach speed, the impulse m (1 + e) v
  out.writeln("restitution  velocity after (x, y)   impulse   m (1 + e) v");
  for (double e : {0.0, 0.5, 1.0}) {
    const rb::ImpactResult hit = rb::impact(robot, floor_contact(), rb::Pins{}, q, vec({0.0, -3.0}), e);
    out.writeln(e, flat(hit.velocity)[0], flat(hit.velocity)[1], flat(hit.contact_impulse)[0], 2.0 * 3.0 * (1.0 + e));
  }
}
```

Output

```text
restitution  velocity after (x, y)   impulse   m (1 + e) v
0.00000 0.00000 0.00000 6.00000 6.00000
0.50000 0.00000 1.50000 9.00000 9.00000
1.00000 0.00000 3.00000 12.00000 12.00000
```
### Example 2

Friction in an impact.

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
static rb::Contacts floor_contact(double friction = 0.0) {
  rb::Contacts floor;
  floor.link = {1};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  if (friction > 0.0) floor.friction = vec({friction});
  return floor;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  const pluss::Series q = vec({0.0, 0.0}), v = vec({2.0, -1.0});
  // landing at 2 m/s sideways and 1 m/s down with a plastic impact: friction removes up to mu x the normal impulse of tangential speed
  out.writeln("friction  velocity after (x, y)   normal impulse  friction impulse");
  for (double mu : {0.0, 0.5, 1.0, 2.0}) {
    const rb::ImpactResult hit = rb::impact(robot, floor_contact(mu), rb::Pins{}, q, v, 0.0);
    out.writeln(mu, flat(hit.velocity)[0], flat(hit.velocity)[1], flat(hit.contact_impulse)[0], flat(hit.friction_impulse)[0]);
  }
  out.writeln("sticks when mu >= 2: the sideways speed 2 needs a tangential impulse m v_t = 4 = mu x the normal impulse 2 x 1");
}
```

Output

```text
friction  velocity after (x, y)   normal impulse  friction impulse
0.00000 2.00000 0.00000 2.00000 0.00000
0.50000 1.50000 -0.00000 2.00000 -1.00000
1.00000 1.00000 -0.00000 2.00000 -2.00000
2.00000 0.00000 -0.00000 2.00000 -4.00000
sticks when mu >= 2: the sideways speed 2 needs a tangential impulse m v_t = 4 = mu x the normal impulse 2 x 1
```
### Example 3

Separating and out-of-range contacts.

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
static rb::Contacts floor_contact(double friction = 0.0) {
  rb::Contacts floor;
  floor.link = {1};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  if (friction > 0.0) floor.friction = vec({friction});
  return floor;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  // only a contact in range that approaches its plane takes an impulse
  out.writeln("separating (moving up):     ", flat(rb::impact(robot, floor_contact(), rb::Pins{}, vec({0.0, 0.0}), vec({1.0, 2.0}), 0.5).velocity));
  out.writeln("approaching, out of range:  ", flat(rb::impact(robot, floor_contact(), rb::Pins{}, vec({0.0, 0.5}), vec({0.0, -3.0}), 0.5).velocity));
  out.writeln("approaching, in range:      ", flat(rb::impact(robot, floor_contact(), rb::Pins{}, vec({0.0, 0.0}), vec({0.0, -3.0}), 0.5).velocity));
  out.writeln("no constraints at all:      ", flat(rb::impact(robot, rb::Contacts{}, rb::Pins{}, vec({0.0, 0.0}), vec({0.0, -3.0}), 0.5).velocity));
}
```

Output

```text
separating (moving up):      [1.00000, 2.00000]
approaching, out of range:   [0.00000, -3.00000]
approaching, in range:       [0.00000, 1.50000]
no constraints at all:       [0.00000, -3.00000]
```
### Example 4

A pin stops its point.

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
  // a pin stops its point completely, whatever the restitution
  rb::Pins pin;
  pin.link = {1};
  pin.point = mat({0, 0, 0}, {1, 3});
  const rb::ImpactResult hit = rb::impact(robot, rb::Contacts{}, pin, vec({0.0, 0.0}), vec({2.0, -1.0}), 0.5);
  out.writeln("velocity after", flat(hit.velocity), " pin impulse", flat(hit.pin_impulse), " (= -m v = (-4, 2, 0))");
}
```

Output

```text
velocity after [-0.00000, 0.00000]  pin impulse [-4.00000, 2.00000, 0.00000]  (= -m v = (-4, 2, 0))
```
### Example 5

Three blocks at once.

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
static rb::Contacts floor_contact(double friction = 0.0) {
  rb::Contacts floor;
  floor.link = {1};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  if (friction > 0.0) floor.friction = vec({friction});
  return floor;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = block();
  // three blocks at once: falling, rising and sliding
  const pluss::Series q = mat({0, 0, 0, 0, 0, 0}, {3, 2}), v = mat({0, -3, 0, 3, 2, -1}, {3, 2});
  const rb::ImpactResult hit = rb::impact(robot, floor_contact(0.5), rb::Pins{}, q, v, 0.5);
  out.writeln("velocity shape", hit.velocity.sizes(), " impulse shape", hit.contact_impulse.sizes(), " friction impulse shape", hit.friction_impulse.sizes());
  out.writeln("velocities after", flat(hit.velocity));
  out.writeln("normal impulses ", flat(hit.contact_impulse));
}
```

Output

```text
velocity shape [3, 2]  impulse shape [3, 1]  friction impulse shape [3, 1, 3]
velocities after [0.00000, 1.50000, 0.00000, 3.00000, 1.25000, 0.50000]
normal impulses  [9.00000, -0.00000, 3.00000]
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
static rb::Contacts floor_contact(double friction = 0.0) {
  rb::Contacts floor;
  floor.link = {1};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  if (friction > 0.0) floor.friction = vec({friction});
  return floor;
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
  const pluss::Series q = vec({0.0, 0.0}), v = vec({0.0, -1.0});
  report([&] { (void)rb::impact(robot, floor_contact(), rb::Pins{}, q, v, 1.5); });
  report([&] { (void)rb::impact(robot, floor_contact(), rb::Pins{}, q, v, -0.1); });
  report([&] { (void)rb::impact(robot, floor_contact(), rb::Pins{}, vec({0.0}), v, 0.5); });
  rb::ConstraintOptions bad;
  bad.contact_threshold = -1.0;
  report([&] { (void)rb::impact(robot, floor_contact(), rb::Pins{}, q, v, 0.5, bad); });
}
```

Output

```text
eng::robot::impact: restitution must lie in [0, 1]
eng::robot::impact: restitution must lie in [0, 1]
eng::robot::impact: q must be a real floating [..., 2] array
eng::robot::impact: contact_threshold must be non-negative and friction_iterations at least 1
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ImpactResult.html">
    <span class="label">Previous</span>
    <span class="title">ImpactResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
