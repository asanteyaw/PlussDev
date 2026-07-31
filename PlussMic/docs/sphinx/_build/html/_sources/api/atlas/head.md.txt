<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 82efc8c9b898 -->

# Atlas::head

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::head(std::int64_t n = 5) const -> Atlas
```

## What It Does

Returns a new atlas containing the first `n` frames (depth-wise), defaulting to 5 when
omitted — the `Atlas` analog of [`Archive::head`](../archive/head.md). Requesting more
than the atlas's depth clamps to the whole atlas rather than throwing; `n <= 0` returns an
empty (zero-depth) atlas rather than throwing.

## Parameters

**n** (`std::int64_t`, defaults to `5`)
    The number of leading frames to keep.

## Returns

`Atlas`
    A new atlas containing at most the first `n` frames.

## Examples

Example 1. Take the first frame of a multi-frame atlas.

```cpp
std::vector<pluss::table::DataFrame> frames;
for (int i = 0; i < 7; ++i) {
  frames.push_back(pluss::table::DataFrame({
      {"id", pluss::table::Thread(pluss::make_series({double(i)}))},
  }));
}
auto atlas = pluss::table::Atlas(frames);

auto out = atlas.head(3);
std::cout << "depth: " << out.depth() << "\n";
std::cout << out.dloc(0);
```

### Output

```text
depth: 3
===========
     | id  
-----------
0    | 0   
-----------
[1 cs x 1 feat]
```

Example 2. The default (`n = 5`) applies when omitted; out-of-range values clamp rather
than throw.

```cpp
std::vector<pluss::table::DataFrame> frames;
for (int i = 0; i < 7; ++i) {
  frames.push_back(pluss::table::DataFrame({
      {"id", pluss::table::Thread(pluss::make_series({double(i)}))},
  }));
}
auto atlas = pluss::table::Atlas(frames);

std::cout << "head() depth: " << atlas.head().depth() << "\n";
std::cout << "head(100) depth: " << atlas.head(100).depth() << "\n";
std::cout << "head(0) depth: " << atlas.head(0).depth();
```

### Output

```text
head() depth: 5
head(100) depth: 7
head(0) depth: 0
```

## Notes

- Defaults to `5` when `n` is omitted.
- Clamps at both ends rather than throwing — see Example 2.
- Returns a new `Atlas`; the receiver is unmodified.

## See Also

- [`Atlas::tail`](tail.md): the mirror operation, taking the last `n` frames.
- [`Atlas::slice_frames`](slice_frames.md): an arbitrary `[start, stop)` depth range instead
  of a fixed count from the front.
- [`Archive::head`](../archive/head.md): the depth-stacked-catalog analog of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frames.html">
    <span class="label">Previous</span>
    <span class="title">frames</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
