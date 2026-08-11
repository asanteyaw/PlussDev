<!-- pluss-hand-authored -->

# Studio::extent

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
const Extent2d& Studio::extent() const noexcept
```

## What It Does

Returns the figure's current canvas size — whatever [`size()`](size.md) most recently set,
or the `1280.0`×`820.0` default if `size()` was never called.

## Parameters

*(none)*

## Returns

`const Extent2d&`
    ```cpp
    struct Extent2d {
      double width{1280.0};
      double height{820.0};
    };
    ```
    A const reference to a simple two-field struct.

## Examples

Example 1. Reading the default extent, and after an explicit `size()` call.

```cpp
using namespace pluss::pl;

auto fig = studio("size_default");
std::cout << fig.extent().width << "x" << fig.extent().height << "\n";

fig.size(900.0, 500.0);
std::cout << fig.extent().width << "x" << fig.extent().height << "\n";
```

### Output

```text
1280x820
900x500
```

## Notes

- `noexcept`; `Extent2d{1280.0, 820.0}` is the well-defined default for a figure where
  `size()` has never been called.
- Reflects whatever `size()` last set, even a degenerate (zero/negative/non-finite) value —
  `extent()` performs no validation of its own, since `size()` itself performs none either.

## See Also

- [`Studio::size`](size.md): the only way to change the extent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="mosaic.html">
    <span class="label">Next</span>
    <span class="title">mosaic</span>
  </a>
</div>
