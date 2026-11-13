<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b65f51f544cb -->

# Stream::freq

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::freq(FreqOptions options = {}) const
```

### `FreqOptions` fields

| Field | Type | Default |
|---|---|---|
| `top` | `std::int64_t` | `0` |
| `dropnull` | `bool` | `true` |

## What It Does

Returns a value-counts table: one row per distinct value, sorted by descending occurrence
count. The distinct **values become the result's row labels** — see
[`lbl()`](lbl.md) — and the result's own numeric payload is each value's occurrence count.
`top` limits the result to the `top` most frequent values (`0`, the default, means no
limit); `dropnull` controls whether `NULL` is counted as its own distinct value, same as
[`distinct()`](distinct.md)/[`ndistinct()`](ndistinct.md).

```{important} The distinct values are labels, not the payload — counts are the payload
Unlike [`distinct()`](distinct.md) (which returns the values themselves as the payload),
`freq()`'s returned stream has `Series[int64]` **counts** as its payload, with the original
values attached via [`lbl()`](lbl.md) as row labels. Read a value with
`out.lbl().values()`, not `out` directly, if you need the values rather than the counts.
```

## Parameters

**options** (`FreqOptions`, defaults to `{}`)
    `top` (default `0`, meaning unlimited) caps the result to the `top` most frequent
    values. `dropnull` (default `true`) controls whether `NULL` is counted.

## Returns

`Stream`
    A `Series[int64]` stream of occurrence counts, sorted descending, with the
    corresponding distinct values attached as row labels.

## Examples

Example 1. Value counts, sorted by descending frequency, values as labels.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, true, true, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0, 1.0}), mask);

std::cout << stream.freq();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
2            | 2                                                                                                        
1            | 1                                                                                                        
3            | 1                                                                                                        
========================================================================================================================
```

Value `2` occurs twice (the most frequent), so it sorts first; the leftmost column is the
**label** (the original value), not a positional row index.

Example 2. `top` limits the result to the single most frequent value.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, true, true, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0, 1.0}), mask);

std::cout << stream.freq(pluss::kindle::FreqOptions{.top = 1});
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=1
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
2            | 2                                                                                                        
========================================================================================================================
```

## Notes

- The distinct values become row **labels**, not the payload — see the important callout
  above. This is the single most important thing to know before consuming this method's
  result.
- Sorted by descending count; ties are not documented as stable in any particular
  secondary order — don't rely on tie order.
- `top=0` (the default) means unlimited — every distinct value gets a row.

## See Also

- [`Stream::distinct`](distinct.md): the distinct values themselves as the payload, with no
  counts.
- [`Stream::ndistinct`](ndistinct.md): just the total count of distinct values.
- [`Stream::lbl`](lbl.md): the accessor to read this method's row labels (the original
  values) back out.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="from_slate.html">
    <span class="label">Next</span>
    <span class="title">from_slate</span>
  </a>
</div>
