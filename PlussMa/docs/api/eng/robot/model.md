<!-- generated from model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robot model

This branch describes a robot. `Links` lists the rigid links as a tree, `make` validates it and compiles an immutable `Robot`, and `rotation` and `rpy` build the rotation matrices used to describe frames.

## Choosing A Surface

Describe the robot once, with one row per link, and compile it. Frames are described by a translation and roll-pitch-yaw angles in the parent frame.

## Practical Examples

The examples exercise the principal surface of this branch.

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

A rotation about an axis.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  // a quarter turn about z maps the x axis onto the y axis
  const pluss::Series quarter = rb::rotation(vec({0, 0, 1}), vec({M_PI / 2}).select(0, 0));
  out.writeln("rotation shape", quarter.sizes());
  out.writeln("R x", flat(quarter.matmul(vec({1, 0, 0}).unsqueeze(-1))));
  // the axis need not be a unit vector
  out.writeln("same rotation from the axis (0, 0, 5)", flat(rb::rotation(vec({0, 0, 5}), vec({M_PI / 2}).select(0, 0)).matmul(vec({1, 0, 0}).unsqueeze(-1))));
}
```

Output

```text
rotation shape [3, 3]
R x [0.00000, 1.00000, 0.00000]
same rotation from the axis (0, 0, 5) [0.00000, 1.00000, 0.00000]
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

model/rotation
model/rpy
model/Links
model/RobotModel
model/make
```
<!-- apiref:toctree:end -->
