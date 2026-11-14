<!-- generated from rotation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::rotation`

The rotation matrix of a turn about an axis.

## Signature

```cpp
pluss::Series robot::rotation(const pluss::Series& axis, const pluss::Series& angle);
```

## What It Does

Builds the rotation matrix of a turn by `angle` radians about `axis` (the right-hand rule), by Rodrigues' formula. The axis need not be a unit vector. Leading axes of `axis` and `angle` broadcast, so one call builds the rotations of a whole batch of angles, axes or both.

## Parameters

`axis`
: The turning axis, a real floating `[..., 3]` array; every axis must be non-zero.

`angle`
: The turning angle in radians, a real floating array whose leading axes broadcast against those of `axis`.

## Returns

A real `[..., 3, 3]` array: row `i`, column `j` of the rotation matrix of each turn. It maps a column vector `v` to `R v`.

## Errors

Throws `pluss::Error` when `axis` is not a real floating `[..., 3]` array, when `angle` is not real floating, or when an axis has zero length.

## Interpretation and Use

The matrices are orthonormal with determinant one, to rounding. Multiply them to compose turns: `rotation(a, x).matmul(rotation(b, y))` turns about `b` by `y` first, then about `a` by `x`. The result carries reverse sensitivity to `angle` and `axis`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A quarter turn about z, and a non-unit axis.

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
### Example 2

Batches of angles and of axes.

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
  // one axis and three angles, and three axes with one angle
  const pluss::Series angles = vec({0.0, M_PI / 4, M_PI / 2});
  const pluss::Series turns = rb::rotation(vec({0, 0, 1}), angles);
  out.writeln("angles batch shape", turns.sizes(), " (angle, row, column)");
  out.writeln("R(pi/4) first row", flat(turns.select(0, 1).select(0, 0)));
  const pluss::Series axes = mat({1, 0, 0, 0, 1, 0, 0, 0, 1}, {3, 3});
  const pluss::Series by_axis = rb::rotation(axes, vec({M_PI / 2}).select(0, 0));
  out.writeln("axes batch shape", by_axis.sizes());
  out.writeln("rotated z axis about each axis", flat(by_axis.matmul(vec({0, 0, 1}).unsqueeze(-1))));
}
```

Output

```text
angles batch shape [3, 3, 3]  (angle, row, column)
R(pi/4) first row [0.70711, -0.70711, 0.00000]
axes batch shape [3, 3, 3]
rotated z axis about each axis [0.00000, -1.00000, 0.00000, 1.00000, 0.00000, 0.00000, 0.00000, 0.00000, 1.00000]
```
### Example 3

Rejected axes.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series angle = vec({0.5}).select(0, 0);
  report([&] { (void)rb::rotation(vec({0, 0, 0}), angle); });
  report([&] { (void)rb::rotation(vec({1, 0}), angle); });
  report([&] { (void)rb::rotation(pluss::series::make(std::vector<std::int64_t>{0, 0, 1}), angle); });
}
```

Output

```text
eng::robot::rotation: axis must not be zero
eng::robot::rotation: axis must be a real floating [..., 3] array
eng::robot::rotation: axis must be a real floating [..., 3] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="rpy.html">
    <span class="label">Next</span>
    <span class="title">rpy</span>
  </a>
</div>
<!-- apiref:nav:end -->
