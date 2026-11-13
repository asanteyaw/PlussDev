<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 52f06c926728 -->

# Stream::from_slate

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
static Stream Stream::from_slate(const pluss::table::Slate& slate, std::optional<pluss::table::Slate> cases = std::nullopt)
```

## What It Does

A static factory that builds a `Stream` from a `pluss::table::Slate` — the inverse of
[`to_slate()`](to_slate.md), and the way to bring a `Slate` produced elsewhere in the
table stack (e.g. a `DataFrame` column extraction) into `Stream`'s API. The optional
`cases` argument attaches a row-label axis in the same call, equivalent to constructing the
`Stream` and then calling [`with_label()`](with_label.md).

## Parameters

**slate** (`const pluss::table::Slate&`)
    The source slate. Its payload kind becomes the resulting stream's kind.

**cases** (`std::optional<pluss::table::Slate>`, defaults to `std::nullopt`)
    An optional row-label axis to attach — see [`has_label()`](has_label.md)/
    [`lbl()`](lbl.md).

## Returns

`Stream`
    A new stream wrapping `slate`'s payload, with `cases` attached as its label axis if
    supplied.

## Examples

Example 1. Build a stream from a plain `Slate`.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0, 20.0, 30.0}));

auto stream = pluss::kindle::Stream::from_slate(slate);
std::cout << stream;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 20                                                                                                       
2            | 30                                                                                                       
========================================================================================================================
```

Example 2. Attach a row-label axis in the same call via `cases`.

```cpp
using pluss::table::Slate;

auto slate = Slate(pluss::make_series({10.0, 20.0}));
auto cases = Slate(pluss::make_note({"a", "b"}));

auto stream = pluss::kindle::Stream::from_slate(slate, cases);
std::cout << "has_label: " << stream.has_label();
```

### Output

```text
has_label: 1
```

## Notes

- `cases`, when supplied, must match `slate`'s length — the same requirement documented
  for [`with_label()`](with_label.md).
- A static method; call it as `Stream::from_slate(...)`, not on an existing instance.
- Round-trips with [`to_slate()`](to_slate.md): `Stream::from_slate(stream.to_slate())`
  reconstructs an equivalent (unlabeled) stream.

## See Also

- [`Stream::to_slate`](to_slate.md): the inverse conversion, from a `Stream` to a
  `Slate`.
- [`Stream::with_label`](with_label.md): attach a label axis after construction, instead of
  in the same call as this method's `cases` argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="has_label.html">
    <span class="label">Next</span>
    <span class="title">has_label</span>
  </a>
</div>
