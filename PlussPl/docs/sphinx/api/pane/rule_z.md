<!-- pluss-hand-authored -->

# Pane::rule_z

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::rule_z(double value, std::string label = {})
```

## What It Does

The z-axis counterpart of [`rule_x()`](rule_x.md)/[`rule_y()`](rule_y.md), meaningful
primarily on 3D (`Volumetric`) panes. Appends to the exact same shared
[`rules()`](rules.md) list, tagged with `CueAxis::Z`. Every other detail carries over
unchanged — see [`rule_x()`](rule_x.md) for the full worked-example set.

## Parameters

**value** (`double`)
    The z-axis position for the rule line. Must be finite.

**label** (`std::string`, default `{}`)
    Optional text drawn alongside the rule line.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. A labeled rule line on the z-axis of a 3D pane.

```cpp
using namespace pluss::pl;

auto fig = studio("rule_z_basic");
auto& pane = fig.pane3();
pane.path3(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0});
pane.rule_z(2.0, "midpoint");
fig.show();

std::cout << pane.rules().size() << "\n";
```

### Output

```text
1
```

![Output of Example 1](img/rule_z-1-basic.svg)

## Notes

See [`rule_x()`](rule_x.md)'s Notes — every detail applies identically here. Most useful
on 3D panes; on 2D panes the z-axis state exists but is inert.

## See Also

- [`Pane::rule_x`](rule_x.md) / [`Pane::rule_y`](rule_y.md): the x- and y-axis
  equivalents, with the full worked-example set.
- [`Pane::rules`](rules.md): reads back every rule added so far.
- [`Pane::band_z`](band_z.md): a span (range) reference region on the z-axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rule_y.html">
    <span class="label">Previous</span>
    <span class="title">rule_y</span>
  </a>
  <a class="next-link" href="band_x.html">
    <span class="label">Next</span>
    <span class="title">band_x</span>
  </a>
</div>
