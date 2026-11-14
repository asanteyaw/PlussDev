<!-- generated from contacts_and_support.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Contacts and support

[Back to Robotics 3: Constraints and Contacts](../robot_03_constraints.md)

A floor can push a body up but never pull it down. This page models that: frictionless unilateral contacts of points fixed to links with planes. It finds the force a floor carries as a function of the pull on a slider, the difference between resting, being pulled free and falling, the effect of the contact threshold, and how to locate contact points.

## The mental model

`robot::Contacts` gives the contact points (`link` and `point` in the link frame), the plane normal (a unit vector in the world) and `offset`, so the point must stay in `n . x >= offset`. The **gap** `n . x - offset` is the distance above the plane. A contact acts while its gap is at most `ConstraintOptions::contact_threshold`.

For a contact in range the dynamics must satisfy three conditions at once: the acceleration of the point along the normal is non-negative (the point does not accelerate into the plane), the contact force is non-negative (it pushes, never pulls) and they are complementary: if the force is positive the point does not accelerate away, and if it accelerates away the force is zero. With `M` the inertia matrix, `J` the contact Jacobian and `W = J M^-1 J^T` the Delassus matrix this is the linear complementarity problem of the forces, which `constrain` solves for the whole batch as a quadratic program with `math::qp`.

The model is deliberately simple: **frictionless** (the force is along the normal) and with **no impact law** (a point that arrives with downward speed keeps it). Use it for resting and sliding support, not for collisions.

## Support, release and free fall

A `2 kg` slider moves vertically (a prismatic joint along `y`) with a floor at `y = 0`; gravity points along `-y`. The program solves three sliders at once: resting on the floor with no pull, resting while pulled up with `40 N`, and `0.5 m` in the air.

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

The resting slider has an acceleration of zero and the floor carries its full weight, `2 x 9.81 = 19.62 N`. The pulled slider leaves the floor (acceleration `10.19 m/s^2 = (40 - 19.62) / 2`) and the force is zero. The slider in the air falls at `-9.81` with no contact force. The force array is `[3, 1]`, configuration then contact.

## How the floor shares the load

The floor carries what the pull does not: `max(0, weight - pull)`. The program sweeps the pull.

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

With no pull the floor carries `19.62 N`; with `5` and `10 N` it carries `14.62` and `9.62 N`; at a pull equal to the weight it carries nothing; and above it the slider accelerates upwards (`2.69` and `10.19 m/s^2`) with no force. The floor never pulls.

## The gap

`gap` gives the distance from each plane. The contact point is fixed to its link, so a foot `0.2 m` below the slider's origin touches the floor when the origin is at `0.2 m`; a tilted plane needs only a different normal and offset.

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

## The threshold

A contact acts while its gap is at most the threshold. A slider resting `1 mm` above the floor is a numerical accident away from a resting contact: with the default threshold of `1 um` it is in free fall, with a threshold of `1 cm` it is supported.

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
  // a contact acts while its gap is at most contact_threshold: sliders 1 mm above the floor, with two thresholds
  const pluss::Series q = vec({0.001}).reshape({1, 1}), rest = vec({0.0}).reshape({1, 1});
  for (double threshold : {1e-6, 1e-2}) {
    rb::ConstraintOptions options = earth_in_plane();
    options.contact_threshold = threshold;
    const rb::ConstrainedResult result = rb::constrain(robot, floor_contact(), rb::Pins{}, q, rest, rest, options);
    out.writeln("threshold", threshold, " contact force", flat(result.contact_force)[0], " acceleration", flat(result.acceleration)[0]);
  }
}
```

Output

```text
threshold 0.00000  contact force -0.00000  acceleration -9.81000
threshold 0.01000  contact force 19.62000  acceleration 0.00000
```

## Gravity

The force the floor carries is the weight, whatever the planet.

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
  const pluss::Series q = vec({0.0}).reshape({1, 1}), rest = q;
  // the gravity of the options sets the weight the floor carries: Earth, the moon and none
  for (double g : {9.81, 1.62, 0.0}) {
    rb::ConstraintOptions options;
    options.dynamics.gravity = vec({0.0, -g, 0.0});
    const rb::ConstrainedResult result = rb::constrain(robot, floor_contact(), rb::Pins{}, q, rest, rest, options);
    out.writeln("gravity", g, " floor force", flat(result.contact_force)[0], " expected", 2.0 * g);
  }
}
```

