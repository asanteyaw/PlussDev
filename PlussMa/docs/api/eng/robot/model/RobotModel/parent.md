<!-- generated from parent.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69c183e1ee21 -->

# `robot::Robot::parent`

[Back to `robot::Robot`](../RobotModel.md)

The parent index of every link; `-1` for the base.

## Signature

```cpp
const std::vector<std::int64_t>& robot::Robot::parent() const noexcept;
```

## What It Does

Returns, for each link, the index of the link it hangs from, or `-1` for a link attached to the fixed base. A serial chain has parents `-1, 0, 1, ...`; a branching tree has repeated parents. Parents always precede their children.

## Returns

A reference to the parent indices.

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
  out.writeln("parent =", obj.parent());
}
```

Output

```text
parent = [-1, 0]
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
  out.writeln("parent =", obj.parent());
}
```

Output

```text
parent = [-1, 0, 0, 2]
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
  out.writeln("parent =", obj.parent());
}
```

Output

```text
parent = [-1, 0, 1]
```

## See Also

- [`links`](links.md): the link count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="links.html">
    <span class="label">Previous</span>
    <span class="title">links</span>
  </a>
  <a class="next-link" href="platform.html">
    <span class="label">Next</span>
    <span class="title">platform</span>
  </a>
</div>
<!-- apiref:nav:end -->
