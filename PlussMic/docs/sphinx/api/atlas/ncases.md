<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6af5646f5d13 -->

# Atlas::ncases

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
std::int64_t Atlas::ncases() const
```

## What It Does

Returns the row count shared by every frame in the stack. Because an `Atlas` is a strict
cube, this is not "the row count of the first frame" as a convenience shortcut — every
frame is validated at construction time to have the exact same `ncases()`, so this single
number describes the whole stack, not just one slice of it.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of cases (rows) in every frame of the stack. `0` for an atlas built from
    an empty frame vector.

## Examples

Example 1. Every quarter in the stack has the same two cases.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << atlas.ncases();
```

### Output

```text
2
```

Example 2. Stacking frames with mismatched row counts throws immediately, at
construction — `ncases()` is never ambiguous about which frame it's describing.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2_wrong_shape = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0, 80.0}))}});

try {
  auto atlas = Atlas({q1, q2_wrong_shape});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas: all frames must have identical row count (strict cube)
```

## Notes

- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.
- Every frame in the stack shares this exact row count by construction — there is no
  concept of frames with different row counts coexisting in one `Atlas`.

## See Also

- [`Atlas::depth`](depth.md): the stack's own axis — how many frames share this row
  count.
- [`Atlas::nfeatures`](nfeatures.md): the other shared dimension, the column count.
- [`Atlas::column_names`](column_names.md): the names behind `nfeatures()`'s count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map_frames.html">
    <span class="label">Previous</span>
    <span class="title">map_frames</span>
  </a>
  <a class="next-link" href="nfeatures.html">
    <span class="label">Next</span>
    <span class="title">nfeatures</span>
  </a>
</div>
