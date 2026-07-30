<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 647145cc82f1 -->

# Stream::tail

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::tail(std::int64_t n) const -> Stream
```

## What It Does

Returns a new stream containing the last `n` rows — the mirror of [`head()`](head.md).
`n` is required. Same clamping behavior as `head()`: requesting more rows than exist
clamps to the whole stream, and `n <= 0` returns an empty stream, rather than throwing.

## Parameters

**n** (`std::int64_t`)
    The number of trailing rows to keep.

## Returns

`Stream`
    A new stream containing at most the last `n` rows.

## Examples

Example 1. Take the last two rows.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

std::cout << stream.tail(2);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 40                                                                                                       
1            | 50                                                                                                       
========================================================================================================================
```

## Notes

- No zero-argument overload — `n` must be supplied explicitly.
- Clamps at both ends rather than throwing, same as [`head()`](head.md).
- Returns a new `Stream`; the receiver is unmodified.
- The rows returned by `tail(n)` re-index from `0` in the output, same as every other
  row-selecting method — not from the original stream's positional offset.

## See Also

- [`Stream::head`](head.md): the mirror operation, taking the first `n` rows.
- [`Stream::slice`](slice.md): an arbitrary `[start, stop)` range instead of a fixed count
  from the back.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
