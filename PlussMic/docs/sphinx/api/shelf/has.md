<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9150e09ac0a7 -->

# Shelf::has

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::has(double value) const
Shelf Shelf::has(std::string_view value) const
Shelf Shelf::has(const char* value) const
Shelf Shelf::has(const Shelf& values) const
```

## What It Does

A row-wise membership test — despite the name suggesting a single "does this shelf
contain X anywhere" answer, `has()` returns one boolean **per row**, not one boolean for
the whole shelf.

The scalar overloads (`double`, `std::string_view`, `const char*`) are a direct alias for
[`eq()`](eq.md): `has(value)` is exactly `eq(value)`, testing each row against the one
value. The `Shelf`-argument overload is the more useful form in practice — it's an `isin`
style test: row `i` is `true` if it equals **any** value in `values`, checked by ORing
together one `has(scalar)` call per (valid) entry of `values`.

```{important} has(const Shelf&) is per-row "is this value in the candidate set", not a global "does this shelf contain any of these"
`shelf.has(candidates)` returns a `Shelf` the same length as `shelf` — row `i` tells you
whether `shelf`'s row `i` matches one of `candidates`' values, not whether `candidates`
appears anywhere in `shelf` as a whole. This is the `isin`/membership pattern common to
most table libraries. Invalid rows in `candidates` are skipped (never contribute a match).
```

## Parameters

**value** (`double` / `std::string_view` / `const char*`)

A single scalar to test every row against — equivalent to [`eq(value)`](eq.md).

**values** (`const Shelf&`)

A set of candidate values (typically a short, `Note`- or `Series`-kind shelf) — each
row of `*this` is tested against every entry of `values`, matching if it equals *any*
of them.

## Returns

`Shelf`
    A `Series`-kind shelf of `Bool` values, one membership result per row.

## Examples

Example 1. Test a numeric shelf against one scalar — identical to `eq(20.0)`.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.has(20.0);
std::cout << out;
```

### Output

```text
0  false
1  true
2  false
ptype: Series
```

Example 2. Test a `Note`-backed shelf against one text scalar.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "Bola", "Chen"}));

auto out = names.has("Bola");
std::cout << out;
```

### Output

```text
0  false
1  true
2  false
ptype: Series
```

Example 3. `isin`-style membership against a short candidate set of numeric values.

```cpp
auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto candidates = Shelf(pluss::make_series({20.0, 40.0}));

auto out = scores.has(candidates);
std::cout << out;
```

### Output

```text
0  false
1  true
2  false
3  true
ptype: Series
```

Example 4. The same `isin`-style membership test, with a `Note`-backed candidate set —
useful for allowlist/denylist-style row filtering.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "Bola", "Chen", "Ada"}));
auto allowed = Shelf(pluss::make_note({"Ada", "Chen"}));

auto out = names.has(allowed);
std::cout << out;
```

### Output

```text
0  true
1  false
2  true
3  true
ptype: Series
```

Example 5. Mismatched payload kinds compare by string form rather than throwing — a
`Note`-backed shelf tested against a numeric candidate set simply finds no matches
(here the candidates are the *counts* from a [`freq()`](freq.md) result, a common
mistake — the distinct values live on the label axis, reachable via
[`label().values()`](label.md)).

```cpp
auto ids = Shelf(pluss::make_note({"H-104", "H-887", "H-104", "H-231"}));
auto counts = ids.freq();               // values: 2, 1, 1 ; case labels: H-104, H-887, H-231

auto wrong = ids.has(counts);           // tests "H-104" against 2, 1, 1 -> all false
std::cout << wrong;

auto right = ids.has(counts.label().values());  // tests against H-104, H-887, H-231
std::cout << right;
```

### Output

```text
0  false
1  false
2  false
3  false
ptype: Series
0  true
1  true
2  true
3  true
ptype: Series
```

```text
0  true
1  true
2  true
3  true
ptype: Series
```

## Notes

- The scalar overloads are a pure alias for [`eq()`](eq.md) — prefer whichever name reads
  better at the call site.
- The numeric fast path in `has(const Shelf&)` is only taken when **both** `*this` and
  the candidate set are numeric `Series`. Any other kind combination compares by string
  form (matching [`freq()`](freq.md)/[`distinct()`](distinct.md)/[`duplicated()`](duplicated.md)),
  so `note_shelf.has(numeric_shelf)` yields all-`false` instead of raising
  `"Shelf::eq(double): operation requires a Series-backed Shelf"`.
- The `Shelf`-argument overload is the genuinely distinct behavior: a per-row `isin` test
  against a candidate set, not a whole-shelf containment check.
- Invalid rows in the `values` candidate set are skipped and never produce a match; an
  invalid row in `*this` is compared normally against each candidate (matching
  [`compare()`](compare.md)'s general rule that invalid rows just compare `false`).

## See Also

- [`Shelf::eq`](eq.md): the scalar-comparison method `has(scalar)` is an alias for.
- [`Shelf::compare`](compare.md): the general comparison surface underlying `eq()`.
- [`DataFrame::filter`](../dataframe/filter.md): use `has()`'s boolean result as a row mask.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
