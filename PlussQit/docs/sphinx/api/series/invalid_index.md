<!-- pluss-hand-authored -->

# Series::invalid_index

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::invalid_index() const
```

## What It Does

`invalid_index()` compresses the same diagnosis as
[`invalid()`](invalid.md) into a 1D `int64` list of flat positions.

The positions are row-major flattened indices into `self`.

That matters because the result is no longer mask-shaped like the original
`Series`; it is compact. If only three lanes are invalid, the result contains
only three index values.

That compact form is useful in two different directions:

- as a getter input for methods like [`take()`](take.md), where the goal is to
  retrieve just the invalid values
- as a setter input for methods like [`put_()`](put_.md), where the goal is to
  replace exactly those invalid positions

## Parameters

None.

## Returns

`Series`
    A 1D `int64` `Series` containing the flat positions of every invalid lane.

## Gradient

`invalid_index()` is index discovery, not a differentiable numeric transform.

## Examples

Example 1. The returned indices are flattened positions, not 2D coordinate
pairs.

```cpp
auto values = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 2.0},
    {std::numeric_limits<double>::infinity(),
     -std::numeric_limits<double>::infinity(),
     5.0},
});

pluss::series::print(std::cout, values.invalid_index());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 3, 4]
```

In that `2 x 3` example, the flattened positions are:

- position `1` -> `(0, 1)` -> `nan`
- position `3` -> `(1, 0)` -> `+inf`
- position `4` -> `(1, 1)` -> `-inf`

Example 2. Those same indices can be used immediately as a getter. `take()`
reads flat positions from the original `Series` and returns only the invalid
lanes.

```cpp
auto values = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 2.0},
    {std::numeric_limits<double>::infinity(),
     -std::numeric_limits<double>::infinity(),
     5.0},
});

pluss::series::print(std::cout, values.take(values.invalid_index()));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, inf, -inf]
```

This is the compact getter pattern:

- `invalid()` tells you where the bad lanes are as a full mask
- `invalid_index()` compresses that diagnosis into flat positions
- `take(...)` uses those positions to fetch just the bad values

Example 3. For in-place constant repair, the cleanest surface is usually the
shape-preserving boolean mask from [`invalid()`](invalid.md).

```cpp
auto values = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 2.0},
    {std::numeric_limits<double>::infinity(),
     -std::numeric_limits<double>::infinity(),
     5.0},
});

values.where_(values.invalid(), -99.0);
pluss::series::print(std::cout, values);
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡   1.0000 -99.0000 2.0000 ⎤
⎣ -99.0000 -99.0000 5.0000 ⎦
```

Use that route when the replacement policy is one scalar value or one
broadcastable branch. The mask keeps the original shape, so the call reads very
directly: "where invalid, write `-99.0`."

Example 4. For positional replacement with one value per invalid lane,
`invalid_index()` pairs naturally with [`put_()`](put_.md).

```cpp
auto values = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 2.0},
    {std::numeric_limits<double>::infinity(),
     -std::numeric_limits<double>::infinity(),
     5.0},
});

values.put_(values.invalid_index(), pluss::make_series({70.0, 80.0, 90.0}));
std::cout << values;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 70, 2],
  [80, 90, 5]]
```

This is the array-valued setter pattern:

- `invalid_index()` returns exactly three positions here
- the replacement `Series` therefore must provide exactly three values unless a
  scalar broadcast is intended
- the write still lands back into the original `2 x 3` layout even though the
  index list itself is flat

There is not yet a dedicated callback-style "fill invalid lanes by function"
method on the public `Series` surface. The current general pattern is to build
the replacement `Series` first, using whatever deterministic rule is needed,
and then write it back with `put_()`.

Example 5. If nothing is invalid, the result is an empty 1D index list.

```cpp
pluss::series::print(std::cout, pluss::make_series({1.5, 2.5, 3.5}).invalid_index());
```

### Output

```text
Series(shape=[0], strides=[1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## Gotchas And Failure Modes

- These are flat row-major positions, not per-axis coordinate rows.
- The result is compact, so its size depends on how many invalid lanes were
  found, not on the shape of `self`.
- For shape-preserving replacement, the boolean-mask route via
  [`invalid()`](invalid.md) and [`where_()`](where_.md) is often simpler than a
  flat index write.
- For per-invalid replacement values, [`put_()`](put_.md) expects either one
  scalar source value or a source with one lane per returned invalid index.
- If you need a same-shape boolean answer for branching or masking, use
  [`invalid()`](invalid.md) instead.

## Summary

Use `invalid_index()` when the important result is the compact list of bad
positions, especially if the next step is "fetch those exact lanes" or
"overwrite those exact lanes."

## See Also

- [`Series::invalid`](invalid.md): same diagnosis, but shape-preserving.
- [`Series::nonzero`](nonzero.md): general truthy-position extractor.
- [`Series::take`](take.md): consume flat positions against flattened storage.
- [`Series::put_`](put_.md): overwrite flat positions in place.
- [`Series::where_`](where_.md): shape-preserving masked in-place replacement.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="irfft.html">
    <span class="label">Next</span>
    <span class="title">irfft</span>
  </a>
</div>
