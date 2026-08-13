<!-- pluss-hand-authored -->

# Pane::align_y

[Back to Pane API Reference](../../pane_api_reference)

## Signatures

```cpp
Pane& Pane::align_y(std::string group)          // opts into an alignment group
Pane& Pane::clear_y_align() noexcept             // opts back out (empty group)
```

## What It Does

The y-axis counterpart of [`align_x()`](align_x.md)/[`clear_x_align()`](align_x.md):
opts this pane's y-axis into a named alignment group, matched against
[`Studio::y_caption(group, text)`](../studio/y_caption.md). Every detail carries over
unchanged — see [`align_x()`](align_x.md) for the full worked-example set.

## Parameters

**group** (`std::string`, setter only)
    The alignment-group key.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. Default (empty) group, and `clear_y_align()` resetting an explicit one back to
empty.

```cpp
using namespace pluss::pl;

auto fig = studio("align_y_check");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});
std::cout << pane.y_align_group().empty() << "\n";

pane.align_y("some_group");
std::cout << pane.y_align_group() << "\n";

pane.clear_y_align();
std::cout << pane.y_align_group().empty() << "\n";
fig.show();
```

### Output

```text
1
some_group
1
```

![Output of Example 1](img/align_y-1-check.svg)

## Notes

See [`align_x()`](align_x.md)'s Notes — every detail applies identically here, including
the reference-invalidation caution for reading the group back across multiple pane
creations.

## See Also

- [`Pane::align_x`](align_x.md): the x-axis equivalent, with the full worked-example set
  including a real `Studio::x_caption` pairing.
- [`Pane::align_z`](align_z.md): the z-axis equivalent.
- [`Pane::align_xy`](align_xy.md): sets both x and y groups to the same string at once.
- [`Studio::y_caption`](../studio/y_caption.md): the figure-wide caption this group string
  is matched against.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_x.html">
    <span class="label">Previous</span>
    <span class="title">align_x</span>
  </a>
  <a class="next-link" href="align_z.html">
    <span class="label">Next</span>
    <span class="title">align_z</span>
  </a>
</div>
