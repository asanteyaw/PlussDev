<!-- generated from RobotModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::Robot`

An immutable compiled robot.

## Signature

```cpp
std::int64_t robot::Robot::links() const noexcept;
pluss::ScalarType robot::Robot::dtype() const noexcept;
pluss::Platform robot::Robot::platform() const noexcept;
bool robot::Robot::has_mass() const noexcept;
const std::vector<std::int64_t>& robot::Robot::parent() const noexcept;
```

## What It Does

What `robot::make` returns: the validated tree with its constant transforms compiled, ready for any number of kinematic and dynamic calls on any batch of configurations.

## Methods

`links()`
: The number of links, which is also the number of joint values of a configuration.

`dtype(), platform()`
: The element type and device of the compiled data. Configurations are converted to this dtype.

`has_mass()`
: Whether mass properties were given. The dynamics functions need them; the kinematics functions do not.

`parent()`
: The parent index of every link, `-1` for the base.

## Returns

Accessors return counts, flags and the parent list.

## Errors

Accessors do not fail. Invalid descriptions are rejected by `make`.

## Changelog

- **0.1** — Initial public contract.

## Interpretation and Use

A `Robot` is immutable and cheap to copy; build it once and reuse it. Its compiled transforms carry reverse sensitivity to the values they were built from.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The accessors of a tree.

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
  out.writeln("links", robot.links());
  out.writeln("double precision", robot.dtype() == pluss::ScalarType::Float64);
  out.writeln("has_mass", robot.has_mass());
  out.writeln("parent", robot.parent());
}
```

Output

```text
links 4
double precision true
has_mass false
parent [-1, 0, 0, 2]
```
### Example 2

A planar arm and its parents.

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
### Example 3

A tree.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Links.html">
    <span class="label">Previous</span>
    <span class="title">Links</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
