<!-- generated from Links.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::Links`

The description of a robot as a tree of rigid links.

## Signature

```cpp
robot::Links
```

## What It Does

Describes a robot with one joint per link. Link `i` hangs from link `parent[i]` (`-1` for the base), and a parent always comes before its children. At zero joint value the link's joint frame sits at `translation[i]` with orientation `rpy[i]` in its parent's frame. A revolute joint turns the link about `axis[i]`, a prismatic joint slides it along `axis[i]`; the axis is expressed in the joint frame. Mass properties are expressed in the link's own frame, which is its joint frame.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `parent` | The parent of each link: `-1` or an earlier link's index. Length `n`. | required |
| `prismatic` | `0` for a revolute joint and `1` for a prismatic joint, one per link. | required |
| `axis` | The joint axis of each link in its joint frame, `[n, 3]`; non-zero, scaled to unit length by `make`. | required |
| `translation` | The origin of each joint frame in its parent frame at zero joint value, `[n, 3]`, in metres. | required |
| `rpy` | The orientation of each joint frame in its parent frame at zero joint value as roll-pitch-yaw angles, `[n, 3]`, in radians. | identity |
| `mass` | The mass of each link, `[n]`, non-negative. Without it the robot is a kinematic model. | undefined (no dynamics) |
| `center` | The centre of mass of each link in its own frame, `[n, 3]`. Needs `mass`. | the link origin |
| `inertia` | The inertia matrix of each link about its centre of mass in its own frame, `[n, 3, 3]`, symmetric. Needs `mass`. | zero |

## Returns

A value; validated and compiled by `make`.

## Errors

`make` throws `pluss::Error` for no links, a `prismatic` array of a different length or with an entry other than `0` or `1`, a parent outside `-1` or an earlier link, a zero or non-finite axis, an array of the wrong shape, dtype or platform, a negative mass, a `center` or `inertia` without `mass`, or an inertia matrix that is not symmetric.

## Interpretation and Use

All values must share one real floating dtype and platform; the robot computes in that dtype. A joint value is a radian for a revolute joint and a metre for a prismatic one. Several links may share a parent, which is how branches (two arms on one trunk) are described.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A planar two-link arm.

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

A tree with a branch and a prismatic joint.

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

A kinematic-only robot.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  // a robot without mass properties is a kinematic model: kinematics work and dynamics are refused
  rb::Links links;
  links.parent = {-1};
  links.prismatic = {0};
  links.axis = mat({0, 0, 1}, {1, 3});
  links.translation = mat({0, 0, 0}, {1, 3});
  const rb::Robot robot = rb::make(links);
  out.writeln("has mass", robot.has_mass());
  out.writeln("pose of the only link", robot.links());
  try {
    (void)rb::inertia(robot, vec({0.1}));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
```

Output

```text
has mass false
pose of the only link 1
eng::robot::inertia: the robot has no mass properties; set mass (and optionally center and inertia) in Links
```
### Example 4

The mass fields move the gravity torque.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  // the same arm with the forearm mass doubled: the gravity torque at the shoulder grows
  const pluss::Series q = vec({0.3, 0.5});
  const rb::Robot light = rb::make(arm_links());
  auto heavy_links = arm_links();
  heavy_links.mass = vec({1.0, 4.0});
  const rb::Robot heavy = rb::make(heavy_links);
  out.writeln("gravity torque, forearm 2 kg", flat(rb::bias(light, q, vec({0, 0}), in_plane())));
  out.writeln("gravity torque, forearm 4 kg", flat(rb::bias(heavy, q, vec({0, 0}), in_plane())));
  // omitted center and inertia: a point mass at the link origin
  auto point = arm_links();
  point.center = pluss::Series{};
  point.inertia = pluss::Series{};
  out.writeln("gravity torque, point masses at the joints", flat(rb::bias(rb::make(point), q, vec({0, 0}), in_plane())));
}
```

Output

```text
gravity torque, forearm 2 kg [14.4487, 2.7339]
gravity torque, forearm 4 kg [26.5544, 5.4678]
gravity torque, point masses at the joints [9.3719, 0.0000]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rpy.html">
    <span class="label">Previous</span>
    <span class="title">rpy</span>
  </a>
  <a class="next-link" href="RobotModel.html">
    <span class="label">Next</span>
    <span class="title">Robot</span>
  </a>
</div>
<!-- apiref:nav:end -->
