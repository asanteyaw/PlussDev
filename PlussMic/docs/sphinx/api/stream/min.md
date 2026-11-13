<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c74936571923 -->

# Stream::min

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::min() const
```

## What It Does

Returns the minimum of a `Series`-kind stream's non-`NULL` values, as a one-element
`Stream`. Delegates to `Slate::min()`, so it's `Series`-only, same as
[`sum()`](sum.md)/[`mean()`](mean.md)/[`max()`](max.md)/[`var()`](var.md)/
[`std()`](std.md).

## Parameters

`None`

## Returns

`Stream`
    A one-element `Series` stream holding the minimum value.

## Examples

Example 1. Minimum of a stream with one masked `NULL` row — the `NULL` row is excluded
from consideration.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.min();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
========================================================================================================================
```

## Notes

- `Series`-only; throws `"Slate::min is only valid for Series payload"` on
  `Note`/`Timeline`/`TimeIndex` streams.
- `NULL` rows are excluded from consideration.
- Returns a one-element `Stream`, not a raw scalar, same as every other reduction in this
  family.

## See Also

- [`Stream::max`](max.md): the mirror reduction.
- [`Stream::sum`](sum.md) / [`Stream::mean`](mean.md): the other reductions in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="meta.html">
    <span class="label">Previous</span>
    <span class="title">meta</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
