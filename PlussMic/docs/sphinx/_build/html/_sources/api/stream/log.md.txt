<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8bbb2cf52087 -->

# Stream::log

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::log() const -> Stream
```

## What It Does

Applies the elementwise natural logarithm to a `Series`-kind stream. Delegates to
`Thread::log()`, so it's `Series`-only, same as
[`abs()`](abs.md)/[`clip()`](clip.md)/[`exp()`](exp.md). Follows ordinary IEEE-754
floating-point rules rather than throwing for out-of-domain input: `log(0)` is `-inf`, and
`log` of a negative value is `nan` — neither raises an exception.

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream, same length, with every value replaced by its natural log.

## Examples

Example 1. Natural log of strictly positive values.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.718281828, 7.389056099}));

std::cout << stream.log();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 0                                                                                                        
1            | 0.999999999831                                                                                          
2            | 2.00000000001                                                                                            
========================================================================================================================
```

Example 2. Zero and negative inputs produce `-inf`/`nan` rather than throwing.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({0.0, -1.0, 2.0}));

std::cout << stream.log();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | -inf                                                                                                     
1            | nan                                                                                                      
2            | 0.69314718056                                                                                            
========================================================================================================================
```

Example 3. Calling `log()` on a non-`Series` stream throws, matching every other method in
this elementwise math family.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

try {
  auto out = note.log();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 44);
}
```

### Output

```text
Thread::log is only valid for Series payload
```

## Notes

- `Series`-only; throws on `Note`/`Timeline`/`TimeIndex` streams — see Example 3.
- Domain errors (zero, negative input) produce `-inf`/`nan` rather than throwing — see
  Example 2. If you need to guard against these, check the input with
  [`filter()`](filter.md) or compare against `0.0` before calling `log()`.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::exp`](exp.md): the inverse transform.
- [`Stream::abs`](abs.md) / [`Stream::clip`](clip.md): the other elementwise numeric
  transforms in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lbl.html">
    <span class="label">Previous</span>
    <span class="title">lbl</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
