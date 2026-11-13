<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55e606db8eef -->

# Stream::slice

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Returns a new stream containing the contiguous half-open range `[start, stop)` — same
convention as [`to_slate(start, stop)`](to_slate.md). `stop` beyond the stream's length
clamps to the end rather than throwing.

## Parameters

**start** (`std::int64_t`)
    Inclusive starting row index.

**stop** (`std::int64_t`)
    Exclusive ending row index.

## Returns

`Stream`
    A new stream containing rows `[start, stop)`.

## Examples

Example 1. Slice a middle range.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));

std::cout << stream.slice(1, 4);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2                                                                                                        
1            | 3                                                                                                        
2            | 4                                                                                                        
========================================================================================================================
```

Example 2. A `stop` beyond the stream's length clamps to the end instead of throwing.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));

std::cout << stream.slice(2, 100);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 3                                                                                                        
1            | 4                                                                                                        
2            | 5                                                                                                        
========================================================================================================================
```

## Notes

- `stop` is exclusive; `slice(1, 4)` returns 3 rows (indices 1, 2, 3).
- `stop` beyond the stream's length clamps to the end rather than throwing — see
  Example 2.
- Returns a new `Stream`; the receiver is unmodified.

## See Also

- [`Stream::head`](head.md) / [`Stream::tail`](tail.md): fixed-count selections from either
  end, instead of an arbitrary range.
- [`Stream::take`](take.md): select arbitrary (possibly non-contiguous, possibly reordered)
  positions instead of a contiguous range.
- [`Stream::to_slate`](to_slate.md): the two-argument overload uses the identical
  `[start, stop)` convention, converting to `Slate` instead of `Stream`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
