<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e8678436b11 -->

# Atlas::depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::depth() const -> std::int64_t
```

## What It Does

Returns the number of frames stacked in the atlas — the third axis of the cube, beyond a
single frame's own cases (rows) and features (columns). An `Atlas` is a strict cube:
every one of its `depth()` frames shares the same `ncases()`, the same `nfeatures()`, and
the same column names, kinds, and dtypes (enforced at construction time — mismatched
frames throw immediately rather than being silently stored). `depth()` is that third
dimension's size, the same way [`ncases()`](ncases.md) and [`nfeatures()`](nfeatures.md)
report the other two.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of frames in the stack. `0` for an atlas built from an empty frame
    vector.

## Examples

Example 1. Three quarters stacked into one atlas.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto q3 = DataFrame({{"sales", Slate(pluss::make_series({130.0, 140.0}))}});
auto atlas = Atlas({q1, q2, q3});

std::cout << atlas.depth();
```

### Output

```text
3
```

Example 2. `depth()` throws on an undefined atlas — unlike `empty()`, which treats an
undefined atlas as trivially empty instead.

```cpp
using namespace pluss::table;

Atlas undefined_atlas;
try {
  auto d = undefined_atlas.depth();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas: undefined handle
```

## Notes

- Requires a defined `Atlas` ([`defined()`](defined.md)); throws `"Atlas: undefined
  handle"` on a default-constructed `Atlas()`.
- Every frame in the stack has identical `ncases()`/`nfeatures()`/column schema by
  construction — `depth()` is purely the count of how many such frames are stacked, not a
  measurement of any single frame's size.

## See Also

- [`Atlas::ncases`](ncases.md) / [`Atlas::nfeatures`](nfeatures.md): the other two axes
  of the cube — every frame's shared row count and column count.
- [`Atlas::frame`](frame.md) / [`Atlas::dloc`](dloc.md): retrieve one frame by its
  position along this axis.
- [`Atlas::empty`](empty.md): a `depth() == 0` check that's also safe on an undefined
  `Atlas`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="dloc.html">
    <span class="label">Next</span>
    <span class="title">dloc</span>
  </a>
</div>
