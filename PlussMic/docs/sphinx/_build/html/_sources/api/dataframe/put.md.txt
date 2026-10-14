<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b84f0fb430a0 -->

# DataFrame::put

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::put(const Slate& bool_mask, const Slate& value) const -> DataFrame
DataFrame::put(const Slate& bool_mask, const Slate& value, const AlignOptions& options) const -> DataFrame
DataFrame::put(const Slate& bool_mask, double value) const -> DataFrame
DataFrame::put(const Slate& bool_mask, double value, const AlignOptions& options) const -> DataFrame
DataFrame::put(const Slate& bool_mask, std::string_view value) const -> DataFrame
DataFrame::put(const Slate& bool_mask, std::string_view value, const AlignOptions& options) const -> DataFrame
DataFrame::put(const Slate& bool_mask, const char* value) const -> DataFrame
DataFrame::put(const Slate& bool_mask, const char* value, const AlignOptions& options) const -> DataFrame
DataFrame::put(const Slate& bool_mask, pluss::null_t value) const -> DataFrame
DataFrame::put(const Slate& bool_mask, pluss::null_t value, const AlignOptions& options) const -> DataFrame
// each Slate-mask overload above also has a pluss::Series-mask convenience:
DataFrame::put(const pluss::Series& bool_mask, /* value as above */) const -> DataFrame
```

## What It Does

Rewrites values across **every** feature on the frame wherever `bool_mask` is `true`,
leaving every other row untouched — the whole-frame, mask-driven sibling of
[`mask()`](mask.md) (which nulls out cells rather than replacing them with a chosen
value). It applies independently to each column, via that column's own
[`Slate::put()`](../slate/put.md): where `value` is itself a `Slate` rather than a
scalar, each masked row is replaced by **that same row's own value** in `value` — not
one shared replacement broadcast to every masked cell.

`put()` carries no feature selector of its own: it writes `value` into every column on
the frame it's called on, which only makes sense if `value`'s kind is compatible with
every one of those columns (a `double` value into every numeric column, a string value
into every `Note` column, and so on) — a `double` reaching a `Note` column throws.
Narrow to the feature(s) that share a compatible kind first with
[`features()`](features.md) — `orders.features({"sales", "cost"}).put(mask, 0.0)` —
and recombine with [`update()`](update.md) afterward if the untouched features need to
survive alongside the result. For a single named feature you want to mutate in place,
[`put_()`](put_.md) is a separate, still-public sibling that keeps its own `feature`
parameter directly — reach for that instead of narrowing.

`put()` returns a new frame; [`put_()`](put_.md) is the in-place counterpart that
mutates `*this` directly instead.

## Parameters

**bool_mask** (`const Slate&` or `const pluss::Series&`)

A boolean row mask. `true` rewrites the corresponding row on every column; `false`
leaves that row's existing values unchanged. A masked-missing mask entry behaves like
`false`.

**value** (`const Slate&`, `double`, `std::string_view`, `const char*`, or
`pluss::null_t`)

The replacement, written into every column on the frame. A scalar (`double`, string,
or `pluss::series::null`) is broadcast to every masked row identically. A full `Slate`
instead supplies a **different** replacement value per row — row `i` of `value`
replaces row `i` of each column, wherever `bool_mask` is `true` at row `i`.

**options** (`const AlignOptions&`, optional)

Controls how `bool_mask` is aligned to the frame's case axis when it doesn't already
match exactly, the same alignment mechanism used by [`filter()`](filter.md) and
[`mask()`](mask.md).

## Returns

`DataFrame`
    A new frame, same shape as the (possibly `features()`-narrowed) source, with every
    column's masked rows replaced by `value`.

## Examples

Example 1. Narrow to one numeric feature, replace a scalar value at the masked row,
and merge the result back into the full frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({1, 2, 3}))},
    {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto is_middle = orders.features("id") == 2.0;
auto sales_only = orders.features({"sales"}).put(is_middle, 999.0);
auto corrected = orders.update(sales_only);
std::cout << corrected;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     999    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

Only case `1` (where `id == 2`) is rewritten; cases `0` and `2` keep their original
`sales` values untouched. Narrowing to `{"sales"}` first keeps `put()`'s scalar
`double` compatible with every column it touches — calling it on the unnarrowed
`orders` directly would fail once it reached the `Note`-kind `city` column.

Example 2. Replace with a per-row `Slate` instead of one shared scalar, across two
compatible numeric features at once.

```cpp
auto mask = Slate(pluss::make_series({true, false, true}));
auto replacements = Slate(pluss::make_series({500.0, 999.0, 700.0}));
auto both = orders.features({"id", "sales"}).put(mask, replacements);
std::cout << both;
```

### Output

```text
====================
     | id    sales 
--------------------
0    | 500   500   
1    | 2     120   
2    | 700   700   
--------------------
[3 cs x 2 feat]
```

`mask` is `true` at rows `0` and `2`; both narrowed columns pick up `replacements`'
own values at those same positions on every masked row. Row `1` is unmasked, so both
columns keep their original values there — `replacements`' row-`1` value (`999`) is
never used, since a `Slate` replacement supplies per-row values rather than a single
broadcast value.

Example 3. Replace with `pluss::series::null` to mark matched cells as missing.

```cpp
auto out = orders.features({"sales"}).put(is_middle, pluss::series::null);
std::cout << orders.update(out);
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     NULL   Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

## Notes

- A scalar `value` (`double`, string, or `pluss::series::null`) is the same replacement for
  every masked row; a `Slate` `value` supplies a different replacement per row,
  taken from that row's own position in `value`.
- `bool_mask` may be a `Slate` or a `pluss::Series` — the `Series` overloads are pure
  convenience wrappers with identical behavior.
- `put()` never changes a column's kind: a scalar `double`/`pluss::series::null` value only
  works against numeric (`Series`-kind) columns, and a string/`pluss::series::null` value only
  works against `Note`-kind columns. Calling `put()` on a frame that mixes kinds throws
  as soon as it reaches an incompatible column — narrow with
  [`features()`](features.md) first to a kind-compatible subset.
- For a single named feature, prefer [`put_()`](put_.md) (still public with its own
  `feature` parameter) over narrowing — it's both simpler and mutating.

## See Also

- [`DataFrame::put_`](put_.md): the single-feature, in-place counterpart — still takes
  a `feature` name directly, mutating `*this` instead of returning a new frame.
- [`DataFrame::mask`](mask.md): nulls out masked cells outright, rather than
  replacing them with a chosen value.
- [`DataFrame::filter`](filter.md): drops whole rows by a boolean mask, instead of
  rewriting values within surviving rows.
- [`DataFrame::features`](features.md), [`DataFrame::update`](update.md): narrow to a
  kind-compatible subset before calling `put()`, then recombine the result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptypes.html">
    <span class="label">Previous</span>
    <span class="title">ptypes</span>
  </a>
  <a class="next-link" href="put_.html">
    <span class="label">Next</span>
    <span class="title">put_</span>
  </a>
</div>
