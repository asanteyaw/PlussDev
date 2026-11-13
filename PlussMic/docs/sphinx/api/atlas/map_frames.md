<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dd38b27b6972 -->

# Atlas::map_frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas Atlas::map_frames(const std::function<DataFrame(const DataFrame&)>& fn) const
```

## What It Does

Applies `fn` to every frame in the atlas independently and returns a new atlas built from
the results — depth-preserving (one output frame per input frame, same order) and
depth-label-preserving (any [`label()`](label.md) set on the receiver is carried over
unchanged onto the result). The `Atlas` analog of
[`Archive::map_catalogs`](../archive/map_catalogs.md).

```{important} The result must still be a strict cube — every frame the same row count
`Atlas` requires every frame to have the same row count (a "strict cube"), enforced at
construction. If `fn` returns frames with different row counts across the atlas, building
the result atlas throws `"Atlas: all frames must have identical row count (strict cube)"`.
```

## Parameters

**fn** (`const std::function<DataFrame(const DataFrame&)>&`)
    A callback receiving one frame and returning its replacement. Must not be empty, and
    must return frames with a uniform row count across the whole atlas — see the important
    callout above.

## Returns

`Atlas`
    A new atlas with every frame replaced by `fn`'s result, same depth and depth labels as
    the receiver.

## Examples

Example 1. Apply the same per-frame transform (here, `head(1)`) across the whole atlas.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0, 2.0, 3.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({4.0, 5.0, 6.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"jan", "feb"})));

auto out = atlas.map_frames([](const pluss::table::DataFrame& df) { return df.head(1); });
std::cout << "out depth: " << out.depth() << "\n";
std::cout << "depth labels preserved: " << out.label().has() << "\n";
std::cout << out.dloc(0);
```

### Output

```text
out depth: 2
depth labels preserved: 1
===========
     | id  
-----------
0    | 1   
-----------
[1 cs x 1 feat]
```

Example 2. A callback that produces different row counts per frame throws when the
resulting atlas is built — see the important callout above.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0, 2.0, 3.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({4.0, 5.0, 6.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

int calls = 0;
try {
  atlas.map_frames([&calls](const pluss::table::DataFrame& df) {
    ++calls;
    return calls == 1 ? df.head(1) : df;
  });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas: all frames must have identical row count (strict cube)
```

## Notes

- Depth-preserving and depth-label-preserving.
- `fn` must not be empty; throws `"Atlas::map_frames: mapper function is empty"` for an
  empty callback.
- Requires the result to remain a strict cube — see the important callout above.

## See Also

- [`Atlas::aggregate_over_frames`](aggregate_over_frames.md): reduce every frame down to a
  single `DataFrame`, instead of transforming each one independently.
- [`Atlas::flow`](flow.md): transform the whole `Atlas` in one call, instead of per-frame.
- [`Archive::map_catalogs`](../archive/map_catalogs.md): the depth-stacked-catalog analog
  of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="ncases.html">
    <span class="label">Next</span>
    <span class="title">ncases</span>
  </a>
</div>
