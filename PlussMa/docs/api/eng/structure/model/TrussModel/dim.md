<!-- generated from dim.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b6a353b63fab -->

# `structure::Truss::dim`

[Back to `structure::Truss`](../TrussModel.md)

The number of spatial axes: 2 for a plane truss, 3 for a space truss.

## Signature

```cpp
std::int64_t structure::Truss::dim() const noexcept;
```

## What It Does

Returns the dimension of the node coordinates. It is `2` for a plane truss and `3` for a space truss, and fixes how many displacement components each node has.

## Returns

`2` or `3`.

## Practical Examples

### Example 1

A plane truss: two bars from the feet to the apex.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const structure::Mesh mesh{grid({0, 0, 6, 0, 3, 4}, 2), {0, 1}, {2, 2}};
  const auto obj = structure::truss(mesh, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  out.writeln("axes =", obj.dim(), " displacements", obj.dim() * obj.nodes());
}
```

Output

```text
axes = 2  displacements 6
```

### Example 2

A space truss: three legs from a ring on the ground to an apex 2 m above its centre.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const structure::Mesh mesh{grid({1.5, 0, 0, -0.75, 1.299038105676658, 0, -0.75, -1.299038105676658, 0, 0, 0, 2}, 3), {0, 1, 2}, {3, 3, 3}};
  const auto obj = structure::truss(mesh, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  out.writeln("axes =", obj.dim(), " displacements", obj.dim() * obj.nodes());
}
```

Output

```text
axes = 3  displacements 12
```

### Example 3

A closed triangle: three bars between three nodes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const structure::Mesh mesh{grid({0, 0, 6, 0, 3, 4}, 2), {0, 1, 0}, {2, 2, 1}};
  const auto obj = structure::truss(mesh, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  out.writeln("axes =", obj.dim(), " displacements", obj.dim() * obj.nodes());
}
```

Output

```text
axes = 2  displacements 6
```

## See Also

- [`nodes`](nodes.md): the node count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bars.html">
    <span class="label">Previous</span>
    <span class="title">bars</span>
  </a>
  <a class="next-link" href="nodes.html">
    <span class="label">Next</span>
    <span class="title">nodes</span>
  </a>
</div>
<!-- apiref:nav:end -->
