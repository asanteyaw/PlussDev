<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 180182122e2f -->

# Atlas::squeeze

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::squeeze() const -> DataFrame
```

## What It Does

Collapses a depth-1 atlas back into its single `DataFrame` — the inverse of wrapping a
lone frame in `Atlas({frame})`. Requires the atlas to have exactly one frame; throws
otherwise.

```{important} Requires depth() == 1 — throws for any other depth
`squeeze()` only works on a single-frame atlas; it throws `"Atlas::squeeze requires depth
== 1"` for a depth of `0` or `2+`. Use [`dloc(i)`](dloc.md)/[`frame(i)`](frame.md) to pull
out one specific frame from a multi-frame atlas instead — `squeeze()` is specifically for
the "this atlas only ever had one frame" case.
```

## Parameters

`None`

## Returns

`DataFrame`
    The atlas's single frame.

## Examples

Example 1. Squeeze a depth-1 atlas back to a plain `DataFrame`.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0, 2.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
});
auto atlas = pluss::table::Atlas({f0});

std::cout << atlas.squeeze();
```

### Output

```text
==================
     | id    city 
------------------
0    | 1     Accra
1    | 2     Tema 
------------------
[2 cs x 2 feat]
```

Example 2. Calling `squeeze()` on a multi-frame atlas throws.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

try {
  atlas.squeeze();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas::squeeze requires depth == 1
```

## Notes

- Requires exactly one frame — see the important callout above.
- Depth labels (if any) are not carried over — the return type is a plain `DataFrame`,
  which has no depth-label concept.

## See Also

- [`Atlas::frame`](frame.md) / [`Atlas::dloc`](dloc.md): pull out one specific frame from a
  multi-frame atlas by index, instead of requiring exactly one.
- [`Atlas::aggregate_over_frames`](aggregate_over_frames.md): collapse a multi-frame atlas
  to one `DataFrame` via a reducer, instead of requiring the atlas already has depth `1`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort_depth.html">
    <span class="label">Previous</span>
    <span class="title">sort_depth</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
