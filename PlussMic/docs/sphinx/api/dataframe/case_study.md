# DataFrame::case_study

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::case_study(const CasePick& pick) -> DataFrame
DataFrame::case_study(const CaseSelector& rows) -> DataFrame
DataFrame::case_study(const CaseSelector& rows, const FeatureSelector& features) -> DataFrame
```

## Parameters

**pick**
    Structured-case selector used when the DataFrame has named case levels created by `set_cases(...)`.

    Use this overload for hierarchical case lookup rather than ordinary row slicing.

    Supported forms:
    - `.level = "region", .value = "West"` selects every row whose named case level equals the supplied value.
    - `.levels = {"region", "quarter"}, .values = {"West", "Q2"}` matches several named case levels at once.
    - `.key = {"East", "201", "Q2"}` performs a full structured-key lookup using the current case-level order.
    - `.drop = true` removes the matched case level(s) from the returned structured case metadata after selection.

**rows**
    Row selector used for ordinary case-axis lookup and positional indexing.

    Supported forms:
    - `pluss::table::pos(row)` selects one row by zero-based position. Negative positions count from the end.
    - `pluss::table::pos(start, stop)` selects a half-open positional slice `[start, stop)`.
    - `pluss::table::pos({r1, r2, ...})` selects explicit row positions in the exact output order you request.
    - `pluss::table::label(value)` selects every row whose current case label equals `value`.
    - `pluss::table::labels({...})` selects several labels in the requested order. Duplicate labels expand to all matching rows.
    - `pluss::table::label_range(start, stop)` selects an inclusive case-label window from the first `start` match through the last `stop` match.
    - `pluss::table::mask(bool_thread)` selects rows using a boolean thread with one value per case.

**features**
    Optional feature selector applied after the row selection has been resolved.

    Supported forms:
    - `pluss::table::all_features()` keeps every feature in source order.
    - `pluss::table::col("name")` keeps one named feature.
    - `pluss::table::col(position)` keeps one feature by zero-based position.
    - `pluss::table::cols({"a", "b"})` keeps named features in the requested order.
    - `pluss::table::cols({0, 2})` keeps feature positions in the requested order.
    - `pluss::table::cols(start, stop)` keeps a half-open positional feature slice `[start, stop)`.
    - `pluss::table::col_range("start", "stop")` keeps an inclusive feature-name window from `start` through `stop`.

## Returns

`DataFrame`
    Always returns a `DataFrame`, even when the selection resolves to one row, one column, or one cell.

## What It Does

`case_study(...)` is the unified DataFrame indexing surface. It handles ordinary positional row access, case-label lookup, boolean-mask selection, feature projection, and structured multi-level case lookup through `CasePick` when the frame has hierarchical cases.

## Selector Rules

- Positional row selectors use zero-based indexing. Negative positions count from the end.
- Positional row slices are half-open: `pos(start, stop)` means `[start, stop)`.
- Label selectors match against the current case axis returned by `cases()`.
- `label(value)` returns every matching row, so duplicate case labels are preserved instead of collapsed.
- `label_range(start, stop)` is inclusive on both ends and walks the current case-axis order.
- Feature-position slices are half-open, while `col_range(start, stop)` is inclusive on both ends.
- `CasePick` reads named case levels created by `set_cases(...)`; it does not inspect ordinary feature columns.
- Partial `CasePick` matches preserve the existing row order already stored on the frame.
- Because `case_study(...)` always returns a frame, you do not need separate `at(...)` or `iat(...)` methods for single-cell access.

## Notes

- Use `features({...})` when you only need feature projection and no row lookup.
- Use `filter(mask)` when you already have a boolean row mask and want full-row retention with no feature projection in the same call.
- Use `observe(start, stop)` for display-oriented middle slices. Use `case_study(...)` when the selected result itself is part of later computation.
- If your current case axis is numeric and you want those labels as numeric data, call `cases().as_series()` explicitly.

## Examples

Example 1. Study one row by zero-based position.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos(1));
```

