<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c74e0e3d64c5 -->

# Atlas::tail

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::tail(std::int64_t n = 5) const -> Atlas
```

## What It Does

Returns a new atlas containing the last `n` frames (depth-wise), defaulting to 5 when
omitted — the mirror of [`head()`](head.md), and the `Atlas` analog of
[`Archive::tail`](../archive/tail.md). Same clamping behavior as `head()`: requesting more
than the atlas's depth clamps to the whole atlas, and `n <= 0` returns an empty atlas,
rather than throwing.

## Parameters

**n** (`std::int64_t`, defaults to `5`)
    The number of trailing frames to keep.

## Returns

`Atlas`
    A new atlas containing at most the last `n` frames.

## Examples

Example 1. Take the last two frames of a multi-frame atlas.

```cpp
std::vector<pluss::table::DataFrame> frames;
for (int i = 0; i < 7; ++i) {
  frames.push_back(pluss::table::DataFrame({
      {"id", pluss::table::Thread(pluss::make_series({double(i)}))},
  }));
}
auto atlas = pluss::table::Atlas(frames);

auto out = atlas.tail(2);
std::cout << "depth: " << out.depth() << "\n";
std::cout << out.dloc(0);
```

### Output

```text
depth: 2
===========
     | id  
-----------
0    | 5   
-----------
[1 cs x 1 feat]
```

The frame returned re-indexes from depth `0`, same as every other depth-selecting method —
`dloc(0)` here is the atlas's original depth-index `5`, not `0`.

## Notes

- Defaults to `5` when `n` is omitted.
- Clamps at both ends rather than throwing, same as [`head()`](head.md).
- Returns a new `Atlas`; the receiver is unmodified.

## See Also

- [`Atlas::head`](head.md): the mirror operation, taking the first `n` frames.
- [`Atlas::slice_frames`](slice_frames.md): an arbitrary `[start, stop)` depth range instead
  of a fixed count from the back.
- [`Archive::tail`](../archive/tail.md): the depth-stacked-catalog analog of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
