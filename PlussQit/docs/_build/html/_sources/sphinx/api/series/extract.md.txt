<!-- pluss-hand-authored -->

# Series::extract

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::extract(pluss::RangeView<ExtractSpec> specs) const -> Series
Series::extract(ExtractSpec a0) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2, ExtractSpec a3) const -> Series
```

## What It Does

`extract()` evaluates a heterogeneous selector list against a `Series`.

This is the broad, dynamic indexing surface. Use it when the selector itself is
part of your program state instead of being hard-coded into a simpler helper
such as [`pos()`](pos.md), [`slice()`](slice.md), or [`block()`](block.md).

`extract()` understands the full `ExtractSpec` family:

- scalar indices
- slice selectors such as `SliceAll()`, `SliceFrom()`, `SliceTo()`, and
  `SliceRange()`
- `Ellipsis()`
- `NewAxis()`
- advanced bool and integer index `Series`

The most important mental model is this:

- `extract()` works axis by axis
- when a selector can remain a cheap view, it does
- when a selector cannot remain a view, `extract()` materializes the needed
  position list and continues

That is why `extract()` can express things that [`traverse_slice()`](traverse_slice.md)
deliberately rejects, such as reverse slices and advanced index tensors.

## Overload Map

**`extract(a0)` through `extract(a0, a1, a2, a3)`**

Convenience overloads for small fixed selector lists.

**`extract(specs)`**

The general form for dynamic selector lists and lists longer than four entries.

The overloads differ only in how the selector sequence is supplied. The
selection semantics are the same.

## Parameters

**specs / a0..a3** (`ExtractSpec`)

The selector sequence.

Each selector kind changes rank traversal differently:

- scalar index: consumes one existing axis and removes it from the result
- slice selector: consumes one existing axis and keeps it, possibly with a
  different length
- `NewAxis()`: inserts a new size-1 axis and consumes no source axis
- `Ellipsis()`: expands to however many trailing identity slices are needed
- bool advanced index: consumes one axis and keeps only the `true` positions
- integer advanced index: consumes one axis and selects explicit positions

Normalization rules that matter in real use:

- omitted trailing axes are filled with `SliceAll()`
- at most one `Ellipsis()` is allowed
- negative scalar indices count from the end of the indexed axis
- slice steps may be positive or negative, but may not be `0`
- bool advanced indices must be 1D and match the indexed axis length
- integer advanced indices are normalized to `int64`, and negative values count
  from the end

## Returns

`Series`
    The selected result.

    Depending on the selector path, this may be:

    - a view
    - a partially materialized result
    - a fully materialized result

## Gradient

`extract()` is differentiable when the selected value path is differentiable.

Backward follows the normalized selection path:

- positions not selected receive zero sensitivity
- positions selected once receive one contribution
- positions selected multiple times accumulate contributions

That accumulation behavior matters most for advanced integer indexing, where the
same source location may be read repeatedly.

## Examples

The examples below start with ordinary slice-like use, then move into rank
changes, reverse traversal, advanced selectors, and the important difference
between `extract()` and [`index()`](index.md).

Example 1. A single selector is enough when only one axis is being narrowed.

```cpp
auto grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});

std::cout << grid.extract(pluss::Slicing::SliceRange(0, 2, 1));
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [20, 21, 22, 23]]
```

Because only the first axis is mentioned, `extract()` implicitly appends
identity slices for the remaining axes.

Example 2. Two selectors give ordinary row/column slicing.

```cpp
std::cout << grid.extract(
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(2, 4, 1));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[12, 13],
  [22, 23],
  [32, 33]]
