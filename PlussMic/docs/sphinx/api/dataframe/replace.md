<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4709cc40a0b9 -->

# DataFrame::replace

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::replace(double from, double to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(double from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, double to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame

DataFrame::replace(std::string_view from, std::string_view to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(std::string_view from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, std::string_view to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(const char* from, const char* to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(const char* from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, const char* to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame

DataFrame::replace(const pluss::Series& from, const pluss::Series& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(const pluss::Series& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, const pluss::Series& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame

DataFrame::replace(const pluss::Note& from, const pluss::Note& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(const pluss::Note& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, const pluss::Note& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame

DataFrame::replace(const pluss::Timeline& from, const pluss::Timeline& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(const pluss::Timeline& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, const pluss::Timeline& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame

DataFrame::replace(const pluss::Duration& from, const pluss::Duration& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(const pluss::Duration& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::replace(pluss::null_t from, const pluss::Duration& to, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
```

Every overload above except the two `const char*` overloads also has a template sibling
that swaps `pluss::RangeView<std::string_view> features` for `const NameRange& features`
(`template <detail::StringViewNameRange NameRange>`), accepting any container of
`std::string_view`-convertible names — a `std::vector<std::string>`, a
`std::set<std::string>`, and so on — directly, not just a braced list or an explicit
`RangeView`. These sibling overloads are omitted from the list above to keep it
readable; they do not change dispatch or behavior, only which container type `features`
accepts.

## What It Does

Finds a specific value (`from`) across compatible features and rewrites every matching
cell to a replacement value (`to`), returning the updated frame. The long overload list
looks intimidating, but it collapses into one rule: **the type of `from` decides which
feature kind participates**. Pass a `double` and only numeric `Series` features are
searched; pass a string and only `Note` features are searched; pass a `Timeline` and
only `Timeline` features are searched; pass a `Duration` and only `Duration` features
are searched. Features of any other kind are left completely alone, the same way
[`fillnull()`](fillnull.md) and [`astype()`](astype.md) restrict themselves by dtype.

The other half of the rule concerns `pluss::series::null` (of type `pluss::null_t`), the
project's null-sentinel constant: pass it as `to` to turn matched cells into missing
values instead of a replacement value, or pass it as `from` to replace **existing**
nulls with a concrete value (`replace(pluss::series::null, 0.0)` is a targeted, single-value
alternative to a broader [`fillnull(0.0)`](fillnull.md) call).

## Parameters

**from** (`double` / `std::string_view` / `const char*` / `const pluss::Series&` /
`const pluss::Note&` / `const pluss::Timeline&` / `const pluss::Duration&` /
`pluss::null_t`)

The value to search for. Its type selects which feature kind participates in the search;
`pluss::series::null` searches for existing missing values instead of a concrete value.

**to** (same family of types as `from`, or `pluss::null_t`)

The replacement value written wherever `from` matches. Pass `pluss::series::null` to turn
matches into missing values instead of a concrete replacement.

**features** (`pluss::RangeView<std::string_view>`, default empty)

Restricts the search to an explicit feature subset. Leave empty to search every
compatible feature (every feature whose kind matches `from`'s type).

## Returns

`DataFrame`
    A new frame with every matching cell rewritten to `to`. Features that don't match
    `from`'s type, and cells within compatible features that don't match `from`'s value,
    are left exactly as they were.

## Examples

Example 1. Replace a specific text value in one feature.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",     Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
});

auto out = orders.replace("Kumasi", "Kumasi City");
std::cout << out;
```

### Output

```text
============================
     | order_id  city       
----------------------------
0    | 101       Accra      
1    | 102       Accra      
2    | 103       Tema       
3    | 104       Tema       
4    | 105       Kumasi City
5    | 106       Kumasi City
----------------------------
[6 cs x 2 feat]
```

`replace("Kumasi", "Kumasi City")` only ever considers `Note` features (since `from` is a
string), so `order_id` was never a candidate no matter what value it held.

Example 2. Turn a specific sentinel value into a genuine null.

```cpp
auto orders_with_price = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 42.0, 1150.0, 320.0}))},
});

// Suppose 42.0 was actually a placeholder for "price not yet set" upstream.
auto out = orders_with_price.replace(42.0, pluss::series::null);
std::cout << out;
```

### Output

```text
===========================
     | order_id  unit_price
---------------------------
0    | 101       1200      
1    | 102       NULL      
2    | 103       1150      
3    | 104       320       
---------------------------
[4 cs x 2 feat]
```

Case `1`'s `unit_price` was exactly `42.0`, so it becomes `NULL` — useful when an
upstream system used a magic-number sentinel instead of a genuine missing-value marker,
and you want to normalize it into PlussMic's null representation before running
`describe()`, `fillnull()`, or any other null-aware method. Every other `unit_price`
value, none of which equal `42.0`, is left untouched.

Example 3. Replace existing nulls with a concrete fallback in one named column.

```cpp
auto out2 = orders_with_price.replace(42.0, pluss::series::null).replace(pluss::series::null, 0.0, {"unit_price"});
std::cout << out2;
```

### Output

```text
===========================
     | order_id  unit_price
---------------------------
0    | 101       1200      
1    | 102       0         
2    | 103       1150      
3    | 104       320       
---------------------------
[4 cs x 2 feat]
```

This is the mirror image of Example 2, and functionally similar to
`fillnull(0.0, {"unit_price"})` — `replace(pluss::series::null, ...)` is most useful when you
want the "search for a value" framing to read consistently alongside other
`replace(...)` calls in the same block of code, rather than switching vocabulary to
`fillnull` mid-pipeline.

Example 4. Replace several distinct values in one call using the `Series`-range overload.

```cpp
auto ids = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 42.0, 1150.0, 320.0}))},
});

// Renumber two specific order IDs in one call instead of two chained replace() calls.
auto renumbered = ids.replace(
    pluss::make_series({101, 103}),
    pluss::make_series({9101, 9103}),
    {"order_id"});
std::cout << renumbered;
```

### Output

```text
===========================
     | order_id  unit_price
---------------------------
0    | 9101      1200      
1    | 102       42        
2    | 9103      1150      
3    | 104       320       
---------------------------
[4 cs x 2 feat]
```

The two `pluss::Series` arguments are matched up positionally: every occurrence of
`from[i]` becomes `to[i]`, for every `i`, in a single pass over the frame. `order_id`
`101` and `103` are renumbered to `9101` and `9103` respectively; `102` and `104` never
appear in `from`, so they pass through unchanged. This is the overload to reach for when
you have a whole lookup table of value substitutions rather than a single `from`/`to`
pair — build the two `Series` from a mapping and pass them straight through, instead of
writing one `replace()` call per pair.

## Notes

- `from`'s type is the whole dispatch rule: a `double` never touches `Note` features, a
  string never touches `Series` features, and so on, regardless of what `features` you
  pass.
- Passing `features` narrows the search further, but never widens it beyond what `from`'s
  type already allows.
- `replace()` matches values exactly. It has no tolerance, rounding, or pattern-matching
  behavior for numeric or string values — use [`query()`](query.md) or a mask built with
  [`filter()`](filter.md) for anything more elaborate than an exact-value swap.
- `replace()` is non-mutating; it always returns a new frame.

## See Also

- [`DataFrame::fillnull`](fillnull.md): broader, dtype-driven null replacement, when you
  are not targeting one specific sentinel value.
- [`DataFrame::mask`](mask.md): replace values based on a boolean condition instead of an
  exact match.
- [`DataFrame::astype`](astype.md), [`DataFrame::cast`](cast.md): change a feature's
  dtype rather than individual values within it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeated.html">
    <span class="label">Previous</span>
    <span class="title">repeated</span>
  </a>
  <a class="next-link" href="reshuffle.html">
    <span class="label">Next</span>
    <span class="title">reshuffle</span>
  </a>
</div>
