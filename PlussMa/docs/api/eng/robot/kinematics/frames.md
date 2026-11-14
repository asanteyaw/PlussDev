<!-- generated from frames.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::frames`

The world frames of every link for given joint values.

## Signature

```cpp
robot::Pose robot::frames(const robot::Robot& robot, const pluss::Series& q);
```

## What It Does

Composes the fixed transform and the joint motion of every link down the tree: the world rotation of link `i` is that of its parent times the link's own rotation, and its world position is the parent's position plus the parent's rotation applied to the link's offset. A revolute joint turns the link about its axis; a prismatic joint slides it along it.

## Parameters

`robot`
: The compiled robot.

`q`
: The joint values: a real floating `[..., links]` array on the robot's platform, in radians for revolute joints and metres for prismatic ones. Leading axes are a batch of configurations.

## Returns

A `Pose` with `rotation` `[..., links, 3, 3]` and `position` `[..., links, 3]`.

## Errors

Throws `pluss::Error` when `q` is not a real floating `[..., links]` array on the robot's platform.

## Interpretation and Use

The call is vectorized over the batch of configurations: a trajectory of a thousand configurations is one call. The result carries reverse sensitivity to `q` and to the values the robot was built from.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The elbow and forearm of a planar arm against closed forms.

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
### Example 2

Three configurations in one call.

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
  // three configurations at once: a leading axis on q is a batch
  const pluss::Series q = mat({0.0, 0.0, 0.6, -0.9, M_PI / 2, 0.0}, {3, 2});
  const rb::Pose pose = rb::frames(robot, q);
  out.writeln("position shape", pose.position.sizes(), " (configuration, link, xyz)");
  out.writeln("elbow x for each configuration", flat(pose.position.select(1, 1).select(-1, 0)));
  out.writeln("elbow y for each configuration", flat(pose.position.select(1, 1).select(-1, 1)));
}
```

Output

```text
position shape [3, 2, 3]  (configuration, link, xyz)
elbow x for each configuration [0.50000, 0.41267, 0.00000]
elbow y for each configuration [0.00000, 0.28232, 0.50000]
```
### Example 3

A branching tree: which links move when a joint moves.

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
### Example 4

Rejected configurations.

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
  const rb::Robot robot = rb::make(arm_links());
  report([&] { (void)rb::frames(robot, vec({0.1, 0.2, 0.3})); });
  report([&] { (void)rb::frames(robot, pluss::series::make(std::vector<std::int64_t>{1, 2})); });
  report([&] { (void)rb::frames(robot, pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
}
```

Output

```text
eng::robot::frames: q must be a real floating [..., 2] array
eng::robot::frames: q must be a real floating [..., 2] array
eng::robot::frames: q must be a real floating [..., 2] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Pose.html">
    <span class="label">Previous</span>
    <span class="title">Pose</span>
  </a>
  <a class="next-link" href="jac.html">
    <span class="label">Next</span>
    <span class="title">jac</span>
  </a>
</div>
<!-- apiref:nav:end -->