```

Example 3. `NewAxis()` inserts a size-1 dimension, while a scalar selector
still consumes the axis it indexes.

```cpp
std::cout << grid.extract(
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{1});
```

### Output

```text
pluss::Series(shape=[1,3], strides=[12,4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[11, 21, 31]]
```

The inserted axis survives in the result. The scalar column index does not.

Example 4. Mixed rank-changing and rank-preserving selectors are often easiest
to read in the fixed-arity overloads.

```cpp
auto cube = pluss::make_series({
    {
        {1.0, 2.0, 3.0, 4.0},
        {5.0, 6.0, 7.0, 8.0},
        {9.0, 10.0, 11.0, 12.0},
    },
    {
        {13.0, 14.0, 15.0, 16.0},
        {17.0, 18.0, 19.0, 20.0},
        {21.0, 22.0, 23.0, 24.0},
    },
});

std::cout << cube.extract(
    std::int64_t{1},
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(0, 4, 2),
    pluss::Slicing::NewAxis());
```

### Output

```text
pluss::Series(shape=[3,2,1], strides=[4,2,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[[13],
  [15]],
  [[17],
  [19]],
  [[21],
  [23]]]
```

Example 5. `Ellipsis()` expands to the needed identity slices.

```cpp
std::cout << grid.extract(
    pluss::Slicing::Ellipsis(),
    std::int64_t{2});
```

### Output

```text
pluss::Series(shape=[3], strides=[4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[12, 22, 32]
```

On this rank-2 input, that means "keep all earlier axes, then take column `2`."

Example 6. Reverse slicing is legal on `extract()`.

```cpp
std::cout << grid.extract({
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)});
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[13, 12, 11, 10],
  [23, 22, 21, 20],
  [33, 32, 31, 30]]
```

This is one of the places where `extract()` intentionally goes beyond
[`traverse_slice()`](traverse_slice.md).

Example 7. A bool advanced index acts as a mask over the consumed axis.

```cpp
auto row_mask = pluss::make_series({true, false, true});

std::cout << grid.extract({
    row_mask,
    pluss::Slicing::SliceAll()});
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [30, 31, 32, 33]]
```

Example 8. A shaped integer advanced index is flattened by `extract()`.

```cpp
auto row_pick_2d = pluss::make_series({
    {2LL, 0LL},
    {2LL, 1LL},
});

std::cout << grid.extract({
    row_pick_2d,
    pluss::Slicing::SliceAll()});
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[[30, 31, 32, 33],
  [10, 11, 12, 13],
  [30, 31, 32, 33],
  [20, 21, 22, 23]]
```

This is the key behavior to remember: `extract()` consumes one axis at a time,
so the shaped integer tensor becomes a flat list of positions on that axis.

Example 9. Zero-step slices are invalid.

```cpp
try {
  std::cout << grid.extract({
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(0, 4, 0)}) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Series::extract: step cannot be 0
```

## Gotchas And Failure Modes

- `extract()` is dynamic and broad, but not vague. Each selector kind has a
  precise rank effect.
- `NewAxis()` adds rank. Scalar indices remove rank.
- `Ellipsis()` is normalized into ordinary identity slices.
- Reverse slices are legal here even though they are rejected by
  [`traverse_slice()`](traverse_slice.md).
- Shaped integer advanced indices are flattened one axis at a time on this
  surface.
- Zero-step slices fail explicitly.

## Teaching Summary

Use `extract()` when your indexing problem is heterogeneous, dynamic, or rich
enough that the smaller helpers stop being expressive.

- Use it for mixed selectors.
- Use it for reverse slices.
- Use it when advanced indices are part of a general selector list.
- Prefer [`index()`](index.md) when advanced index tensor shape preservation is
  the main story.

The single most important habit is to ask whether your selector should be read
axis by axis. If yes, `extract()` is usually the right surface.

## See Also

- [`Series::index`](index.md): advanced-index-preserving routing.
- [`Series::traverse_slice`](traverse_slice.md): the stricter view-only sibling.
- [`Series::extract_put`](extract_put.md): copy-returning write-side
  counterpart.
- [`Series::extract_put_`](extract_put_.md): in-place write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="block.html">
    <span class="label">Previous</span>
    <span class="title">block</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