### Output

```text
city  region  units  price
Tema  south   7.0    4.0
```

Example 2. Study a half-open positional row window.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos(1, 3));
```

### Output

```text
city    region  units  price
Tema    south   7.0    4.0
Kumasi  middle  12.0   2.5
```

Example 3. Reorder rows by explicit positions and keep only named features.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos({3, 0}), cols({"city", "price"}));
```

### Output

```text
city    price
Tamale  5.0
Accra   3.0
```

Example 4. Match every row whose case label equals one label.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(label("B200"));
```

### Output

```text
city    region  units  price
Tema    south   7.0    4.0
Kumasi  middle  12.0   2.5
```

Example 5. Request several labels in your own order and project one feature subset.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(labels({"D400", "A100"}), cols({"city", "units"}));
```

### Output

```text
city    units
Tamale  5.0
Accra   10.0
```

Example 6. Slice by case labels inclusively and slice features by name inclusively.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(label_range("B200", "D400"), col_range("region", "price"));
```

### Output

```text
region  units  price
south   7.0    4.0
middle  12.0   2.5
north   5.0    5.0
```

Example 7. Use a boolean mask just like a filter, then keep only the columns you care about.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(mask(sales.features("units") >= 10.0), cols({"city", "units"}));
```

### Output

```text
city    units
Accra   10.0
Kumasi  12.0
```

Example 8. Get a one-cell result without a separate scalar-only indexing method.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(label("A100"), col("price"));
```

### Output

```text
price
3.0
```

Example 9. Count backward from the end with a negative row position.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos(-1), cols({"city", "price"}));
```

### Output

```text
city    price
Tamale  5.0
```

Example 10. Match one structured case level with `CasePick` after `set_cases(...)`.

```cpp
using namespace pluss::table;

auto indexed = DataFrame({
    {"region", pluss::make_note({"West", "West", "East", "East"})},
    {"account", pluss::make_series({101.0, 101.0, 201.0, 201.0})},
    {"quarter", pluss::make_note({"Q1", "Q2", "Q1", "Q2"})},
    {"sales", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
    {"cost", pluss::make_series({4.0, 8.0, 12.0, 16.0})},
}).set_cases({"region", "account", "quarter"}, false);

auto out = indexed.case_study(DataFrame::CasePick{
    .level = "region",
    .value = "West",
}).features({"sales", "cost"});
```

### Output

```text
sales  cost
10.0   4.0
20.0   8.0
```

Example 11. Look up one full structured case key with `CasePick`.

```cpp
using namespace pluss::table;

auto indexed = DataFrame({
    {"region", pluss::make_note({"West", "West", "East", "East"})},
    {"account", pluss::make_series({101.0, 101.0, 201.0, 201.0})},
    {"quarter", pluss::make_note({"Q1", "Q2", "Q1", "Q2"})},
    {"sales", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
    {"cost", pluss::make_series({4.0, 8.0, 12.0, 16.0})},
}).set_cases({"region", "account", "quarter"}, false);

auto out = indexed.case_study(DataFrame::CasePick{
    .key = {"East", "201", "Q2"},
}).features({"sales", "cost"});
```

### Output

```text
sales  cost
40.0   16.0
```

## Behavior Notes

- `cases()` remains the accessor for the case axis itself. It no longer doubles as a row-selection API.
- `CasePick` is the structured-case overload. Use it only after `set_cases(...)` has created named case levels.
- Duplicate label matches are not deduplicated. If the case axis contains the same label twice, both rows are returned.
- When you only want to preview a row window instead of materializing a selection result for later work, use `observe(...)`.
- If you already know the final feature order and only need column projection, `features({...})` is still the lighter-weight feature-only path.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bucketize.html">
    <span class="label">Previous</span>
    <span class="title">bucketize</span>
  </a>
  <a class="next-link" href="cases.html">
    <span class="label">Next</span>
    <span class="title">cases</span>
  </a>
</div>
