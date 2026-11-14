<!-- generated from constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Constraints and contacts

This branch holds a robot to constraints. `Pins` hold points at rest in the world and `Contacts` keep points on one side of planes without friction; `constrain` gives the accelerations and the constraint forces (with Coulomb friction when set), `impact` the velocities after a collision, and `locate` and `gap` locate the points.

## Choosing A Surface

Use `Pins` for points that must not move and `Contacts` for supports that can only push; use `constrain` for either, and `accel` when there is nothing to hold.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Support, release and free fall of a slider.

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
### Example 3

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
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

constraints/Contacts
constraints/Pins
constraints/ConstraintOptions
constraints/ConstrainedResult
constraints/locate
constraints/gap
constraints/constrain
constraints/ImpactResult
constraints/impact
```
<!-- apiref:toctree:end -->
