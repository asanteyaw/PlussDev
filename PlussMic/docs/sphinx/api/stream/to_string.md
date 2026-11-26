<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Stream::to_string

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::string Stream::to_string() const
```

## What It Does

Renders the stream as the text a Shelf prints — its labels beside its values, with a
`ptype` footer saying what it holds — as a `std::string`. It is the same rendering
`operator<<` writes to a stream, minus the trailing newline that appends.

```{important} Not quite byte-identical to operator<< — a trailing newline
`operator<<` ends its output with `\n`; `to_string()` does not. Two otherwise-identical
captures differ by exactly that one trailing byte — verified directly: `to_string().size()`
is one less than an `ostringstream` populated via `operator<<`. If you're comparing
rendered output across the two, account for the difference.
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
0  1
1  2
2  3
ptype: Series
```

Example 2. `to_string()` is one byte shorter than the equivalent `operator<<` capture — the
missing trailing newline.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

auto str = stream.to_string();
std::ostringstream oss;
oss << stream;

std::cout << "to_string length: " << str.size() << "\n";
std::cout << "operator<< length: " << oss.str().size();
```

### Output

```text
to_string length: 28
operator<< length: 29
```

## Notes

- One byte shorter than `operator<<`'s output — see the important callout above.
- A distinct text representation from [`to_json()`](to_json.md), which returns a
  structural JSON document rather than this plain text.

## See Also

- `operator<<`: the same rendering, written to a stream instead of returned as a string,
  with a trailing newline this method omits.
- [`Stream::to_json`](to_json.md): a structural JSON dump of the same stream, distinct
  from this plain-text rendering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_shelf.html">
    <span class="label">Previous</span>
    <span class="title">to_shelf</span>
  </a>
  <a class="next-link" href="ts.html">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>
