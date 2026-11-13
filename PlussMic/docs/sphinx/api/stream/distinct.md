<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5bc4cd883948 -->

# Stream::distinct

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::distinct(DistinctOptions options = {}) const
```

### `DistinctOptions` fields

| Field | Type | Default |
|---|---|---|
| `dropnull` | `bool` | `true` |

## What It Does

Returns a new stream containing each distinct value from the receiver exactly once. By
default (`dropnull=true`), `NULL` is excluded from the result even if it appears in the
source; set `dropnull=false` to include a single `NULL` entry when the source has at least
one.

## Parameters

**options** (`DistinctOptions`, defaults to `{}`)
    `dropnull` (default `true`) controls whether a `NULL` value counts as one of the
    distinct values in the result.

## Returns

`Stream`
    A new stream with one row per distinct value found in the receiver.

## Examples

Example 1. Default behavior: `NULL` excluded, duplicates collapsed.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, true, true, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0, 1.0}), mask);

std::cout << stream.distinct();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 2. `dropnull=false` includes a single `NULL` entry as one of the distinct values.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, true, true, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0, 1.0}), mask);

std::cout << stream.distinct(pluss::kindle::DistinctOptions{.dropnull = false});
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=4
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
3            | NA                                                                                                       
========================================================================================================================
```

## Notes

- `NULL` is excluded by default; even when included via `dropnull=false`, every `NULL`
  source value collapses into a single distinct `NULL` entry, not one per occurrence.
- The result's own kind matches the receiver's kind.
- For just the *count* of distinct values rather than the values themselves, use
  [`ndistinct()`](ndistinct.md).

## See Also

- [`Stream::ndistinct`](ndistinct.md): the count of distinct values, as a one-element
  stream.
- [`Stream::freq`](freq.md): distinct values paired with their occurrence counts.
- [`Stream::dropnull`](dropnull.md): removes `NULL` rows without deduplicating.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="drop_meta.html">
    <span class="label">Next</span>
    <span class="title">drop_meta</span>
  </a>
</div>
