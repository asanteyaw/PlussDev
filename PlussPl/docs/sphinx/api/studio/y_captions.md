<!-- pluss-hand-authored -->

# Studio::y_captions

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
const std::map<std::string, std::string>& Studio::y_captions() const noexcept
```

## What It Does

The y-axis counterpart of [`x_captions`](x_captions.md): returns the full map of
registered y-axis captions — every `group → text` pair set via
[`y_caption(group, text)`](y_caption.md), keyed by group string, sorted lexicographically.
Empty if `y_caption()` has never been called.

## Parameters

*(none)*

## Returns

`const std::map<std::string, std::string>&`
    A reference to the figure's internal y-caption map.

## Examples

Example 1. Empty by default, then populated.

```cpp
using namespace pluss::pl;

auto fig = studio("y_captions_check");
std::cout << fig.y_captions().empty() << "\n";

fig.y_caption("shared_y", "Temperature (C)");
std::cout << fig.y_captions().size() << "\n";
std::cout << fig.y_captions().at("shared_y") << "\n";

auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0}).label("series");
fig.show();
```

### Output

```text
1
1
Temperature (C)
```

![Output of Example 1](img/y_captions-1-check.svg)

## Notes

- `noexcept`.
- Returns a reference into the `Studio`'s own storage.
- Same caveats as [`x_captions`](x_captions.md): sorted by key, doesn't reflect whether any
  pane's `align_y` actually matches.

## See Also

- [`Studio::y_caption`](y_caption.md): the only way to add or overwrite entries.
- [`Studio::x_captions`](x_captions.md) / [`z_captions`](z_captions.md): the x- and z-axis
  equivalents.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="y_caption.html">
    <span class="label">Previous</span>
    <span class="title">y_caption</span>
  </a>
  <a class="next-link" href="z_caption.html">
    <span class="label">Next</span>
    <span class="title">z_caption</span>
  </a>
</div>
