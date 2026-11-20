<!-- generated from has_mass.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 740bf57972f8 -->

# `robot::Robot::has_mass`

[Back to `robot::Robot`](../RobotModel.md)

Whether mass properties were given.

## Signature

```cpp
bool robot::Robot::has_mass() const noexcept;
```

## What It Does

Returns true when the `Links` carried masses, centres and inertias. The dynamics functions (inverse and forward dynamics, the mass matrix) need them and refuse a model without; the kinematics functions (poses, Jacobians, inverse kinematics) do not.

## Returns

`true` when the model has mass properties.

## Practical Examples

### Example 1

A planar two-link arm with masses 1 kg and 2 kg.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rb = pluss::subsp::eng::robot;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  const rb::Robot obj = rb::make(links);
  out.writeln("has mass =", obj.has_mass());
}
```

Output

```text
has mass = true
```

### Example 2

A four-link branching tree with one prismatic joint and no mass properties.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rb = pluss::subsp::eng::robot;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  rb::Links links;
  links.parent = {-1, 0, 0, 2};
  links.prismatic = {0, 0, 1, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0}, {4, 3});
  links.translation = mat({0, 0, 0.3, 0.2, 0, 0.1, -0.2, 0, 0.1, 0, 0, 0.3}, {4, 3});
  links.rpy = mat({0, 0, 0, 0.2, 0, 0.1, 0, -0.1, 0, 0.3, 0.2, 0}, {4, 3});
  const rb::Robot obj = rb::make(links);
  out.writeln("has mass =", obj.has_mass());
}
```

Output

```text
has mass = false
```

### Example 3

A serial chain of three revolute joints, with no mass properties.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rb = pluss::subsp::eng::robot;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  rb::Links links;
  links.parent = {-1, 0, 1};
  links.prismatic = {0, 0, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 1, 0, 0}, {3, 3});
  links.translation = mat({0, 0, 0.3, 0.3, 0, 0, 0.3, 0, 0}, {3, 3});
  const rb::Robot obj = rb::make(links);
  out.writeln("has mass =", obj.has_mass());
}
```

Output

```text
has mass = false
```

## See Also

- [`links`](links.md): the link count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtype.html">
    <span class="label">Previous</span>
    <span class="title">dtype</span>
  </a>
  <a class="next-link" href="links.html">
    <span class="label">Next</span>
    <span class="title">links</span>
  </a>
</div>
<!-- apiref:nav:end -->
