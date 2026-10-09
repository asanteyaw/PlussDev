<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5cd8cbc1667 -->

# ContingencyTable::to_frame

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable::to_frame() const -> DataFrame
ContingencyTable::to_frame(const DataFrame::ContingencyOptions& options) const -> DataFrame
ContingencyTable::to_frame(std::initializer_list<std::string_view> row_dims,
                           std::initializer_list<std::string_view> column_dims) const -> DataFrame
ContingencyTable::to_frame(std::initializer_list<std::string_view> row_dims,
                           std::initializer_list<std::string_view> column_dims,
                           const DataFrame::ContingencyOptions& options) const -> DataFrame
ContingencyTable::to_frame(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                           pluss::msharp::RangeView::Of<std::string_view> column_dims) const -> DataFrame
ContingencyTable::to_frame(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                           pluss::msharp::RangeView::Of<std::string_view> column_dims,
                           const DataFrame::ContingencyOptions& options) const -> DataFrame
```

## What It Does

Renders one explicit 2D `DataFrame` view from the retained contingency object.

- `row_dims` chooses what becomes row keys
- `column_dims` chooses what becomes rendered columns
- any remaining unsupplied dimensions are appended onto the column side
- if `column_dims` ends up empty, the rendered frame gets one scalar value
  column such as `freq` or `share`

This page is the most important rendering page on the surface because it is the point
where the retained N-dimensional object becomes an ordinary `DataFrame`.

## Parameters

**row_dims**

Dimensions that should become explicit row-key columns in the rendered frame.

**column_dims**

Dimensions that should become rendered columns. Any retained dimensions not named here
and not already named on the row side are appended to the column side automatically.

**options**

Render-time options:

- `totals`: append row totals, column totals, and the grand total
- `separator`: join multi-dimension flattened column labels
- `null_label`: display text for retained null levels

## Returns

`DataFrame`
    One 2D projection of the retained contingency object.

## When To Use It

Use `to_frame(...)` when you want one explicit tabular projection, either because:

- the retained object is already 2D
- flattening the leftover dimensions into columns is acceptable
- you want a conventional crosstab-style `DataFrame` result

## Examples

Example 1. Default 2D render from a two-way table.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

std::cout << df.contingency("city", "channel").to_frame();
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     1     1   
1    | B     1     2   
-----------------------
[2 cs x 3 feat]
```

Example 2. Explicitly project a 3D retained table to 2D.

```cpp
auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

std::cout << df.contingency({"city", "channel", "weekend"})
                 .to_frame({"city"}, {"channel"});
```

### Output

```text
===============================================================
     | city  channel=Card | weekend=no  channel=Card | weekend=yes  channel=Cash | weekend=no  channel=Cash | weekend=yes
-------------------------------------------------------------------------------------------------------------------------------
0    | A     1                          0                           1                          0                         
1    | B     0                          1                           1                          1                         
-------------------------------------------------------------------------------------------------------------------------------
[2 cs x 5 feat]
```

Example 3. Add totals at render time without changing the retained object.

```cpp
DataFrame::ContingencyOptions options;
options.totals = true;

std::cout << df.contingency("city", "channel").to_frame(options);
```

### Output

```text
==============================
     | city   Card  Cash  Total
------------------------------
0    | A      1     1     2
1    | B      1     2     3
2    | Total  2     3     5
------------------------------
[3 cs x 4 feat]
```

Example 4. Render a one-dimensional retained table against an implicit scalar
value column.

```cpp
auto out = df.contingency({"city", "channel", "weekend"})
             .marginalize({"channel", "weekend"})
             .to_frame({"city"}, {});
std::cout << out;
```

### Output

```text
=============
     | city  freq
-------------
0    | A     2
1    | B     3
-------------
[2 cs x 2 feat]
```

## Notes

- The row side and explicit column side cannot reuse the same dimension.
- Duplicate row dimensions are rejected.
- Duplicate column dimensions are rejected.
- Any retained dimensions left unassigned are appended to the column side in retained
  dimension order.
- `totals` changes the render only; it does not add a synthetic retained dimension to
  the `ContingencyTable`.
- If the current table is in `share()` or `condition(...)` space, the implicit scalar
  value column is named `share`; otherwise it is named `freq`.
