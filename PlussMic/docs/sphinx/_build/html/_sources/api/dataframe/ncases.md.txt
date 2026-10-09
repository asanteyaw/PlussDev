<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6af5646f5d13 -->

# DataFrame::ncases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::ncases() const -> std::int64_t
```

## What It Does

Returns the number of cases (rows) in the frame. This is the same count that appears
as the first number in a printed frame's `[N cs x M feat]` footer, and the length
every case-aligned `Slate` argument (the `Axis::Case` side of [`add()`](add.md),
[`eq()`](eq.md), and similar methods) is checked against.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of cases in the frame.

## Examples

Example 1. Read the case count directly.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.ncases();
std::cout << out;
```

### Output

```text
3
```

This matches the `3` in `scores`'s own `[3 cs x 2 feat]` footer when printed.

## Notes

- `ncases()` counts rows, not distinct case labels — a frame is not required to have
  unique case labels, so `ncases()` can exceed the number of *distinct* labels
  reported by [`ndistinct()`](ndistinct.md) on the case axis.
- Every feature `Slate` in the frame has exactly `ncases()` elements; this is an
  invariant the frame maintains internally.

## See Also

- [`DataFrame::nfeatures`](nfeatures.md): the companion count for columns instead of
  rows.
- [`DataFrame::shape`](shape.md): both counts together as one struct.
- [`DataFrame::ndistinct`](ndistinct.md): count of distinct values, as opposed to raw
  row count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mul.html">
    <span class="label">Previous</span>
    <span class="title">mul</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
