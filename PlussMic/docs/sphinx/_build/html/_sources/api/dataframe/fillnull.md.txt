<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c7d49206cbc1 -->

# DataFrame::fillnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::fillnull(double value) const -> DataFrame
DataFrame::fillnull(double value, pluss::RangeView<std::string_view> features) const -> DataFrame
template <detail::StringViewNameRange NameRange>
DataFrame::fillnull(double value, const NameRange& features) const -> DataFrame

DataFrame::fillnull(std::string_view value) const -> DataFrame
DataFrame::fillnull(std::string_view value, pluss::RangeView<std::string_view> features) const -> DataFrame
template <detail::StringViewNameRange NameRange>
DataFrame::fillnull(std::string_view value, const NameRange& features) const -> DataFrame
DataFrame::fillnull(const char* value) const -> DataFrame
DataFrame::fillnull(const char* value, pluss::RangeView<std::string_view> features) const -> DataFrame
template <detail::StringViewNameRange NameRange>
DataFrame::fillnull(const char* value, const NameRange& features) const -> DataFrame

DataFrame::fillnull(const pluss::Timeline& value) const -> DataFrame
DataFrame::fillnull(const pluss::Timeline& value, pluss::RangeView<std::string_view> features) const -> DataFrame
template <detail::StringViewNameRange NameRange>
DataFrame::fillnull(const pluss::Timeline& value, const NameRange& features) const -> DataFrame

DataFrame::fillnull(const pluss::TimeIndex& value) const -> DataFrame
DataFrame::fillnull(const pluss::TimeIndex& value, pluss::RangeView<std::string_view> features) const -> DataFrame
template <detail::StringViewNameRange NameRange>
DataFrame::fillnull(const pluss::TimeIndex& value, const NameRange& features) const -> DataFrame

DataFrame::fillnull(pluss::RangeView<FillSpec> specs) const -> DataFrame
DataFrame::fillnull(std::initializer_list<FillSpec> specs) const -> DataFrame
```

## What It Does

Returns a copy of the frame with missing values replaced by a fill value, and is the
main non-mutating entry point in the missing-data cleanup path — typically applied right
before a summary (`describe()`, `corr()`), a grouped `agg()` workflow, or an export (`save_csv`,
`save_excel`), all of which either skip nulls silently or fail outright depending on the
destination format.

The overload set exists because "the right fill value" depends on the column's dtype:
a `double` fills numeric `Series` columns, a `string_view`/`const char*` fills `Note`
columns, and a `Timeline` / `TimeIndex` fills the matching temporal column type. Passing
a scalar fills **every feature whose dtype matches that scalar's type**, unless you
narrow the target with the `features` overload. When different columns need different
fill values or you want to mix dtypes in a single call, use the `FillSpec` overload
instead of chaining several single-value calls.

`fillnull()` is non-mutating — it returns a new frame and leaves the original untouched.
For an in-place variant, see [`fillnull_()`](fillnull_.md) (note the trailing
underscore, the project-wide convention for mutating siblings of a non-mutating method).

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` /
`const pluss::TimeIndex&`)

The replacement value. Its type determines which columns are eligible: a `double` only
ever fills numeric `Series` features, a string only fills `Note` features, and so on.
Columns whose dtype does not match `value`'s type are left completely unchanged, nulls
included — `fillnull(0.0)` will never touch a `Note` column, even one that has missing
entries.

**features** (`pluss::RangeView<std::string_view>`, optional)

Restricts the fill to an explicit subset of feature names. Omit this parameter (the
single-argument overloads) to apply the fill to every eligible feature on the frame.
Passing a feature name whose dtype does not match `value`'s type is not an error — that
feature is simply left unaffected, same as if it had been excluded. Any container of
`std::string_view`-convertible names (`std::vector<std::string>`, `std::set<std::string>`,
and similar) works too, via a generic template overload — not just a braced list or an
explicit `RangeView`.

