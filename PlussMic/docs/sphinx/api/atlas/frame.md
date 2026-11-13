<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c9f70ec86ffe -->

# Atlas::frame

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
DataFrame Atlas::frame(std::int64_t index) const
```

## What It Does

Retrieves one frame from the stack by its depth position, returning it as a plain
`DataFrame`. `index` supports Python-style negative indexing: `-1` is the last frame,
`-2` the second-to-last, and so on, the same convention [`dloc()`](dloc.md) (its exact
synonym) and slicing methods like [`head()`](head.md)/[`tail()`](tail.md) use elsewhere
on `Atlas`.

## Parameters

**index** (`std::int64_t`)

Zero-based position in the stack. Negative values count back from the end
(`-1` is `depth() - 1`). Out-of-range values (after negative-index normalization) throw.

## Returns

`DataFrame`
    The frame at the requested depth position.

## Examples

Example 1. Retrieve a frame by its forward position.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << atlas.frame(0);
```

### Output

```text
==========
     | sales
----------
0    | 100
1    | 120
----------
[2 cs x 1 feat]
```

Example 2. A negative index counts back from the end of the stack.

```cpp
std::cout << atlas.frame(-1);
```

### Output

```text
==========
     | sales
----------
0    | 110
1    | 90
----------
[2 cs x 1 feat]
```

`frame(-1)` returns the same frame as `frame(1)` here, since the stack has two frames —
`q2`, the last one.

Example 3. An out-of-range index throws, rather than clamping or returning an undefined
frame.

```cpp
try {
  auto out = atlas.frame(5);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas: frame index out of bounds
```

## Notes

- `frame()` and [`dloc()`](dloc.md) are exact synonyms — `dloc()` forwards directly to
  `frame()`. Use whichever name reads better at the call site.
- The returned `DataFrame` is a cheap handle copy, not a deep copy — retrieving the same
  frame twice does not duplicate its underlying data.
- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.

## See Also

- [`Atlas::dloc`](dloc.md): the exact same operation, under a second name.
- [`Atlas::frames`](frames.md): retrieve every frame at once as a `std::vector<DataFrame>`.
- [`Atlas::loc`](loc.md): retrieve a frame by its depth **label** instead of its
  positional index.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow.html">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="frames.html">
    <span class="label">Next</span>
    <span class="title">frames</span>
  </a>
</div>
