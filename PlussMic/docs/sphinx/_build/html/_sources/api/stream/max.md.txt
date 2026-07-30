<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fed93d0c325e -->

# Stream::max

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::max() const -> Stream
```

## What It Does

Returns the maximum of a `Series`-kind stream's non-`NULL` values, as a one-element
`Stream`. Delegates to `Thread::max()`, so it's `Series`-only, same as
[`sum()`](sum.md)/[`mean()`](mean.md)/[`min()`](min.md)/[`var()`](var.md)/
[`std()`](std.md).

## Parameters

`None`

## Returns

`Stream`
    A one-element `Series` stream holding the maximum value.

## Examples

Example 1. Maximum of a stream with one masked `NULL` row — the `NULL` row is excluded
from consideration.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.max();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 5                                                                                                        
========================================================================================================================
```

## Notes

- `Series`-only; throws `"Thread::max is only valid for Series payload"` on
  `Note`/`Timeline`/`TimeIndex` streams.
- `NULL` rows are excluded from consideration.
- Returns a one-element `Stream`, not a raw scalar, same as every other reduction in this
  family.

## See Also

- [`Stream::min`](min.md): the mirror reduction.
- [`Stream::sum`](sum.md) / [`Stream::mean`](mean.md): the other reductions in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
