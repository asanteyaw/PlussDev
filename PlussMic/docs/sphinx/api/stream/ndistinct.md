<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: be2d7de169a7 -->

# Stream::ndistinct

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::ndistinct(DistinctOptions options = {}) const
```

## What It Does

Returns the **count** of distinct values as a one-element `Series[int64]` stream — the
same `options` and same `dropnull` semantics as [`distinct()`](distinct.md), but returning
just the count instead of the distinct values themselves.

```{important} Returns a Stream, not a plain integer
`ndistinct()` returns `Stream`, not `std::int64_t` — a one-row, one-column stream wrapping
the count. Use `.as_series()` (further unwrapped as needed) or compare it via its own
values if you need the raw integer, rather than assuming it converts implicitly.
```

## Parameters

**options** (`DistinctOptions`, defaults to `{}`)
    `dropnull` (default `true`) controls whether a `NULL` value counts as one of the
    distinct values being counted — identical semantics to [`distinct()`](distinct.md).

## Returns

`Stream`
    A one-element `Series[int64]` stream holding the distinct-value count.

## Examples

Example 1. Count distinct values, `NULL` excluded by default.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, true, true, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0, 1.0}), mask);

std::cout << stream.ndistinct();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 3                                                                                                        
========================================================================================================================
```

Example 2. Including `NULL` as its own distinct value adds one to the count.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, true, true, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0, 1.0}), mask);

std::cout << stream.ndistinct(pluss::kindle::DistinctOptions{.dropnull = false});
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 4                                                                                                        
========================================================================================================================
```

## Notes

- Returns a `Stream`, not a raw `std::int64_t` — see the important callout above.
- Always exactly one row, regardless of the receiver's own size.
- Same `dropnull` semantics as [`distinct()`](distinct.md): a `NULL` source value, when
  counted, always contributes exactly `1` to the total, not one per occurrence.

## See Also

- [`Stream::distinct`](distinct.md): the values themselves, rather than just their count.
- [`Stream::freq`](freq.md): each distinct value paired with its own occurrence count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
