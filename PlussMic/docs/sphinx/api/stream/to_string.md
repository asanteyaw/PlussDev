<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Stream::to_string

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::string Stream::to_string() const
```

## What It Does

Renders the stream's boxed-table view as a `std::string` — the same rendering
`operator<<` and [`show()`](show.md) write to a stream, minus the trailing newline those
two append.

```{important} Not quite byte-identical to show()/operator<< — a trailing newline
`show(ostream)`/`operator<<` end their output with `\n`; `to_string()` does not. Two
otherwise-identical captures differ by exactly that one trailing byte — verified directly:
`to_string().size()` is one less than an `ostringstream` populated via `show()`. If you're
comparing rendered output across these three, account for the difference.
```

## Parameters

`None`

## Returns

`std::string`
    The rendered stream table, without a trailing newline.

## Examples

Example 1. Render a stream to a string.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

std::cout << stream.to_string();
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

Example 2. `to_string()` is one byte shorter than the equivalent `show()` capture — the
missing trailing newline.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

auto str = stream.to_string();
std::ostringstream oss;
stream.show(oss);

std::cout << "to_string length: " << str.size() << "\n";
std::cout << "show length: " << oss.str().size();
```

### Output

```text
to_string length: 1055
show length: 1056
```

## Notes

- One byte shorter than [`show()`](show.md)/`operator<<`'s output — see the important
  callout above.
- A distinct text representation from [`to_json()`](to_json.md), which returns a
  structural JSON document rather than this plain-text table.

## See Also

- [`Stream::show`](show.md): the same rendering, written to a stream instead of returned
  as a string, with a trailing newline this method omits.
- [`Stream::to_json`](to_json.md): a structural JSON dump of the same stream, distinct
  from this plain-text rendering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_json.html">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="to_slate.html">
    <span class="label">Next</span>
    <span class="title">to_slate</span>
  </a>
</div>
