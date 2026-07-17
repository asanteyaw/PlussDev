# DataFrame::reorder

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::reorder(const ReorderSpec& spec) -> DataFrame

DataFrame::reorder(pluss::RangeView<ReorderSpec> specs) -> DataFrame

DataFrame::reorder(pluss::RangeView<std::string_view> names) -> DataFrame

DataFrame::reorder(std::initializer_list<ReorderSpec> specs) -> DataFrame
```

## What It Does

Moves selected features into a new relative position while preserving row data and the untouched feature order.

## Parameters

**spec**
    One partial-order move. Use this when one move instruction is enough.

**specs**
    Several partial-order moves applied in the exact order you pass them. Each move sees the column order produced by the previous move.

**names**
    Column names to move together to the front while preserving the relative order of every column not listed here.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Move selected columns to the front, back, or around an anchor while preserving the rest.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto spec = pluss::table::DataFrame::ReorderSpec::move("sales").after("city");
auto out = df.reorder(spec);
```

### Output

```text
id  city    sales
1   Accra   100.0
2   Tema    120.0
3   Kumasi   90.0
```
Example 2. Apply a reusable multi-step reorder plan stored in a standard container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

using Move = pluss::table::DataFrame::ReorderSpec;
std::vector<Move> plan{
    Move::move({"city", "sales"}).first(),
    Move::move("id").after("sales"),
};
auto out = df.reorder(plan);
```

### Output

```text
city    sales  id
Accra   100.0  1
Tema    120.0  2
Kumasi  90.0   3
```
Example 3. Move the named columns to the front while preserving the relative order of every untouched column.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder({"city"});
```

### Output

```text
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```
Example 4. Apply an inline multi-step reorder plan that moves only the columns you care about.

```cpp
auto df = pluss::table::DataFrame({
    {"col_1", pluss::make_series({1.0, 101.0})},
    {"col_2", pluss::make_series({2.0, 102.0})},
    {"col_3", pluss::make_series({3.0, 103.0})},
    {"col_4", pluss::make_series({4.0, 104.0})},
    {"col_5", pluss::make_series({5.0, 105.0})},
    {"col_6", pluss::make_series({6.0, 106.0})},
    {"col_7", pluss::make_series({7.0, 107.0})},
    {"col_8", pluss::make_series({8.0, 108.0})},
    {"col_9", pluss::make_series({9.0, 109.0})},
    {"col_10", pluss::make_series({10.0, 110.0})},
    {"col_15", pluss::make_series({15.0, 115.0})},
    {"col_16", pluss::make_series({16.0, 116.0})},
    {"col_17", pluss::make_series({17.0, 117.0})},
    {"col_20", pluss::make_series({20.0, 120.0})},
    {"col_21", pluss::make_series({21.0, 121.0})},
    {"col_22", pluss::make_series({22.0, 122.0})},
    {"col_23", pluss::make_series({23.0, 123.0})},
    {"col_24", pluss::make_series({24.0, 124.0})},
    {"col_25", pluss::make_series({25.0, 125.0})},
});

using Move = pluss::table::DataFrame::ReorderSpec;
auto out = df.reorder({
    Move::move({"col_25", "col_10"}).first(),
    Move::move("col_20").before("col_5"),
    Move::move({"col_15", "col_16", "col_17"}).after("col_8"),
    Move::move_regex("^col_2[1-4]$").last(),
});
```

### Output

```text
out.features():
["col_25", "col_10", "col_1", "col_2", "col_3", "col_4", "col_20", "col_5", "col_6", "col_7", "col_8", "col_15", "col_16", "col_17", "col_9", "col_21", "col_22", "col_23", "col_24"]

out.head():
col_25  col_10  col_1  col_2  col_3  col_4  col_20  col_5  col_6  col_7  col_8  col_15  col_16  col_17  col_9  col_21  col_22  col_23  col_24
25.0    10.0    1.0    2.0    3.0    4.0    20.0    5.0    6.0    7.0    8.0    15.0    16.0    17.0    9.0    21.0    22.0    23.0    24.0
125.0   110.0   101.0  102.0  103.0  104.0  120.0   105.0  106.0  107.0  108.0  115.0   116.0   117.0   109.0  121.0   122.0   123.0   124.0
```

## Practical Behavior

- `reorder({"a", "b"})` now means “move these features together and preserve the relative order of everything else.”
- Use `ReorderSpec` when you need to move a subset to the front, back, before an anchor, or after an anchor.
- Use the multi-spec overload when you want to apply several moves in one call from left to right.
- This avoids listing a 3000-feature schema just to reposition two columns.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename_.html">
    <span class="label">Previous</span>
    <span class="title">rename_</span>
  </a>
  <a class="next-link" href="repeated.html">
    <span class="label">Next</span>
    <span class="title">repeated</span>
  </a>
</div>
