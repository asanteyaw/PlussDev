<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4b1b10e4f9fd -->

# Stream::clip

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::clip(double min_value, double max_value) const -> Stream
```

## What It Does

Clamps every value in a `Series`-kind stream to `[min_value, max_value]` — values below
`min_value` become `min_value`, values above `max_value` become `max_value`, and everything
in between passes through unchanged. Delegates to `Slate::clip()`, so it's `Series`-only,
same as [`abs()`](abs.md)/[`log()`](log.md)/[`exp()`](exp.md).

## Parameters

**min_value** (`double`)
    The lower bound. Values below this are replaced with it.

**max_value** (`double`)
    The upper bound. Values above this are replaced with it.

## Returns

`Stream`
    A new `Series`-kind stream, same length, with every value clamped into
    `[min_value, max_value]`.

## Examples

Example 1. Clamp values on both ends of the range.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 5.0, 10.0, 15.0, 20.0}));

std::cout << stream.clip(5.0, 15.0);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=5
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 5                                                                                                        
1            | 5                                                                                                        
2            | 10                                                                                                       
3            | 15                                                                                                       
4            | 15                                                                                                       
========================================================================================================================
```

Example 2. Calling `clip()` on a non-`Series` stream throws, matching every other method
in this elementwise math family.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

try {
  auto out = note.clip(0.0, 1.0);
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 45);
}
```

### Output

```text
Slate::clip is only valid for Series payload
```

## Notes

- `Series`-only; throws on `Note`/`Timeline`/`TimeIndex` streams — see Example 2.
- `min_value`/`max_value` are not validated against each other; passing `min_value >
  max_value` is not checked for here and follows whatever `Slate::clip()` does with an
  inverted range — keep them in the expected order to avoid surprising results.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::abs`](abs.md): take the magnitude instead of clamping to a range.
- [`Stream::log`](log.md) / [`Stream::exp`](exp.md): the other elementwise numeric
  transforms in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_label_.html">
    <span class="label">Previous</span>
    <span class="title">clear_label_</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
