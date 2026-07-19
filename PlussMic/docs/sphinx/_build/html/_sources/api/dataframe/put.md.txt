<!-- pluss-hand-authored -->

# DataFrame::put

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::put(std::string_view feature, const Thread& bool_mask, const Thread& value) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, const Thread& value, const AlignOptions& options) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, double value) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, double value, const AlignOptions& options) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, std::string_view value) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, std::string_view value, const AlignOptions& options) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, const char* value) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, const char* value, const AlignOptions& options) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, pluss::null_t value) const -> DataFrame
DataFrame::put(std::string_view feature, const Thread& bool_mask, pluss::null_t value, const AlignOptions& options) const -> DataFrame
// each Thread-mask overload above also has a pluss::Series-mask convenience:
DataFrame::put(std::string_view feature, const pluss::Series& bool_mask, /* value as above */) const -> DataFrame
```

## What It Does

Rewrites one feature's values only at the rows where `bool_mask` is `true`, leaving
every other row's value on that feature untouched — the single-feature, mask-driven
sibling of [`mask()`](mask.md) (which nulls out cells rather than replacing them with
a chosen value). Where `value` is itself a `Thread` rather than a scalar, each masked
row is replaced by **that same row's own value** in `value` — not one shared
replacement broadcast to every masked cell. `put()` returns a new frame;
[`put_()`](put_.md) is the in-place counterpart that mutates `*this` directly instead.

Every value-type overload above also has a `pluss::Series`-mask convenience
(`put(feature, series_mask, value)`) that just wraps the mask in a `Thread` and
forwards to the `Thread`-mask overload — the two mask types behave identically.

## Parameters

**feature** (`std::string_view`)

The feature to rewrite. Must already exist on the frame.

**bool_mask** (`const Thread&` or `const pluss::Series&`)

A boolean row mask. `true` rewrites the corresponding row on `feature`; `false`
leaves that row's existing value unchanged. A masked-missing mask entry behaves like
`false`.

**value** (`const Thread&`, `double`, `std::string_view`, `const char*`, or
`pluss::null_t`)

The replacement. A scalar (`double`, string, or `pluss::null`) is broadcast to every
masked row identically. A full `Thread` instead supplies a **different** replacement
value per row — row `i` of `value` replaces row `i` of `feature`, wherever
`bool_mask` is `true` at row `i`.

**options** (`const AlignOptions&`, optional)

Controls how `bool_mask` is aligned to the frame's case axis when it doesn't already
match exactly, the same alignment mechanism used by [`filter()`](filter.md) and
[`mask()`](mask.md).

## Returns

`DataFrame`
    A new frame identical to the source, except `feature`'s masked rows have been
    replaced by `value`.

## Examples

Example 1. Replace a scalar value at the masked row.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto is_middle = orders.features("id") == 2.0;
auto corrected = orders.put("sales", is_middle, 999.0);
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
`sales` values untouched.

Example 2. Replace with a per-row `Thread` instead of one shared scalar.

```cpp
auto mask = Thread(pluss::make_series({true, false, true}));
auto replacements = Thread(pluss::make_series({500.0, 999.0, 700.0}));
auto out = orders.put("sales", mask, replacements);
std::cout << out;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     500    Accra 
1    | 2     120    Tema  
2    | 3     700    Kumasi
--------------------------
[3 cs x 3 feat]
```

`mask` is `true` at rows `0` and `2`; those rows pick up `replacements`' own values at
those same positions (`500` and `700`). Row `1` is unmasked, so it keeps its original
`sales` value (`120`) — `replacements`' row-`1` value (`999`) is never used, since a
`Thread` replacement supplies per-row values rather than a single broadcast value.

Example 3. Replace with `pluss::null` to mark matched cells as missing.

```cpp
auto out = orders.put("sales", is_middle, pluss::null);
std::cout << out;
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

- A scalar `value` (`double`, string, or `pluss::null`) is the same replacement for
  every masked row; a `Thread` `value` supplies a different replacement per row,
  taken from that row's own position in `value`.
- `bool_mask` may be a `Thread` or a `pluss::Series` — the `Series` overloads are pure
  convenience wrappers with identical behavior.
- `put()` never changes `feature`'s kind; replacing a `Note` feature requires a
  string (or null) `value`, and replacing a numeric `Series` feature requires a
  `double` (or null) `value`.

## See Also

- [`DataFrame::put_`](put_.md): the in-place counterpart, mutating `*this` instead of
  returning a new frame.
- [`DataFrame::mask`](mask.md): nulls out masked cells outright, rather than
  replacing them with a chosen value.
- [`DataFrame::filter`](filter.md): drops whole rows by a boolean mask, instead of
  rewriting one feature's values within surviving rows.

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
