<!-- generated from ConstraintOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::ConstraintOptions`

Controls of constrained dynamics.

## Signature

```cpp
robot::ConstraintOptions
```

## What It Does

Holds the gravity of the underlying dynamics and the distance within which a contact acts.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `dynamics` | The `DynamicsOptions` (gravity) of the dynamics. | Earth gravity along `-z` |
| `friction_iterations` | The rounds of the friction fixed-point iteration (the friction bound of each round is `friction` times the normal force of the previous one), at least `1`; frictionless problems need one. | `12` |
| `contact_threshold` | A contact acts while its gap is at most this many metres, non-negative. | `1e-6` |

## Returns

A value; read by `constrain`.

## Errors

`constrain` and `impact` throw `pluss::Error` for a negative or non-finite `contact_threshold`, a `friction_iterations` below `1` or a malformed gravity.

## Interpretation and Use

A threshold of zero makes a contact act only when the point touches or penetrates the plane. A small positive value keeps a numerically resting robot supported.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The threshold decides whether a near point is supported.

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
### Example 2

Different gravities.

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
### Example 3

Contacts of a resting slider.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Pins.html">
    <span class="label">Previous</span>
    <span class="title">Pins</span>
  </a>
  <a class="next-link" href="ConstrainedResult.html">
    <span class="label">Next</span>
    <span class="title">ConstrainedResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
