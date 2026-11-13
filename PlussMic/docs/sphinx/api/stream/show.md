<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86826e14ef4e -->

# Stream::show

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
void Stream::show(std::ostream& os) const
```

## What It Does

Writes the stream's boxed-table rendering to `os` — the same rendering `operator<<`
writes, and the same content [`to_string()`](to_string.md) returns as a string, plus a
trailing newline `to_string()` omits.

## Parameters

**os** (`std::ostream&`)
    The destination stream to write to (e.g. `std::cout`, an `std::ostringstream`).

## Returns

`void`
    Writes to `os` as a side effect; produces no return value.

## Examples

Example 1. Write directly to `std::cout`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

stream.show(std::cout);
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

Example 2. `show()` and `operator<<` produce byte-identical output; both are one byte
longer than [`to_string()`](to_string.md) (the trailing newline).

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

std::ostringstream via_show;
stream.show(via_show);

std::ostringstream via_operator;
via_operator << stream;

std::cout << "equal: " << (via_show.str() == via_operator.str()) << "\n";
std::cout << "longer than to_string() by: "
          << (via_show.str().size() - stream.to_string().size()) << " byte(s)";
```

### Output

```text
equal: 1
longer than to_string() by: 1 byte(s)
```

## Notes

- Byte-identical to `operator<<`'s output — see Example 2.
- One byte longer than [`to_string()`](to_string.md): the trailing newline `show()`
  appends that `to_string()` omits.
- Any `std::ostream` works as `os`, not just `std::cout` — e.g. an `std::ostringstream` to
  capture the rendering into a string manually.

## See Also

- [`Stream::to_string`](to_string.md): the same rendering, returned as a string (minus the
  trailing newline) instead of written to a stream.
- [`Stream::to_json`](to_json.md): a structural JSON dump, a distinct text representation
  from this plain-text rendering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
