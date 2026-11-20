<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 52f06c926728 -->

# Stream::from_shelf

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
static Stream Stream::from_shelf(const pluss::table::Shelf& shelf, std::optional<pluss::table::Shelf> cases = std::nullopt)
```

## What It Does

A static factory that builds a `Stream` from a `pluss::table::Shelf` — the inverse of
[`to_shelf()`](to_shelf.md), and the way to bring a `Shelf` produced elsewhere in the
table stack (e.g. a `DataFrame` column extraction) into `Stream`'s API. The optional
`cases` argument attaches a row-label axis in the same call, equivalent to constructing the
`Stream` and then calling [`with_label()`](with_label.md).

## Parameters

**shelf** (`const pluss::table::Shelf&`)
    The source shelf. Its payload kind becomes the resulting stream's kind.

**cases** (`std::optional<pluss::table::Shelf>`, defaults to `std::nullopt`)
    An optional row-label axis to attach — see [`has_label()`](has_label.md)/
    [`lbl()`](lbl.md).

## Returns

`Stream`
    A new stream wrapping `shelf`'s payload, with `cases` attached as its label axis if
    supplied.

## Examples

Example 1. Build a stream from a plain `Shelf`.

```cpp
auto shelf = pluss::table::Shelf(pluss::make_series({10.0, 20.0, 30.0}));

auto stream = pluss::kindle::Stream::from_shelf(shelf);
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
using pluss::table::Shelf;

auto shelf = Shelf(pluss::make_series({10.0, 20.0}));
auto cases = Shelf(pluss::make_note({"a", "b"}));

auto stream = pluss::kindle::Stream::from_shelf(shelf, cases);
std::cout << "has_label: " << stream.has_label();
```

### Output

```text
has_label: 1
```

## Notes

- `cases`, when supplied, must match `shelf`'s length — the same requirement documented
  for [`with_label()`](with_label.md).
- A static method; call it as `Stream::from_shelf(...)`, not on an existing instance.
- Round-trips with [`to_shelf()`](to_shelf.md): `Stream::from_shelf(stream.to_shelf())`
  reconstructs an equivalent (unlabeled) stream.

## See Also

- [`Stream::to_shelf`](to_shelf.md): the inverse conversion, from a `Stream` to a
  `Shelf`.
- [`Stream::with_label`](with_label.md): attach a label axis after construction, instead of
  in the same call as this method's `cases` argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="gather.html">
    <span class="label">Next</span>
    <span class="title">gather</span>
  </a>
</div>
