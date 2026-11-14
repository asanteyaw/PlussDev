<!-- generated from ImpactResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::ImpactResult`

The result of an impact.

## Signature

```cpp
robot::ImpactResult
```

## What It Does

Holds the joint velocities after an impact and the impulses that produced them.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `velocity` | The joint velocities after the impact, `[..., links]`. |  |
| `contact_impulse` | The non-negative normal impulse of each contact, `[..., m]`, in newton-seconds. |  |
| `pin_impulse` | The world impulse on each pinned point, `[..., p, 3]`. |  |
| `friction_impulse` | The world friction impulse on each contact point, `[..., m, 3]`; zero for frictionless contacts. |  |

## Returns

Returned by `impact`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

A contact that is out of range, or not approaching its plane, has zero impulse. With no constraints the velocity is unchanged and the impulse arrays are empty.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of an impact.

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

Friction impulses.

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

A batch of impacts.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="constrain.html">
    <span class="label">Previous</span>
    <span class="title">constrain</span>
  </a>
  <a class="next-link" href="impact.html">
    <span class="label">Next</span>
    <span class="title">impact</span>
  </a>
</div>
<!-- apiref:nav:end -->
