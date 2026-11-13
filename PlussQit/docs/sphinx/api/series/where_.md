<!-- pluss-hand-authored -->

# Series::where_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::where_(const Series& condition, const Series& other)

Series& Series::where_(const Series& condition, double other)
```

## What It Does

`where_()` is the in-place sibling of [`where()`](where.md).

It uses a condition mask to decide which positions in `self` should be replaced
and which should be left alone. The public reading for the in-place form is the
same as the common numeric-self reading of `where()`:

- where `condition` is true, write `other`
- where `condition` is false, keep the existing value already stored in `self`

Because this is an in-place method, the important difference is not the branch
math but the mutation behavior:

- the returned reference is still `self`
- storage version and alias-visible contents can change
- downstream code sees the updated values immediately

## Overload Map

**`where_(condition, other_series)`**

Replace selected positions from another `Series`.

**`where_(condition, scalar_other)`**

Replace selected positions from one scalar value that broadcasts as needed.

## Parameters

**condition** (`const Series&`)

The selector mask.

Verified behavior:

- boolean masks work directly
- numeric masks are interpreted by truthiness
- the mask may broadcast to the shape of `self`

**other** (`const Series&` or `double`)

The replacement payload for the `true` positions.

If it is a scalar, that one value is used for every selected position. If it is
a `Series`, it follows the usual broadcast and dtype-promotion rules.

## Returns

`Series&`
    `self`, after the selected positions have been updated in place.

## Examples

Example 1. Series-to-series replacement updates only the `true` positions.

```cpp
auto values = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto cond = pluss::make_series({false, true, false, true}, pluss::ScalarType::Bool);
auto other = pluss::make_series({100.0, 200.0, 300.0, 400.0});

values.where_(cond, other);
pluss::series::print(std::cout, values);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 200.0000, 30.0000, 400.0000]
```

Example 2. A broadcasted mask and scalar replacement let one expression update
many aligned positions.

```cpp
auto matrix = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
auto col_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);

matrix.where_(col_mask, -1.0);
std::cout << matrix;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 2, -1],
  [-1, 5, -1]]
```

## Gotchas And Failure Modes

- `where_()` mutates the original `Series`.
- The mask may broadcast, so a small condition can rewrite many positions.
- Numeric mask values are interpreted by truthiness on the current surface.

## Summary

Use `where_()` when the current `Series` should be rewritten in place under a
mask instead of producing a separate result.

## See Also

- [`Series::where`](where.md): out-of-place branch selection.
- [`Series::masked_scatter_`](masked_scatter_.md): sequential source writes at
  `true` positions.
- [`Series::logical_not_`](logical_not_.md): another in-place boolean-driven
  transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="where.html">
    <span class="label">Previous</span>
    <span class="title">where</span>
  </a>
  <a class="next-link" href="xlogy.html">
    <span class="label">Next</span>
    <span class="title">xlogy</span>
  </a>
</div>
