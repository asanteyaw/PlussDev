<!-- pluss-hand-authored -->

# Series::size

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::size(int dim) const -> std::int64_t
Series::size(std::int64_t dim) const -> std::int64_t
```

## What It Does

Returns the length of one axis in the **current logical shape**.

That wording matters. `size()` is not reporting how the tensor was originally
created, and it is not reporting anything about memory order. It answers a much
more immediate question: "if I index this `Series` right now, how long is axis
`dim`?"

So if you transpose, unsqueeze, squeeze, slice, or reshape the tensor first,
`size()` reflects that new logical view.

The two overloads are only an ergonomics detail. The `int` form exists so plain
C++ literals like `0` and `-1` resolve naturally; both overloads normalize and
validate the dimension the same way and both return `std::int64_t`.

## Parameters

**dim** (`int` or `std::int64_t`)

The axis to inspect.

`size()` accepts both forward and reverse indexing:

- `0` means the first axis
- `1` means the second axis
- `-1` means the last axis
- `-2` means the second-to-last axis

Negative dimensions are normalized against the current rank first, then checked.
If the normalized dimension still falls outside `[0, rank)`, the call throws.

## Returns

`std::int64_t`
    The extent of the requested axis.

## Examples

Example 1. Query the leading axis of a rank-3 `Series`.

```cpp
auto cube = pluss::make_series(
    std::vector<double>{
        1,  2,  3,  4,
        5,  6,  7,  8,
        9,  10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24},
    {2, 3, 4});

std::cout << cube.size(0);
```

### Output

```text
2
```

Example 2. Negative dimensions count from the back, so `-1` means the trailing
axis.

```cpp
std::cout << cube.size(-1);
```

### Output

```text
4
```

Example 3. `size()` follows the current logical view, not the original factory
shape.

```cpp
std::cout << cube.transpose(0, 2).size(1);
```

### Output

```text
3
```

The original shape was `[2, 3, 4]`. After `transpose(0, 2)`, the logical shape
becomes `[4, 3, 2]`, so axis `1` is now `3`.

## Notes

- `size()` answers one axis at a time. Use [`sizes()`](sizes.md) when you need
  the full shape in one call.
- The answer is about logical shape, not about contiguity or strides.
- Scalars have rank `0`, so they have no valid `dim` to query.

## See Also

- [`Series::sizes`](sizes.md): inspect the entire shape at once.
- [`Series::view`](view.md), [`Series::reshape`](reshape.md): methods that
  change what later `size()` calls return.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pos.html">
    <span class="label">Previous</span>
    <span class="title">pos</span>
  </a>
  <a class="next-link" href="sizes.html">
    <span class="label">Next</span>
    <span class="title">sizes</span>
  </a>
</div>
