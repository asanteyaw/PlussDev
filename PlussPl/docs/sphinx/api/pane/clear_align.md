<!-- pluss-hand-authored -->

# Pane::clear_align

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::clear_align() noexcept
```

## What It Does

Clears **all three** axis alignment groups at once — equivalent to calling
[`clear_x_align()`](align_x.md), [`clear_y_align()`](align_y.md), and
[`clear_z_align()`](align_z.md) together. The bulk counterpart of
[`align_xyz()`](align_xyz.md), which sets all three at once; `clear_align()` is the matching
all-three-at-once way to undo it (or to undo any mix of `align_x`/`align_y`/`align_z`
calls, regardless of whether they used matching group strings).

## Parameters

*(none)*

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. All three groups set via `align_xyz()`, then cleared together.

```cpp
using namespace pluss::pl;

auto fig = studio("clear_align_check");
auto& pane = fig.pane3();
pane.path3(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});
pane.align_xyz("all_shared");
std::cout << (!pane.x_align_group().empty() && !pane.y_align_group().empty() && !pane.z_align_group().empty()) << "\n";

pane.clear_align();
std::cout << (pane.x_align_group().empty() && pane.y_align_group().empty() && pane.z_align_group().empty()) << "\n";
fig.show();
```

### Output

```text
1
1
```

![Output of Example 1](img/clear_align-1-check.svg)

## Notes

- `noexcept`.
- Clears all three groups regardless of how they were set (`align_x`/`align_y`/`align_z`
  individually, or `align_xy`/`align_xyz`) — there's no way to clear only two of the three
  through this method; use the individual `clear_x_align()`/`clear_y_align()`/
  `clear_z_align()` for that.
- Harmless no-op if none of the three groups were set.

## See Also

- [`Pane::align_xyz`](align_xyz.md): sets all three groups at once, the operation this
  undoes.
- [`Pane::align_x`](align_x.md) / [`Pane::align_y`](align_y.md) / [`Pane::align_z`](align_z.md):
  the individual clears (`clear_x_align`/`clear_y_align`/`clear_z_align`) for clearing
  just one axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_xyz.html">
    <span class="label">Previous</span>
    <span class="title">align_xyz</span>
  </a>
  <a class="next-link" href="align_group_accessors.html">
    <span class="label">Next</span>
    <span class="title">align_group_accessors</span>
  </a>
</div>
