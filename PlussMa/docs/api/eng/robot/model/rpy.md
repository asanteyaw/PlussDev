<!-- generated from rpy.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::rpy`

The rotation matrix of roll, pitch and yaw angles.

## Signature

```cpp
pluss::Series robot::rpy(const pluss::Series& angles);
```

## What It Does

Builds `R = Rz(yaw) Ry(pitch) Rx(roll)` from the three angles `[roll, pitch, yaw]`: a turn about the fixed x axis, then about the fixed y axis, then about the fixed z axis. This is the convention of the `rpy` field of `Links`.

## Parameters

`angles`
: Roll, pitch and yaw in radians, a real floating `[..., 3]` array; leading axes are a batch.

## Returns

A real `[..., 3, 3]` rotation matrix per set of angles.

## Errors

Throws `pluss::Error` when `angles` is not a real floating `[..., 3]` array.

## Interpretation and Use

Use `rpy` to author a joint frame's fixed orientation and to turn a camera or tool attitude given as three angles into the matrix that `ik` takes. Three angles are ambiguous at pitch of plus or minus a right angle (gimbal lock); work with the matrices, not the angles, when composing many turns.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The rpy matrix is Rz Ry Rx.

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
  // roll about x, then pitch about y, then yaw about z, all about the fixed axes: R = Rz Ry Rx
  const pluss::Series angles = vec({0.4, -0.7, 1.3});
  const pluss::Series product = rb::rotation(vec({0, 0, 1}), angles.select(0, 2))
                                    .matmul(rb::rotation(vec({0, 1, 0}), angles.select(0, 1)))
                                    .matmul(rb::rotation(vec({1, 0, 0}), angles.select(0, 0)));
  const pluss::Series direct = rb::rpy(angles);
  out.writeln("rpy first row   ", flat(direct.select(0, 0)));
  out.writeln("Rz Ry Rx row    ", flat(product.select(0, 0)));
  out.writeln("largest difference below 1e-12", direct.sub(product).abs().amax().item<double>() < 1e-12);
  // pure yaw is a rotation about z
  out.writeln("yaw 90 degrees, R x", flat(rb::rpy(vec({0, 0, M_PI / 2})).matmul(vec({1, 0, 0}).unsqueeze(-1))));
}
```

Output

```text
rpy first row    [0.20459, -0.95460, 0.21650]
Rz Ry Rx row     [0.20459, -0.95460, 0.21650]
largest difference below 1e-12 true
yaw 90 degrees, R x [0.00000, 1.00000, 0.00000]
```
### Example 2

A batch of angle triples.

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
  const pluss::Series angles = mat({0, 0, 0, 0, 0, M_PI / 2, M_PI / 2, 0, 0, 0.3, 0.2, 0.1}, {4, 3});
  const pluss::Series turns = rb::rpy(angles);
  out.writeln("rpy shape", turns.sizes(), " (batch, row, column)");
  out.writeln("trace of each rotation", flat(turns.select(-1, 0).select(-1, 0).add(turns.select(-2, 1).select(-1, 1)).add(turns.select(-2, 2).select(-1, 2))));
}
```

Output

```text
rpy shape [4, 3, 3]  (batch, row, column)
trace of each rotation [3.00000, 1.00000, 1.00000, 2.86789]
```
### Example 3

Rejected angle arrays.

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
  report([&] { (void)rb::rpy(vec({0.1, 0.2})); });
  report([&] { (void)rb::rpy(pluss::series::make(std::vector<std::int64_t>{1, 2, 3})); });
  report([&] { (void)rb::rpy(pluss::Series{}); });
}
```

Output

```text
eng::robot::rpy: angles must be a real floating [..., 3] array
eng::robot::rpy: angles must be a real floating [..., 3] array
eng::robot::rpy: angles must be a real floating [..., 3] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rotation.html">
    <span class="label">Previous</span>
    <span class="title">rotation</span>
  </a>
  <a class="next-link" href="Links.html">
    <span class="label">Next</span>
    <span class="title">Links</span>
  </a>
</div>
<!-- apiref:nav:end -->
