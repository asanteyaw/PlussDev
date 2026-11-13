<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3c8124f99c94 -->

# Atlas::map

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas Atlas::map(const std::function<SlateValue(const SlateValue&)>& fn) const
```

## What It Does

Applies `fn` to every cell of every feature, in every frame, one scalar value at a time —
the whole-atlas analog of [`DataFrame::map`](../dataframe/map.md). `fn` receives one cell
as a `SlateValue` and must return one `SlateValue` back. It is skipped entirely for
missing cells (a `NULL` cell stays `NULL` without `fn` ever being called), and every
non-null cell within one feature's column must map to the same output kind.

## Parameters

**fn** (`const std::function<SlateValue(const SlateValue&)>&`)
    A callback receiving one cell and returning its replacement. Must not be empty.

## Returns

`Atlas`
    A new atlas, same shape as the receiver, with every non-null cell replaced by `fn`'s
    result.

## Examples

Example 1. Scale every numeric cell by 10, across every frame.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0, 2.0, 3.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({4.0, 5.0, 6.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.map([](const pluss::table::SlateValue& v) {
  const auto x = v.to_dbl();
  return pluss::table::SlateValue(pluss::table::Slate(pluss::make_series({x * 10.0})));
});
std::cout << out.dloc(0);
```

### Output

```text
===========
     | id  
-----------
0    | 10  
1    | 20  
2    | 30  
-----------
[3 cs x 1 feat]
```

Example 2. `NULL` cells are skipped — `fn` is never called for them, and they stay `NULL`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0, 2.0, 3.0}), mask)},
});
auto atlas = pluss::table::Atlas({f0});

int calls = 0;
auto out = atlas.map([&calls](const pluss::table::SlateValue& v) {
  ++calls;
  const auto x = v.to_dbl();
  return pluss::table::SlateValue(pluss::table::Slate(pluss::make_series({x * 10.0})));
});
std::cout << "calls: " << calls << "\n";
std::cout << out.dloc(0);
```

### Output

```text
calls: 2
===========
     | id  
-----------
0    | 10  
1    | NULL
2    | 30  
-----------
[3 cs x 1 feat]
```

`fn` was called only twice for three rows — the masked `NULL` at index 1 was skipped
entirely and stays `NULL` in the output.

## Notes

- Applies to every feature in every frame — no feature selector of its own. Narrow with
  [`features()`](features.md) first if only some features need `fn`.
- Skips `NULL` cells without calling `fn` — see Example 2.
- Every non-null cell within one feature's column must map to the same output kind.
- `fn` must not be empty; throws `"Atlas::map: callback is empty"` otherwise.

## See Also

- [`Atlas::map_frames`](map_frames.md): transform whole frames at once, instead of one cell
  at a time.
- [`Atlas::features`](features.md): narrow to specific features before mapping.
- [`DataFrame::map`](../dataframe/map.md): the single-frame analog of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="loc.html">
    <span class="label">Previous</span>
    <span class="title">loc</span>
  </a>
  <a class="next-link" href="map_frames.html">
    <span class="label">Next</span>
    <span class="title">map_frames</span>
  </a>
</div>
