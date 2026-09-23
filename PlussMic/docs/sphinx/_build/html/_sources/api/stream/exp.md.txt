<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f82c4f40f1ac -->

# Stream::exp

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::exp() const -> Stream
```

## What It Does

Applies the elementwise exponential (`e^x`) to a `Series`-kind stream. Delegates to
`Thread::exp()`, so it's `Series`-only, same as
[`abs()`](abs.md)/[`clip()`](clip.md)/[`log()`](log.md) — the inverse of
[`log()`](log.md).

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream, same length, with every value replaced by its exponential.

## Examples

Example 1. Exponentiate a small range of values, including zero.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({0.0, 1.0, 2.0}));

std::cout << stream.exp();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2.71828182846                                                                                           
2            | 7.38905609893                                                                                           
========================================================================================================================
```

Example 2. `exp()` followed by `log()` recovers the original values (up to floating-point
precision).

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

std::cout << stream.exp().log();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 3. Calling `exp()` on a non-`Series` stream throws, matching every other method in
this elementwise math family.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

try {
  auto out = note.exp();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 44);
}
```

### Output

```text
Thread::exp is only valid for Series payload
```

## Notes

- `Series`-only; throws on `Note`/`Timeline`/`TimeIndex` streams — see Example 3.
- Large inputs can overflow to `inf` (`exp()` grows very fast) — no special handling beyond
  standard IEEE-754 float behavior.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::log`](log.md): the inverse transform.
- [`Stream::abs`](abs.md) / [`Stream::clip`](clip.md): the other elementwise numeric
  transforms in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
