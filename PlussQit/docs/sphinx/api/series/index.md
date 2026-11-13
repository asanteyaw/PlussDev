<!-- pluss-hand-authored -->

# Series::index

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::index(pluss::RangeView<ExtractSpec> specs) const
```

## What It Does

`index()` evaluates a selector list with advanced indexing as the primary
intent.

That sounds close to [`extract()`](extract.md), and much of the selector
vocabulary is shared. The difference is routing. `index()` tries to preserve
advanced-index tensor semantics whenever the selector list has the right shape,
and only falls back to [`extract()`](extract.md) when staying on that route
would change the meaning.

In practice, `index()` has three important behavioral zones:

1. all selectors are advanced index tensors
2. advanced index tensors are mixed only with identity `SliceAll()` selectors
3. anything else

Zones 1 and 2 stay on the dedicated advanced-index path. Zone 3 delegates to
`extract()`.

That boundary matters because the advanced-index path preserves advanced index
tensor shape and zips multiple advanced tensors together. The `extract()`
fallback, by contrast, processes one axis at a time.

## Parameters

**specs** (`pluss::RangeView<ExtractSpec>`)

The selector sequence.

The accepted selector vocabulary is the same `ExtractSpec` family used by
[`extract()`](extract.md), but not every selector mix stays on the same routing
path.

Advanced-index route:

- one or more advanced index `Series`
- optionally separated by identity `SliceAll()` selectors only
- omitted trailing axes are treated as `SliceAll()`

Fallback-to-`extract()` route:

- scalar indices
- non-identity slices such as `SliceRange(1, 4, 2)`
- `NewAxis()`
- any heterogeneous pattern that is not "advanced indices plus identity slices
  only"

Advanced index rules that matter in practice:

- bool tensors act as masks
- integer tensors act as explicit positions
- negative integer values count from the end
- index tensors must live on the same platform as the source

## Returns

`Series`
    The indexed result.

    On the advanced route, result shape reflects advanced index tensor shape.
    On the fallback route, result shape follows [`extract()`](extract.md)
    semantics instead.

## Gradient

`index()` is differentiable when the selected value path is differentiable.

Backward follows the effective routing path:

- on the advanced route, repeated positions accumulate sensitivity
- on the fallback route, backward behaves like the equivalent `extract()`
  selection

So repeated advanced selections are not just legal; they also matter for how
gradient is accumulated.

## Examples

These examples are ordered to show the dedicated advanced route first, then the
exact boundary where `index()` stops preserving advanced-index tensor semantics
and falls back to `extract()`.

Example 1. A single integer advanced index tensor selects rows and preserves the
index tensor as the front shape of the result.

```cpp
auto grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});
auto row_pick = pluss::make_series({2LL, 0LL, 2LL});

std::cout << grid.index({row_pick});
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[30, 31, 32, 33],
  [10, 11, 12, 13],
  [30, 31, 32, 33]]
```

Example 2. Multiple advanced index tensors are zipped together when the only
other selector is identity `SliceAll()`.

```cpp
auto row_idx = pluss::make_series({0LL, 1LL, 2LL, 1LL});
auto col_idx = pluss::make_series({2LL, 3LL, 0LL, 2LL});
auto adv_cube = pluss::make_series({
    {
        {0.0, 1.0},
        {2.0, 3.0},
        {4.0, 5.0},
        {6.0, 7.0},
    },
    {
        {8.0, 9.0},
        {10.0, 11.0},
        {12.0, 13.0},
        {14.0, 15.0},
    },
    {
        {16.0, 17.0},
        {18.0, 19.0},
        {20.0, 21.0},
        {22.0, 23.0},
    },
});

std::cout << adv_cube.index({
    row_idx,
    col_idx,
    pluss::Slicing::SliceAll()});
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[4, 5],
  [14, 15],
  [16, 17],
  [12, 13]]
```

Each `(row_idx[n], col_idx[n])` pair chooses one lane from the first two axes.
The trailing identity slice then keeps the full length-2 tail for each pair.

Example 3. A bool mask also rides the advanced-index route.

```cpp
auto row_mask = pluss::make_series({true, false, true});

std::cout << grid.index({
    row_mask,
    pluss::Slicing::SliceAll()});
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [30, 31, 32, 33]]
```

Example 4. A shaped integer index tensor preserves its own shape on the
advanced route.

```cpp
auto row_pick_2d = pluss::make_series({
    {2LL, 0LL},
    {2LL, 1LL},
});

std::cout << grid.index({row_pick_2d});
```

### Output

```text
pluss::Series(shape=[2,2,4], strides=[8,4,1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[[[30, 31, 32, 33],
  [10, 11, 12, 13]],
  [[30, 31, 32, 33],
  [20, 21, 22, 23]]]
```

This is the clearest contrast with [`extract()`](extract.md), where the same
shaped integer selector becomes a flat position list on one axis.

Example 5. Negative integer indices count from the end on the advanced route.

```cpp
auto negative_row_pick = pluss::make_series({-1LL, 0LL});

std::cout << grid.index({negative_row_pick});
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[30, 31, 32, 33],
  [10, 11, 12, 13]]
```

Example 6. Once the selector is ordinary slicing rather than advanced
indexing, `index()` effectively becomes a pass-through to `extract()`.

```cpp
std::cout << grid.index({
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(1, 4, 2)});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]
```

Example 7. A non-identity slice inside a mixed advanced expression is the
precise boundary where `index()` falls back to `extract()`.

```cpp
std::cout << adv_cube.index({
    row_idx,
    pluss::Slicing::SliceRange(1, 4, 2),
    pluss::Slicing::SliceAll()});
```

### Output

```text
pluss::Series(shape=[4,2,2], strides=[8,4,1], dtype=float64, platform=cpu, numel=16, contiguous=false)
data=[[[2, 3],
  [6, 7]],
  [[10, 11],
  [14, 15]],
  [[18, 19],
  [22, 23]],
  [[10, 11],
  [14, 15]]]
```

This is not the same semantics as Example 2. The second selector is no longer
an identity slice, so `index()` stops doing advanced-index zipping and
delegates to `extract()`.

## Gotchas And Failure Modes

- `index()` and [`extract()`](extract.md) share selector syntax but not always
  the same semantics.
- Advanced-index tensor shape is preserved only while the selector stays on the
  dedicated advanced route.
- Identity `SliceAll()` selectors are special here; non-identity slices change
  the routing.
- Negative integer indices are legal and count from the end.

## Summary

Use `index()` when advanced index tensors are the main story.

- Use it for shaped integer or bool index tensors whose shape should remain part
  of the result.
- Use it when multiple advanced tensors should be zipped together.
- Expect fallback to [`extract()`](extract.md) as soon as the selector stops
  being "advanced indices plus identity slices only."

The key habit is to ask what should happen to advanced index tensor shape. If
the answer is "preserve it," start with `index()`.

## See Also

- [`Series::extract`](extract.md): the broader axis-by-axis dynamic selector
  surface.
- [`Series::take`](take.md): flattened linear indexing rather than axis-aware
  advanced indexing.
- [`Series::index_put`](index_put.md): copy-returning write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="imag.html">
    <span class="label">Previous</span>
    <span class="title">imag</span>
  </a>
  <a class="next-link" href="index_put.html">
    <span class="label">Next</span>
    <span class="title">index_put</span>
  </a>
</div>
