<!-- pluss-hand-authored -->

# Series::size

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::int64_t Series::size(int dim) const
std::int64_t Series::size(std::int64_t dim) const
```

## What It Does

Returns the length of one axis in the current logical shape.

This is a small method, but it sits on an important distinction: `size()` is
about how the `Series` is being interpreted now, not about how it was first
constructed and not about how values happen to be laid out in storage.

If you transpose, permute, squeeze, unsqueeze, or reshape first, `size()`
reports the post-transform shape immediately. That makes it the right method
for branch logic, guard checks, and shape-driven code paths that operate on the
current view rather than the source object’s history.

The `int` and `std::int64_t` overloads behave the same. The `int` form simply
lets literals such as `0` and `-1` resolve without friction.

## Parameters

**dim** (`int` or `std::int64_t`)

The axis to inspect.

`size()` accepts both forward and reverse indexing:

- `0` means the first axis
- `1` means the second axis
- `-1` means the last axis
- `-2` means the second-to-last axis

Negative dimensions are normalized against the current rank before validation.
If the normalized dimension is still outside `[0, dim())`, the call throws.

## Returns

`std::int64_t`
    The extent of the requested axis.

## Examples

The examples on this page build incrementally.

Example 1. Read the leading axis from a rank-3 `Series`.

```cpp
auto cube = pluss::make_series(
    {
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

Example 3. `size()` follows the current logical view after a layout transform.

```cpp
std::cout << cube.transpose(0, 2).size(1);
```

### Output

```text
3
```

The original shape is `[2, 3, 4]`. After `transpose(0, 2)`, the logical shape
becomes `[4, 3, 2]`, so axis `1` is `3`.

Example 4. Scalars have rank `0`, so there is no valid dimension to query.

```cpp
auto scalar = pluss::make_series(42.0);

try {
  std::cout << scalar.size(0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
SeriesBase::size: dim out of range
```

## Notes

- `size()` answers one axis at a time. Use [`sizes()`](sizes.md) when you need
  the full shape in one call.
- The answer is about logical shape, not about contiguity or strides.
- Negative dimensions are supported.

## See Also

- [`Series::sizes`](sizes.md): inspect the entire shape at once.
- [`Series::view`](view.md), [`Series::reshape`](reshape.md): methods that
  change what later `size()` calls return.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sinh.html">
    <span class="label">Previous</span>
    <span class="title">sinh</span>
  </a>
  <a class="next-link" href="sizes.html">
    <span class="label">Next</span>
    <span class="title">sizes</span>
  </a>
</div>
