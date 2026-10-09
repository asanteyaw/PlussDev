<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 08a0f2043bc6 -->

# Stream::isnull

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::isnull() const -> Stream
```

## What It Does

Returns a new `Bool`-dtype `Series` stream, same length as the receiver, marking which
rows are `NULL` (validity-masked as invalid, or an unmarked `NaN` in a numeric payload).
Delegates to `Slate::isnull()`; works uniformly across all four payload kinds — unlike the
elementwise math family, `isnull()` never throws based on kind.

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream of `bool` values: `true` where the row is `NULL`, `false`
    where it has a value.

## Examples

Example 1. Rows marked invalid via a validity mask.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0}), mask);

std::cout << stream.isnull();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=4
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | false                                                                                                    
1            | true                                                                                                     
2            | false                                                                                                    
3            | true                                                                                                     
========================================================================================================================
```

Example 2. Works identically for a `Note`-kind stream — no kind restriction.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false});
auto notes = pluss::kindle::Stream(pluss::make_note({"a", "b"}), mask);

std::cout << notes.isnull();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | false                                                                                                    
1            | true                                                                                                     
========================================================================================================================
```

## Notes

- Works on every payload kind — no `Series`-only restriction, unlike
  [`abs()`](abs.md)/[`clip()`](clip.md)/[`log()`](log.md)/[`exp()`](exp.md).
- The result is always `Series`-kind (`Series[bool]`), regardless of the receiver's own
  kind.
- In every case tested here, produces the same result as [`invalid()`](invalid.md) — the
  two call different underlying `Slate` methods but agree in ordinary use.

## See Also

- [`Stream::notnull`](notnull.md): the exact logical complement of this method.
- [`Stream::invalid`](invalid.md): a closely related check with the same practical result.
- [`Stream::fillnull`](fillnull.md) / [`Stream::dropnull`](dropnull.md): act on the rows
  this method identifies.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>
