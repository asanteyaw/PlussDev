<!-- pluss-hand-authored -->

# Studio::mosaic_layout

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
const MosaicLayout& Studio::mosaic_layout() const noexcept
```

## What It Does

Returns the figure's current mosaic configuration — whatever [`mosaic()`](mosaic.md) most
recently set, or an inactive, all-zero default if `mosaic()` was never called.

## Parameters

*(none)*

## Returns

`const MosaicLayout&`
    ```cpp
    struct MosaicLayout {
      std::int64_t rows{0};
      std::int64_t cols{0};
      double gutter_x{18.0};
      double gutter_y{18.0};
      bool active{false};
    };
    ```
    `active` is `true` only after a successful [`mosaic()`](mosaic.md) call — a `Studio`
    that never called `mosaic()` reports `rows = 0`, `cols = 0`, `active = false`, even if it
    has several panes placed via explicit [`subpane2()`](subpane2.md)/
    [`subpane3()`](subpane3.md) slots (the renderer infers an *effective* grid from those
    slots at render time, but that inferred grid is never written back into this accessor's
    return value — `mosaic_layout()` only ever reflects an explicit `mosaic()` call).

## Examples

Example 1. No `mosaic()` call: `active` is `false`.

```cpp
using namespace pluss::pl;

auto fig = studio("mosaic_single");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0});

std::cout << fig.mosaic_layout().active << "\n";
```

### Output

```text
0
```

Example 2. After `mosaic(2, 2)`.

```cpp
using namespace pluss::pl;

auto fig = studio("mosaic_2x2");
fig.mosaic(2, 2);

std::cout << fig.mosaic_layout().rows << "x" << fig.mosaic_layout().cols
          << " active=" << fig.mosaic_layout().active << "\n";
```

### Output

```text
2x2 active=1
```

## Notes

- `noexcept`.
- `active` distinguishes "no `mosaic()` call yet" from "`mosaic()` was called" — check it
  before trusting `rows`/`cols` to mean anything.
- Only reflects an explicit `mosaic()` call — a grid inferred from `subpane2()`/`subpane3()`
  explicit slots at render time is not written back here.

## See Also

- [`Studio::mosaic`](mosaic.md): the only way to change this.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mosaic.html">
    <span class="label">Previous</span>
    <span class="title">mosaic</span>
  </a>
  <a class="next-link" href="tight_layout.html">
    <span class="label">Next</span>
    <span class="title">tight_layout</span>
  </a>
</div>
