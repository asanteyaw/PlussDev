<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6f4bd5f41650 -->

# Shelf::put

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::put(const Shelf& bool_mask, const Shelf& value) const
Shelf Shelf::put(const Shelf& bool_mask, double value) const
Shelf Shelf::put(const Shelf& bool_mask, std::string_view value) const
Shelf Shelf::put(const Shelf& bool_mask, const char* value) const
Shelf Shelf::put(const Shelf& bool_mask, pluss::null_t value) const
Shelf Shelf::put(const pluss::Series& bool_mask, const Shelf& value) const
Shelf Shelf::put(const pluss::Series& bool_mask, double value) const
Shelf Shelf::put(const pluss::Series& bool_mask, std::string_view value) const
Shelf Shelf::put(const pluss::Series& bool_mask, const char* value) const
Shelf Shelf::put(const pluss::Series& bool_mask, pluss::null_t value) const
```

## What It Does

Writes a replacement value into every row where `bool_mask` is `true`, leaving every other
row exactly as it was — a masked, non-mutating write (`put()` returns a new `Shelf`;
`*this` is untouched). The `pluss::Series` overloads are pure convenience wrappers that
just call `Shelf(bool_mask)` and forward to the `Shelf`-mask overload.

Internally, `put(mask, value)` is a thin wrapper over
[`Shelf::select`](select.md) — it's exactly
`Shelf::select({mask}, {value}, *this)`: wherever the mask is true, use `value`;
everywhere else, fall back to the original shelf. Understanding `put()` this way explains
its broadcast rule for free: a scalar `value` (numeric, text, or a size-1 `Shelf`)
broadcasts to every masked row; a same-length `Shelf` value supplies one replacement per
row instead.

```{important} put(mask, pluss::series::null) takes a different code path than every other overload
Unlike the value-writing overloads, `put(mask, pluss::series::null)` doesn't go through `select()`
at all — it directly nulls out the masked rows' validity mask, keeping their underlying
stored values untouched (just marked invalid). This is the masked equivalent of
[`fillnull`](fillnull.md) run in reverse — see Example 4.
```

## Parameters

**bool_mask** (`const Shelf&` or `const pluss::Series&`)

A boolean row mask, the same length as `*this`. `true` marks a row for replacement;
`false` leaves it untouched. The `Series` overloads are equivalent to wrapping the mask
in `Shelf(bool_mask)` first.

**value** (`const Shelf&` / `double` / `std::string_view` / `const char*` / `pluss::null_t`)

The replacement written into every masked row. A scalar (`double`/text/`pluss::series::null`)
or a size-1 `Shelf` broadcasts to every masked row; a `Shelf` matching `*this`'s
length supplies one replacement value per row instead.

## Returns

`Shelf`
    A new shelf the same length as `*this`, with masked rows replaced and every other row
    unchanged.

## Examples

Example 1. Replace every row above a threshold with a fixed scalar.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = scores.compare(15.0, ShelfBinaryCompareOp::Gt);

auto out = scores.put(mask, 0.0);
std::cout << out;
```

### Output

```text
0  10
1  0
2  0
ptype: Series
```

Example 2. Replace masked rows with a **rowwise** `Shelf` of replacement values — row `i`
of `value` supplies row `i`'s replacement, rather than one value broadcasting everywhere.

```cpp
auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = scores.compare(15.0, ShelfBinaryCompareOp::Gt);
auto replacements = Shelf(pluss::make_series({-1.0, -2.0, -3.0}));

auto out = scores.put(mask, replacements);
std::cout << out;
```

### Output

```text
0  10
1  -2
2  -3
ptype: Series
```

Example 3. Replace masked rows in a `Note`-kind shelf with a text scalar — useful for
redaction-style rewrites.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "Bola", "Chen"}));
auto mask = names.compare("Bola", ShelfBinaryCompareOp::Eq);

auto out = names.put(mask, "REDACTED");
std::cout << out;
```

### Output

```text
0  Ada
1  REDACTED
2  Chen
ptype: Note
```

Example 4. `put(mask, pluss::series::null)` nulls out masked rows instead of writing a value —
see the important callout above.

```cpp
auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = scores.compare(15.0, ShelfBinaryCompareOp::Gt);

auto out = scores.put(mask, pluss::series::null);
std::cout << out;
```

### Output

```text
0  10
1  NULL
2  NULL
ptype: Series
```

## Notes

- Non-mutating — `put()` returns a new `Shelf` and leaves `*this` unchanged; reassign the
  result (`shelf = shelf.put(...)`) if you want the effect to stick.
- `put(mask, value)` is exactly `Shelf::select({mask}, {value}, *this)` — see
  [`Shelf::select`](select.md) for the general multi-condition form this specializes.
- `put(mask, pluss::series::null)` is the one overload that doesn't go through `select()` — it nulls
  the validity mask directly rather than writing a value.
- The `pluss::Series`-mask overloads exist purely for convenience; they're identical to
  wrapping the mask in `Shelf(...)` first.

## See Also

- [`Shelf::select`](select.md): the general multi-branch select construct `put()`
  specializes — multiple ordered conditions/choices instead of one mask/value pair.
- [`Shelf::fillnull`](fillnull.md): the inverse direction — replace `NULL` rows with a
  value, rather than nulling out selected rows.
- [`Shelf::replace`](replace.md): replace by *value* match instead of by boolean mask.
- [`Shelf::compare`](compare.md): the usual way to build the `bool_mask` argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptype.html">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="radd.html">
    <span class="label">Next</span>
    <span class="title">radd</span>
  </a>
</div>
