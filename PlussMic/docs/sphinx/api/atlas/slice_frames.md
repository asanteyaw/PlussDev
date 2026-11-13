<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fda8af3f9606 -->

# Atlas::slice_frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas Atlas::slice_frames(std::int64_t start, std::int64_t stop) const
```

## What It Does

Returns a new atlas containing the contiguous half-open depth range `[start, stop)` — the
`Atlas` analog of [`Archive::slice_depth`](../archive/slice_depth.md). Supports
Python-style negative indices for both `start` and `stop`, and clamps out-of-range bounds
rather than throwing.

## Parameters

**start** (`std::int64_t`)
    Inclusive starting depth-index. Negative values count from the end.

**stop** (`std::int64_t`)
    Exclusive ending depth-index. Negative values count from the end.

## Returns

`Atlas`
    A new atlas containing frames `[start, stop)`.

## Examples

Example 1. Slice a middle range of frames.

```cpp
std::vector<pluss::table::DataFrame> frames;
for (int i = 0; i < 5; ++i) {
  frames.push_back(pluss::table::DataFrame({
      {"id", pluss::table::Slate(pluss::make_series({double(i)}))},
  }));
}
auto atlas = pluss::table::Atlas(frames);

auto out = atlas.slice_frames(1, 4);
std::cout << "depth: " << out.depth() << "\n";
std::cout << out.dloc(0);
```

### Output

```text
depth: 3
===========
     | id  
-----------
0    | 1   
-----------
[1 cs x 1 feat]
```

Example 2. Negative `start`/`stop` count from the end, same convention as
[`dloc()`](dloc.md)'s negative indexing.

```cpp
std::vector<pluss::table::DataFrame> frames;
for (int i = 0; i < 5; ++i) {
  frames.push_back(pluss::table::DataFrame({
      {"id", pluss::table::Slate(pluss::make_series({double(i)}))},
  }));
}
auto atlas = pluss::table::Atlas(frames);

auto out = atlas.slice_frames(-3, -1);
std::cout << "depth: " << out.depth() << "\n";
std::cout << out.dloc(0);
```

### Output

```text
depth: 2
===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

`slice_frames(-3, -1)` on a 5-frame atlas is equivalent to `slice_frames(2, 4)` — depths
`2` and `3`.

## Notes

- `stop` is exclusive.
- Both `start` and `stop` support negative indexing, and both clamp into range rather than
  throwing on an out-of-bounds value.
- Returns a new `Atlas`; the receiver is unmodified.

## See Also

- [`Atlas::head`](head.md) / [`Atlas::tail`](tail.md): fixed-count selections from either
  end, instead of an arbitrary range.
- [`Atlas::dloc`](dloc.md): the single-frame equivalent with the same negative-indexing
  convention.
- [`Archive::slice_depth`](../archive/slice_depth.md): the depth-stacked-catalog analog of
  this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="sort_depth.html">
    <span class="label">Next</span>
    <span class="title">sort_depth</span>
  </a>
</div>
