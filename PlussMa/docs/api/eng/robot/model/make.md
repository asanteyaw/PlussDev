<!-- generated from make.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::make`

Validates a robot description and compiles it.

## Signature

```cpp
robot::Robot robot::make(const robot::Links& links);
```

## What It Does

Checks a `Links` description and compiles the constant transforms (unit axes and the fixed joint-frame rotations) once, so that every analysis reuses them.

## Parameters

`links`
: The description of the robot.

## Returns

An immutable `robot::Robot`.

## Errors

Throws `pluss::Error` for a description with no links, an inconsistent length or shape, a bad parent, joint kind, axis, mass or inertia, or values of mixed dtype or platform. The message names the offending field.

## Interpretation and Use

Validation happens here once; the analyses only check their own arguments. A robot without `mass` compiles and supports kinematics.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A planar arm compiled.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
  return options;
}

int main() {
  OutputEngine out;
  const rb::Robot robot = rb::make(arm_links());
  out.writeln("links", robot.links(), " has mass", robot.has_mass());
  out.writeln("parent of each link", robot.parent());
}
```

Output

```text
links 2  has mass true
parent of each link [-1, 0]
```
### Example 2

A tree compiled.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a trunk (link 0, about z) with a shoulder (link 1, about y), a rail (link 2, a slide along x) and a wrist (link 3, about x) on the rail
static rb::Links tree_links() {
  rb::Links links;
  links.parent = {-1, 0, 0, 2};
  links.prismatic = {0, 0, 1, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0}, {4, 3});
  links.translation = mat({0, 0, 0.3, 0.2, 0, 0.1, -0.2, 0, 0.1, 0, 0, 0.3}, {4, 3});
  links.rpy = mat({0, 0, 0, 0.2, 0, 0.1, 0, -0.1, 0, 0.3, 0.2, 0}, {4, 3});
  return links;
}

int main() {
  OutputEngine out;
  const rb::Robot robot = rb::make(tree_links());
  out.writeln("links", robot.links(), " has mass", robot.has_mass());
  out.writeln("parent of each link", robot.parent(), " (links 1 and 2 both hang from link 0; link 3 from link 2)");
}
```

Output

```text
links 4  has mass false
parent of each link [-1, 0, 0, 2]  (links 1 and 2 both hang from link 0; link 3 from link 2)
```
### Example 3

Rejected descriptions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
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
  auto self = arm_links();
  self.parent = {-1, 1};
  report([&] { (void)rb::make(self); });
  auto no_axis = arm_links();
  no_axis.axis = mat({0, 0, 1, 0, 0, 0}, {2, 3});
  report([&] { (void)rb::make(no_axis); });
  auto kind = arm_links();
  kind.prismatic = {0, 2};
  report([&] { (void)rb::make(kind); });
  auto negative = arm_links();
  negative.mass = vec({1.0, -2.0});
  report([&] { (void)rb::make(negative); });
  auto shape = arm_links();
  shape.translation = mat({0, 0, 0}, {1, 3});
  report([&] { (void)rb::make(shape); });
  auto asymmetric = arm_links();
  asymmetric.inertia = mat({0, 1, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  report([&] { (void)rb::make(asymmetric); });
  rb::Links lonely;
  lonely.parent = {-1};
  lonely.prismatic = {0};
  lonely.axis = mat({0, 0, 1}, {1, 3});
  lonely.translation = mat({0, 0, 0}, {1, 3});
  lonely.center = mat({0, 0, 0}, {1, 3});
  report([&] { (void)rb::make(lonely); });
}
```

Output

```text
eng::robot::make: parent[i] must be -1 or the index of an earlier link
eng::robot::make: every joint axis must be non-zero
eng::robot::make: prismatic must be 0 (revolute) or 1 (prismatic)
eng::robot::make: mass must be non-negative
eng::robot::make: translation must be a real floating array with one row per link
eng::robot::make: every inertia matrix must be symmetric
eng::robot::make: center and inertia need mass
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="RobotModel.html">
    <span class="label">Previous</span>
    <span class="title">Robot</span>
  </a>
</div>
<!-- apiref:nav:end -->
