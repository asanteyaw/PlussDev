<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 57d79bb70393 -->

# Stream::abs

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::abs() const
```

## What It Does

Returns a new `Series`-kind stream whose values are the elementwise absolute value of the
current numeric payload. Delegates to `Slate::abs()` under the hood, so it only accepts a
`Series`-kind stream — calling it on `Note`/`Timeline`/`TimeIndex` throws.

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream, same length, with every value replaced by its absolute
    value.

## Examples

Example 1. Absolute value of a mix of negative, positive, and zero values.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({-3.0, 4.0, -5.5, 0.0}));

std::cout << stream.abs();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=4
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 3                                                                                                        
1            | 4                                                                                                        
2            | 5.5                                                                                                      
3            | 0                                                                                                        
========================================================================================================================
```

Example 2. Calling `abs()` on a non-`Series` stream throws — this is a `Series`-only
operation.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

try {
  auto out = note.abs();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 44);
}
```

### Output

```text
Slate::abs is only valid for Series payload
```

## Notes

- `Series`-only; throws on `Note`/`Timeline`/`TimeIndex` streams — see Example 2. The
  thrown message is inherited verbatim from `Slate::abs()`, the delegate this method
  calls internally.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.
- Returns a new `Stream`; never mutates the receiver — `Stream` has no math-family mutating
  (`_`-suffixed) overloads.

## See Also

- [`Stream::clip`](clip.md): clamp values to a range instead of taking their magnitude.
- [`Stream::log`](log.md) / [`Stream::exp`](exp.md): the other elementwise numeric
  transforms, sharing the same `Series`-only restriction.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="as_note.html">
    <span class="label">Next</span>
    <span class="title">as_note</span>
  </a>
</div>