**specs** (`pluss::RangeView<FillSpec>` / `std::initializer_list<FillSpec>`)

A collection of per-feature fill instructions, each pairing a feature name with its own
fill value. Use this overload when different columns need different replacement values
in one call, for example filling a numeric column with `0.0` while filling a sibling
`Note` column with `"Unknown"` in the same expression.

## Returns

`DataFrame`
    A new frame with the requested nulls replaced. Feature order, names, and dtypes are
    unchanged; only cell values move from null to the supplied fill value.

## Examples

Example 1. Fill every missing numeric value on the frame with `0.0`.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto filled = orders.fillnull(0.0);
std::cout << filled;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
0    | 101       Accra   Laptop      2      1200        0.1     
1    | 102       Accra   Headphones  5      45.5        0.05    
2    | 103       Tema    Laptop      1      1150        0       
3    | 104       Tema    Monitor     3      320         0.08    
4    | 105       Kumasi  Headphones  4      42          0.05    
5    | 106       Kumasi  Monitor     2      0           0.12    
----------------------------------------------------------------
[6 cs x 6 feat]
```

Both nulls in the frame were numeric (`unit_price` at case `5`, `discount` at case `2`),
and both were replaced with `0.0`. `order_id` and `units` had no missing values to begin
with, so they are identical to the source frame.

Example 2. Restrict the fill to a single feature, leaving other missing values as-is.

```cpp
auto filled_discount_only = orders.fillnull(0.0, {"discount"});
std::cout << filled_discount_only;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
0    | 101       Accra   Laptop      2      1200        0.1     
1    | 102       Accra   Headphones  5      45.5        0.05    
2    | 103       Tema    Laptop      1      1150        0       
3    | 104       Tema    Monitor     3      320         0.08    
4    | 105       Kumasi  Headphones  4      42          0.05    
5    | 106       Kumasi  Monitor     2      NULL        0.12    
----------------------------------------------------------------
[6 cs x 6 feat]
```

Only `discount` (case `2`) was filled. `unit_price`'s missing value at case `5` is still
`NULL`, because `"discount"` was the only feature named in the `features` argument.

Example 3. Fill a `Note` column's missing entries with a placeholder string.

```cpp
auto validity = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto cities = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "", "Kumasi"}), validity)},
});

auto filled = cities.fillnull("Unknown");
```

`fillnull("Unknown")` only ever considers `Note` features — passing a `const char*` or
`string_view` never touches `unit_price` or `discount` above, even in the same call,
because their dtype does not match the fill value's type.

## Notes

- Scalar-typed overloads (`double`, string, `Timeline`, `TimeIndex`) only ever fill
  columns whose dtype matches the scalar's type. This is the most common source of
  "why didn't my fill do anything?" confusion — check that the column you expected to
  change actually has the same logical dtype as the value you passed.
- Use the `FillSpec` overload when a single call needs to mix fill values across
  differently-typed columns, instead of chaining `fillnull(0.0).fillnull("Unknown")`
  (which also works, but rebuilds the frame twice).
- `fillnull()` never infers a fill value from the data itself (no mean/median/mode
  imputation). For forward/backward propagation from neighboring rows instead of a fixed
  scalar, see [`ffill()`](ffill.md) and [`bfill()`](bfill.md).

## See Also

- [`DataFrame::fillnull_`](fillnull_.md): the in-place, mutating counterpart.
- [`DataFrame::ffill`](ffill.md) / [`DataFrame::bfill`](bfill.md): fill nulls by carrying
  the previous/next observed value forward instead of a fixed scalar.
- [`DataFrame::isnull`](isnull.md) / [`DataFrame::notnull`](notnull.md): inspect which
  cells are missing before deciding how to fill them.
- [`DataFrame::dropnull`](dropnull.md): remove rows with missing values instead of
  filling them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="fillnull_.html">
    <span class="label">Next</span>
    <span class="title">fillnull_</span>
  </a>
</div>
