<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b1a2aeba4233 -->

# Shelf::info_string

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
std::string Shelf::info_string() const
```

## What It Does

Builds a short, fixed-format summary of the shelf's metadata — name, payload kind, dtype,
row count, and a valid/null breakdown — as a plain `std::string`. This is metadata only;
unlike [`to_string()`](to_string.md), it never renders the shelf's
actual row values. Use it for logging, quick sanity checks, or anywhere you want a
one-glance summary of what a shelf contains without printing every row.

## Parameters

`None`

## Returns

`std::string`
    A multi-line summary in this fixed shape (no trailing newline):
    ```text
    Shelf Info
    Name: <name, or "<unnamed>">
    Kind: <Series|Note|Timeline|TimeIndex>
    Dtype: <dtype string, e.g. float64, utf8, int64>
    Cases: <total row count>
    Non-Null Cases: <valid row count>
    Null Cases: <invalid row count>
    ```

## Examples

Example 1. An unnamed numeric shelf with no `NULL` rows.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.info_string();
std::cout << out;
```

### Output

```text
Shelf Info
Name: <unnamed>
Kind: Series
Dtype: float64
Cases: 3
Non-Null Cases: 3
Null Cases: 0
```

Example 2. A named shelf with a `NULL` row — [`name()`](name.md) assigns the display name
that shows up as `Name:` here, and `Non-Null Cases`/`Null Cases` reflect the validity mask.

```cpp
auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true})).name("sales");

auto out = scores.info_string();
std::cout << out;
```

### Output

```text
Shelf Info
Name: sales
Kind: Series
Dtype: float64
Cases: 3
Non-Null Cases: 2
Null Cases: 1
```

Example 3. Non-numeric payload kinds report their own `Kind`/`Dtype` — here, `Note`-kind
text reports dtype `utf8`.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "Bola"}));

auto out = names.info_string();
std::cout << out;
```

### Output

```text
Shelf Info
Name: <unnamed>
Kind: Note
Dtype: utf8
Cases: 2
Non-Null Cases: 2
Null Cases: 0
```

## Notes

- Metadata only — never renders row values. Use [`to_string()`](to_string.md)
  instead if you want the actual data rendered.
- The returned string has **no trailing newline** — [`info()`](info.md) is the
  convenience wrapper that both calls this and adds one when writing to a stream.
- `Name:` reads `<unnamed>` unless the shelf was given one via [`name()`](name.md).

## See Also

- [`Shelf::info`](info.md): writes this same text straight to a stream (`std::cout` by
  default) with a trailing newline, instead of returning it as a string.
- [`Shelf::to_string`](to_string.md): render the shelf's
  actual row values, rather than metadata.
- [`Shelf::name`](name.md) / [`Shelf::named`](named.md): set or check the display name
  that shows up as `Name:` here.
- [`Shelf::size`](size.md) / [`Shelf::kind`](kind.md) / [`Shelf::dtype`](dtype.md): the
  individual accessors this summary is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info.html">
    <span class="label">Previous</span>
    <span class="title">info</span>
  </a>
  <a class="next-link" href="interp.html">
    <span class="label">Next</span>
    <span class="title">interp</span>
  </a>
</div>
