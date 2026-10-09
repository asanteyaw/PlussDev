<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 59869aeeb7b1 -->

# Stream::astype

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::astype(pluss::codem::ScalarType dtype) const -> Stream
```

## What It Does

Shorthand for `cast(SlateCastTarget::Series, {.series_dtype = dtype})` — converts the
stream to `Series` with the requested numeric dtype. Unlike
[`abs()`](abs.md)/[`clip()`](clip.md)/[`log()`](log.md)/[`exp()`](exp.md), `astype()` is
**not** restricted to already-`Series` streams: it works on any payload kind, parsing
text/timestamps into numbers where meaningful — values that fail to parse become `NULL`
rather than throwing.

## Parameters

**dtype** (`pluss::codem::ScalarType`)
    The target numeric dtype (e.g. `Int64`, `Float64`).

## Returns

`Stream`
    A new `Series`-kind stream with dtype `dtype`.

## Examples

Example 1. Convert numeric-looking text to `Series[int64]`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"}));

std::cout << stream.astype(pluss::codem::ScalarType::Int64);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 2. Non-numeric text fails to parse and becomes `NULL` — not a throw.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"abc", "def"}));

std::cout << stream.astype(pluss::codem::ScalarType::Int64);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | NA                                                                                                       
1            | NA                                                                                                       
========================================================================================================================
```

Example 3. Already-`Series` streams simply change dtype.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

std::cout << stream.astype(pluss::codem::ScalarType::Int64);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

## Notes

- Works on every payload kind, unlike the `Series`-only elementwise math family — a
  `Note`-kind stream of numeric text converts directly; see Example 1.
- A value that fails to parse becomes `NULL` (with a validity mask on the result), not a
  thrown error — see Example 2.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::cast`](cast.md): the more general conversion this method is shorthand for,
  supporting every target kind, not just `Series`.
- [`Stream::dtype`](dtype.md): reflects the dtype this method sets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_timeline.html">
    <span class="label">Previous</span>
    <span class="title">as_timeline</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
