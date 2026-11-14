<!-- generated from Pose.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::Pose`

The frames of every link in the world.

## Signature

```cpp
robot::Pose
```

## What It Does

The result of `frames`: the orientation and the position of every link's frame in the world frame, for every configuration of a batch.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `rotation` | The rotation matrix of each link frame in the world, `[..., links, 3, 3]`. |  |
| `position` | The origin of each link frame in the world, `[..., links, 3]`, in metres. |  |

## Returns

Returned by `frames`.

## Errors

Reading a field does not compute again.

## Interpretation and Use

The base frame is the world frame. The position of a link is the origin of its joint frame, so a prismatic joint's link moves with its joint value. Select one link with `select(-3, link)` on `rotation` and `select(-2, link)` on `position`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The shapes of both fields.

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
  const rb::Robot robot = rb::make(arm_links());
  const rb::Pose pose = rb::frames(robot, mat({0.6, -0.9, 0.1, 0.2}, {2, 2}));
  out.writeln("rotation shape", pose.rotation.sizes(), " (configuration, link, row, column)");
  out.writeln("position shape", pose.position.sizes(), " (configuration, link, xyz)");
  out.writeln("link 1 frame of configuration 0: rotation then position");
  out.writeln(flat(pose.rotation.select(0, 0).select(0, 1)));
  out.writeln(flat(pose.position.select(0, 0).select(0, 1)));
}
```

Output

```text
rotation shape [2, 2, 3, 3]  (configuration, link, row, column)
position shape [2, 2, 3]  (configuration, link, xyz)
link 1 frame of configuration 0: rotation then position
[0.9553, 0.2955, 0.0000, -0.2955, 0.9553, 0.0000, 0.0000, 0.0000, 1.0000]
[0.4127, 0.2823, 0.0000]
```
### Example 2

A planar arm.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(arm_links());
  const pluss::Series q = vec({0.6, -0.9});
  const rb::Pose pose = rb::frames(robot, q);
  out.writeln("position shape", pose.position.sizes(), " rotation shape", pose.rotation.sizes());
  out.writeln("shoulder, elbow positions", flat(pose.position));
  out.writeln("elbow closed form (l1 cos q1, l1 sin q1)", 0.5 * std::cos(0.6), 0.5 * std::sin(0.6));
  out.writeln("forearm rotation about z is q1 + q2 =", 0.6 - 0.9, " cos and sin", flat(pose.rotation.select(0, 1))[0], flat(pose.rotation.select(0, 1))[3]);
}
```

Output

```text
position shape [2, 3]  rotation shape [2, 3, 3]
shoulder, elbow positions [0.00000, 0.00000, 0.00000, 0.41267, 0.28232, 0.00000]
elbow closed form (l1 cos q1, l1 sin q1) 0.41267 0.28232
forearm rotation about z is q1 + q2 = -0.30000  cos and sin 0.95534 -0.29552
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
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(tree_links());
  const rb::Pose pose = rb::frames(robot, vec({0.3, -0.5, 0.25, 0.8}));
  out.writeln("position shape", pose.position.sizes());
  for (int link = 0; link < 4; ++link) out.writeln("link", link, flat(pose.position.select(0, link)));
  // the wrist (link 3) hangs from the rail (link 2): sliding the rail moves it, rotating the shoulder (link 1) does not
  const rb::Pose slid = rb::frames(robot, vec({0.3, -0.5, 0.75, 0.8}));
  const rb::Pose turned = rb::frames(robot, vec({0.3, 1.5, 0.25, 0.8}));
  out.writeln("wrist moved by sliding the rail", flat(slid.position.select(0, 3).sub(pose.position.select(0, 3))));
  out.writeln("wrist moved by turning the shoulder", flat(turned.position.select(0, 3).sub(pose.position.select(0, 3))));
}
```

Output

```text
position shape [4, 3]
link 0 [0.00000, 0.00000, 0.30000]
link 1 [0.19107, 0.05910, 0.40000]
link 2 [0.04657, 0.01441, 0.42496]
link 3 [0.01796, 0.00556, 0.72346]
wrist moved by sliding the rail [0.47528, 0.14702, 0.04992]
wrist moved by turning the shoulder [0.00000, 0.00000, 0.00000]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="frames.html">
    <span class="label">Next</span>
    <span class="title">frames</span>
  </a>
</div>
<!-- apiref:nav:end -->
