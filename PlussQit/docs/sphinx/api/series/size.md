# Series::size

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::size(int dim) const -> std::int64_t
Series::size(std::int64_t dim) const -> std::int64_t
```

## What It Does

Returns the extent of one axis in the current logical view.

That last phrase matters. `size()` does not report some original factory shape;
it reports the shape of the `Series` exactly as it exists at the moment you ask.
If you transpose, squeeze, unsqueeze, slice, or otherwise reshape a series, the
answer changes to match that new logical layout.

The `int` overload is only a convenience forwarder for ordinary C++ literals.
Both overloads have the same runtime behavior.

## Parameters

**dim** (`int` or `std::int64_t`)

The axis to query.

`size()` accepts both positive and negative dimensions:

- `0` means the first axis
- `1` means the second axis
- `-1` means the last axis
- `-2` means the second-to-last axis

After negative-dimension normalization, the dimension is bounds-checked. If it
still falls outside the rank, the call throws `"SeriesBase::size: dim out of range"`.

## Returns

`std::int64_t`
    The length of the requested axis.

## Examples

Example 1. Read the leading axis directly.

```cpp
auto shape_src = pluss::make_series(
    std::vector<double>{
        1, 2, 3, 4, 5, 6,
        7, 8, 9, 10, 11, 12,
        13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24},
    {2, 3, 4});

std::cout << shape_src.size(0);
```

### Output

```text
2
```

Example 2. Negative dimensions read from the back, so `-1` means the trailing
axis.

```cpp
std::cout << shape_src.size(-1);
```

### Output

```text
4
```

## Notes

- `size()` returns one axis only. If you want the whole shape at once, use
  [`sizes()`](sizes.md).
- The result is about the current logical view, not about storage order or
  contiguity.
- The return type is always `std::int64_t`, even when you call the `int`
  overload.

## See Also

- [`Series::sizes`](sizes.md): borrow the entire shape view in one call.
- [`Series::reshape`](reshape.md): one of the operations that changes later
  `size()` answers.

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
