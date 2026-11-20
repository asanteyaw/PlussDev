<!-- generated from dofs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8de1093610f8 -->

# `structure::Frame::dofs`

[Back to `structure::Frame`](../FrameModel.md)

The degrees of freedom per node, always 3.

## Signature

```cpp
std::int64_t structure::Frame::dofs() const noexcept;
```

## What It Does

Returns `3`: a plane-frame node has the displacements along x and y and the rotation, counterclockwise positive. The total number of degrees of freedom of a frame is `dofs() * nodes()`.

## Returns

Always `3`.

## Practical Examples

### Example 1

A portal frame: two columns and a beam on four nodes.

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
  const structure::Mesh mesh{grid({0, 0, 0, 3, 4, 3, 4, 0}, 2), {0, 1, 3}, {1, 2, 2}};
  const structure::Sections sections{vec({2e-2, 2e-2, 2e-2}), vec({6.667e-5, 6.667e-5, 6.667e-5}), vec({200e9, 200e9, 200e9})};
  const auto obj = structure::frame(mesh, sections);
  out.writeln("dofs per node =", obj.dofs(), " total", obj.dofs() * obj.nodes());
}
```

Output

```text
dofs per node = 3  total 12
```

### Example 2

A 2 m cantilever beam divided into four members.

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
  const structure::Mesh mesh{grid({0, 0, 0.5, 0, 1, 0, 1.5, 0, 2, 0}, 2), {0, 1, 2, 3}, {1, 2, 3, 4}};
  const structure::Sections sections{vec({1e-3, 1e-3, 1e-3, 1e-3}), vec({8.333e-8, 8.333e-8, 8.333e-8, 8.333e-8}), vec({200e9, 200e9, 200e9, 200e9})};
  const auto obj = structure::frame(mesh, sections);
  out.writeln("dofs per node =", obj.dofs(), " total", obj.dofs() * obj.nodes());
}
```

Output

```text
dofs per node = 3  total 15
```

### Example 3

A single member between two nodes.

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
  const structure::Mesh mesh{grid({0, 0, 2, 0}, 2), {0}, {1}};
  const structure::Sections sections{vec({1e-3}), vec({8.333e-8}), vec({200e9})};
  const auto obj = structure::frame(mesh, sections);
  out.writeln("dofs per node =", obj.dofs(), " total", obj.dofs() * obj.nodes());
}
```

Output

```text
dofs per node = 3  total 6
```

## See Also

- [`nodes`](nodes.md): the node count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="members.html">
    <span class="label">Next</span>
    <span class="title">members</span>
  </a>
</div>
<!-- apiref:nav:end -->