Output

```text
gravity 9.8100  floor force 19.6200  expected 19.6200
gravity 1.6200  floor force 3.2400  expected 3.2400
gravity 0.0000  floor force 0.0000  expected 0.0000
```

## A resting simulation

A semi-implicit Euler simulation of the slider on the floor with a `5 N` push up (less than the weight) shows it staying where it is, with the floor carrying the rest of the weight, `14.62 N`.

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

## Decisions

- **Supports that can only push:** `Contacts`; set the threshold to the distance within which you want the support to act.
- **Collisions:** not modelled: no impact law, so handle impacts outside (reset the velocity) and use the contacts for what follows.
- **Friction:** not modelled; the force is along the normal only.
- **Many contacts or lanes:** one call; the batch is solved together.

## Pitfalls

The program makes three mistakes with the floor contact: a contact normal that is not a unit vector, a gap measured with the offset in the wrong direction, and expecting the floor to stop a falling slider that is not yet in range.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Robot slider() {
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {1};
  links.axis = mat({0, 1, 0}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  links.mass = vec({2.0});
  return rb::make(links);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = slider();
  rb::ConstraintOptions options;
  options.dynamics.gravity = vec({0.0, -9.81, 0.0});
  const pluss::Series at_rest = vec({0.0}).reshape({1, 1}), tau = vec({0.0}).reshape({1, 1});
  rb::Contacts floor;
  floor.link = {0};
  floor.point = mat({0, 0, 0}, {1, 3});
  floor.offset = vec({0.0});

  // 1. a normal of length two: the gap and the force are scaled by two
  floor.normal = mat({0, 2, 0}, {1, 3});
  out.writeln("gap of a point 0.3 m up with a normal of length 2", flat(rb::gap(robot, floor, vec({0.3}).reshape({1, 1}))));
  // 2. the plane y >= 0.5 is offset 0.5, not -0.5
  floor.normal = mat({0, 1, 0}, {1, 3});
  floor.offset = vec({0.5});
  out.writeln("gap at y = 0.3 against the plane y >= 0.5", flat(rb::gap(robot, floor, vec({0.3}).reshape({1, 1}))), " (negative: below the plane)");
  // 3. a slider out of range is in free fall: the floor does not catch it before the gap closes
  floor.offset = vec({0.0});
  const rb::ConstrainedResult high = rb::constrain(robot, floor, rb::Pins{}, vec({0.2}).reshape({1, 1}), at_rest, tau, options);
  out.writeln("slider 0.2 m up: acceleration", flat(high.acceleration)[0], " floor force", flat(high.contact_force)[0]);
}
```

Output

```text
gap of a point 0.3 m up with a normal of length 2 [0.6000]
gap at y = 0.3 against the plane y >= 0.5 [-0.2000]  (negative: below the plane)
slider 0.2 m up: acceleration -9.8100  floor force -0.0000
```

A normal of length two doubles the gap (`0.6` for a point `0.3 m` up), so give unit normals. The plane `y >= 0.5` has offset `+0.5`: a point at `y = 0.3` has a gap of `-0.2`, below the plane. And a slider `0.2 m` above the floor is in free fall (`-9.81 m/s^2`) with no contact force: the floor acts only within the contact threshold, so a collision needs `impact` (see [Friction and impacts](friction_and_impacts.md)).

## Where next

[Pins and closed chains](pins.md) holds points at rest. Reference pages: [`Contacts`](../../../../api/eng/robot/constraints/Contacts.md), [`constrain`](../../../../api/eng/robot/constraints/constrain.md), [`ConstraintOptions`](../../../../api/eng/robot/constraints/ConstraintOptions.md), [`gap`](../../../../api/eng/robot/constraints/gap.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="pins.html">
    <span class="label">Next</span>
    <span class="title">Pins and closed chains</span>
  </a>
</div>
<!-- apiref:nav:end -->
