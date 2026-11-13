<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4f0854c21b34 -->

# Stream::sum

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::sum() const
```

## What It Does

Returns the sum of a `Series`-kind stream's values, as a one-element `Stream` —
`NULL`-marked rows are excluded from the computation. Delegates to `Slate::sum()`, so it's
`Series`-only, same as [`mean()`](mean.md)/[`min()`](min.md)/[`max()`](max.md)/
[`var()`](var.md)/[`std()`](std.md).

## Parameters

`None`

## Returns

`Stream`
    A one-element `Series` stream holding the sum.

## Examples

Example 1. Sum a stream with one masked `NULL` row — the `NULL` row is excluded.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.sum();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 13                                                                                                       
========================================================================================================================
```

`1 + 3 + 4 + 5 = 13` — the masked `2` at index 1 is excluded, not treated as `0`.

Example 2. Calling `sum()` on a non-`Series` stream throws.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

try {
  auto out = note.sum();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 44);
}
```

### Output

```text
Slate::sum is only valid for Series payload
```

## Notes

- `Series`-only; throws on `Note`/`Timeline`/`TimeIndex` streams — see Example 2.
- `NULL` rows are excluded from the sum, not treated as `0` — see Example 1.
- Returns a one-element `Stream`, not a raw scalar — same pattern as every other reduction
  in this family ([`mean()`](mean.md), [`min()`](min.md), [`max()`](max.md),
  [`var()`](var.md), [`std()`](std.md), [`ndistinct()`](ndistinct.md)).
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::mean`](mean.md): the sum divided by the non-`NULL` count.
- [`Stream::min`](min.md) / [`Stream::max`](max.md): the other order-statistic reductions
  in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
