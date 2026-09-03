<!-- pluss-hand-authored -->

# Series::masked_scatter

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::masked_scatter(const Series& mask, const Series& src) const -> Series
```

## What It Does

`masked_scatter()` writes source values into a copy of `self` wherever `mask`
is `true`.

The write order matters. Conceptually, the method walks the destination in its
logical flat order, finds the positions where the mask is `true`, and consumes
`src` in that same order.

That is why this method is not the same thing as [`where()`](where.md):

- `where()` chooses between two full value sources position by position
- `masked_scatter()` consumes a sequential source list only at `true` mask
  positions

It is also not the same as [`scatter()`](scatter.md):

- `scatter()` uses explicit integer destinations along one axis
- `masked_scatter()` uses a boolean mask over the destination itself

## Parameters

**mask** (`const Series&`)

The boolean write mask.

Load-bearing rules:

- `mask` must be `bool`
- `mask` may broadcast to the destination shape
- the destination must already have the final broadcasted shape

That last rule matters. This method does not expand `self` into a bigger shape
for you. It only expands the mask to match the existing destination geometry.

**src** (`const Series&`)

The source values to write into the `true` positions.

The cardinality rule is strict:

- `src.numel()` must equal the number of `true` entries in the broadcasted
  mask

This method is sequential, not value-broadcasting. Each `true` slot consumes
exactly one source value.

## Returns

`Series`
    A new `Series` with source values written into the masked positions.

## Examples

Example 1. The simplest case replaces only the `true` positions.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto mask = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
auto src = pluss::make_series({1.0, 2.0});

std::cout << base.masked_scatter(mask, src);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 20, 2, 40]
```

The source is consumed in logical order:

- first `true` position gets `1`
- second `true` position gets `2`

Example 2. The mask may broadcast to the destination shape.

```cpp
auto matrix = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
});
auto column_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto src = pluss::make_series({1.0, 2.0, 3.0, 4.0});

std::cout << matrix.masked_scatter(column_mask, src);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 20, 2],
  [3, 50, 4]]
```

The 1-D mask broadcasts across both rows, so the `true` positions appear in
this flat logical order:

```text
(0,0), (0,2), (1,0), (1,2)
```

Those four positions consume the four source values in that order.

Example 3. The source count must match the number of `true` positions exactly.

```cpp
try {
  std::cout << base.masked_scatter(mask, pluss::make_series({9.0}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
masked_scatter_: src numel must match number of True in mask
```

Example 4. The mask must really be boolean.

```cpp
try {
  auto bad_mask = pluss::make_series({1LL, 0LL, 1LL, 0LL});
  std::cout << base.masked_scatter(bad_mask, src);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
masked_scatter_: mask must be bool
```

## Gotchas And Failure Modes

- `mask` is boolean, not integer-index based.
- `src` is consumed sequentially; it does not broadcast across `true`
  positions.
- The destination must already have the final broadcasted shape.
- The source count must match the number of `true` mask positions exactly.

## Summary

Use `masked_scatter()` when you want boolean-mask-based writes into a copied
result, consuming source values in mask order.

## See Also

- [`Series::masked_scatter_`](masked_scatter_.md): the in-place sibling.
- [`Series::where`](where.md): full-shape conditional selection rather than
  sequential masked writes.
- [`Series::scatter`](scatter.md): explicit integer-index scattering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="masked_scatter_.html">
    <span class="label">Next</span>
    <span class="title">masked_scatter_</span>
  </a>
</div>
