<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d80403ce5d23 -->

# Stream::take

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::take(std::span<const std::int64_t> indices) const -> Stream
```

## What It Does

Returns a new stream containing the rows at `indices`, in the order given — unlike
[`slice()`](slice.md), `indices` need not be contiguous or increasing, and may repeat a
position more than once. This makes `take()` the general reordering/gather primitive: use
it to reorder rows, duplicate a row, or select an arbitrary, non-contiguous subset.

## Parameters

**indices** (`std::span<const std::int64_t>`)
    The row positions to select, in the order they should appear in the result. Accepts
    anything convertible to `std::span` — e.g. a `std::vector<std::int64_t>`.

## Returns

`Stream`
    A new stream with one row per entry in `indices`, in that order.

## Examples

Example 1. Reorder and repeat positions in a single call.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::vector<std::int64_t> picks{2, 0, 0, 1};
std::cout << stream.take(picks);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=4
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 30                                                                                                       
1            | 10                                                                                                       
2            | 10                                                                                                       
3            | 20                                                                                                       
========================================================================================================================
```

Example 2. An out-of-range index throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

try {
  std::vector<std::int64_t> bad{5};
  auto out = stream.take(bad);
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 24);
}
```

### Output

```text
Slate: index out of bou
```

## Notes

- Order and repetition are both meaningful — the result has exactly `indices.size()` rows,
  in the order given, with duplicates preserved.
- An out-of-range index throws `"Slate: index out of bounds"` — unlike
  [`head()`](head.md)/[`tail()`](tail.md)/[`slice()`](slice.md), which clamp instead.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::slice`](slice.md): a contiguous `[start, stop)` range, which clamps rather than
  throwing on an out-of-range `stop`.
- [`Stream::filter`](filter.md): select rows by a boolean mask instead of explicit
  positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
