<!-- pluss-hand-authored -->

# Pane::x_align_group / y_align_group / z_align_group

[Back to Pane API Reference](../../pane_api_reference)

## Signatures

```cpp
const std::string& Pane::x_align_group() const noexcept
const std::string& Pane::y_align_group() const noexcept
const std::string& Pane::z_align_group() const noexcept
```

## What It Does

Three read-only accessors, one per axis, each returning the current alignment-group
string set by [`align_x()`](align_x.md)/[`align_y()`](align_y.md)/[`align_z()`](align_z.md)
(or the combined [`align_xy()`](align_xy.md)/[`align_xyz()`](align_xyz.md)). Empty by
default on a fresh pane, and after [`clear_x_align()`](align_x.md)/
[`clear_y_align()`](align_y.md)/[`clear_z_align()`](align_z.md)/[`clear_align()`](clear_align.md).

## Parameters

*(none)*

## Returns

`const std::string&`
    The current alignment-group string for that axis, or empty if never set (or cleared).

## Examples

Example 1. Reading back a mix of group assignments across all three axes.

```cpp
using namespace pluss::pl;

auto fig = studio("align_group_accessors_check");
auto& pane = fig.pane3();
pane.path3(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});

std::cout << pane.x_align_group().empty() << "\n";

pane.align_x("time");
pane.align_y("amplitude");
pane.align_z("depth");

std::cout << pane.x_align_group() << " " << pane.y_align_group() << " " << pane.z_align_group() << "\n";
fig.show();
```

### Output

```text
1
time amplitude depth
```

![Output of Example 1](img/align_group_accessors-1-check.svg)

## Notes

- All three overloads are `noexcept`, `const` — pure, cheap readbacks.
- Empty is a well-defined default, not an error state.
- Reflects whichever of `align_x`/`align_y`/`align_z`/`align_xy`/`align_xyz` most recently
  wrote to that axis's group.

## See Also

- [`Pane::align_x`](align_x.md) / [`Pane::align_y`](align_y.md) / [`Pane::align_z`](align_z.md):
  the individual setters (and their paired `clear_*_align()` methods).
- [`Pane::align_xy`](align_xy.md) / [`Pane::align_xyz`](align_xyz.md): the multi-axis
  convenience setters.
- [`Pane::clear_align`](clear_align.md): clears all three at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_align.html">
    <span class="label">Previous</span>
    <span class="title">clear_align</span>
  </a>
  <a class="next-link" href="visible.html">
    <span class="label">Next</span>
    <span class="title">visible</span>
  </a>
</div>
