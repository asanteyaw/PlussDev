<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5912a46b75c1 -->

# Shelf::compare

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::compare(double other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(std::string_view other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(const char* other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(const pluss::Series& other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(const pluss::Note& other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(const pluss::Timeline& other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(const pluss::Duration& other, ShelfBinaryCompareOp op) const
Shelf Shelf::compare(const Shelf& other, ShelfBinaryCompareOp op) const
```

## What It Does

The single, `op`-parameterized entry point behind every comparison on `Shelf` — the
[`eq()`](eq.md), [`ne()`](ne.md), [`lt()`](lt.md), [`le()`](le.md), [`gt()`](gt.md),
[`ge()`](ge.md) convenience methods, and the free comparison operators (`==`, `!=`, `<`,
`<=`, `>`, `>=`), are all thin wrappers that call `compare()` with a fixed
`ShelfBinaryCompareOp`. Reach for `compare()` directly when the operator is itself a
runtime value (selected from a variable, a config file, a user-facing filter builder);
otherwise the named methods or operators read more naturally at the call site.

### `ShelfBinaryCompareOp` values

| Value | Numeric value | Description |
|---|---|---|
| `Eq` | `0` | equal |
| `Ne` | `1` | not equal |
| `Lt` | `2` | less than |
| `Le` | `3` | less than or equal |
| `Gt` | `4` | greater than |
| `Ge` | `5` | greater than or equal |

The right-hand side can be a numeric scalar, a text scalar, one of the four native pluss
payload types (`Series`, `Note`, `Timeline`, `Duration`), or another `Shelf` — every
overload converges on the same row-by-row boolean result.

```{important} A text literal against a Timeline shelf is parsed as a date, not compared as text
Calling the `std::string_view` overload on a `Timeline`-kind shelf does **not** perform
a lexical string comparison — the literal is parsed as a timestamp first (via
`pluss::make_timeline`), then compared temporally. Only `Note`-kind shelves (and, as a
fallback, everything else) compare the literal as plain text. See Example 4.
```

## Parameters

**other**

The right-hand comparison target — a numeric scalar (`double`), a text scalar
(`std::string_view`/`const char*`), one of `Series`/`Note`/`Timeline`/`Duration`, or
another `Shelf`. Non-`Shelf` payloads that carry more than one row are compared
positionally and must match `size()`; a single-element (`dim() == 0`) payload
broadcasts across every row instead.

**op** (`ShelfBinaryCompareOp`)

Which comparison to perform — `Eq`, `Ne`, `Lt`, `Le`, `Gt`, or `Ge`.

## Returns

`Shelf`
    A `Series`-kind shelf of `Bool` values, one per row. Rows that are invalid in the
    source shelf compare `false` rather than propagating invalidity.

## Examples

Example 1. Broadcast one numeric scalar across a numeric shelf.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));

auto out = scores.compare(20.0, ShelfBinaryCompareOp::Ge);
std::cout << out;
```

### Output

```text
0  false
1  true
2  true
3  true
ptype: Series
```

Example 2. Compare a `Note`-backed shelf against a text literal — case-sensitive, exact
text comparison, since the source is `Note`-kind.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "BOLA", "cHeN"}));

auto out = names.compare("BOLA", ShelfBinaryCompareOp::Eq);
std::cout << out;
```

### Output

```text
0  false
1  true
2  false
ptype: Series
```

Example 3. Compare two numeric shelves element by element — a positional, row-by-row
comparison, not a broadcast.

```cpp
auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Shelf(pluss::make_series({10.0, 10.0, 35.0, 45.0}));

auto out = scores.compare(other, ShelfBinaryCompareOp::Ne);
std::cout << out;
```

### Output

```text
0  false
1  true
2  true
3  true
ptype: Series
```

Example 4. Against a `Timeline`-kind shelf, a text literal is parsed as a date before
comparing — see the important callout above.

```cpp
auto stamps = Shelf(pluss::make_timeline({
    "2024-01-01T00:00:00Z",
    "2024-02-01T00:00:00Z",
}));

auto out = stamps.compare("2024-01-15T00:00:00Z", ShelfBinaryCompareOp::Lt);
std::cout << out;
```

### Output

```text
0  true
1  false
ptype: Series
```

Example 5. Positional (`Shelf`-to-`Shelf`) comparison requires matching lengths — a
size mismatch throws rather than broadcasting or truncating.

```cpp
auto a = Shelf(pluss::make_series({1.0, 2.0}));
auto b = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto out = a.compare(b, ShelfBinaryCompareOp::Eq);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::compare: positional alignment requires identical lengths
```

## Notes

- Invalid (`NULL`) rows in the source shelf compare `false`, not `NULL` — `compare()`
  never propagates invalidity into the result.
- A scalar `Series`/`Note` payload (`dim() == 0`) broadcasts; a 1D payload must match
  `size()` exactly or the call throws.
- Every named comparison method (`eq`, `ne`, `lt`, `le`, `gt`, `ge`) and every free
  comparison operator ultimately calls `compare()` with a fixed `op` — prefer those for
  readability when the operator is known at the call site.

## See Also

- [`Shelf::eq`](eq.md) / [`ne`](ne.md) / [`lt`](lt.md) / [`le`](le.md) / [`gt`](gt.md) /
  [`ge`](ge.md): the fixed-operator convenience methods `compare()` is generalized from.
- [`Shelf::between`](between.md): interval membership — two comparisons combined with a logical
  AND, in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clip.html">
    <span class="label">Previous</span>
    <span class="title">clip</span>
  </a>
  <a class="next-link" href="corr.html">
    <span class="label">Next</span>
    <span class="title">corr</span>
  </a>
</div>
