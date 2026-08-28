<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 782815345eb0 -->

# ContingencyTable::facet

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable::facet() const -> Atlas
ContingencyTable::facet(const DataFrame::ContingencyOptions& options) const -> Atlas
ContingencyTable::facet(std::initializer_list<std::string_view> row_dims,
                        std::initializer_list<std::string_view> column_dims) const -> Atlas
ContingencyTable::facet(std::initializer_list<std::string_view> row_dims,
                        std::initializer_list<std::string_view> column_dims,
                        const DataFrame::ContingencyOptions& options) const -> Atlas
ContingencyTable::facet(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                        pluss::msharp::RangeView::Of<std::string_view> column_dims) const -> Atlas
ContingencyTable::facet(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                        pluss::msharp::RangeView::Of<std::string_view> column_dims,
                        const DataFrame::ContingencyOptions& options) const -> Atlas
```

## What It Does

Renders several 2D tables instead of flattening the remaining dimensions onto
the column side.

`facet(...)` is the alternative to `to_frame(...)` when one very wide flattened table
would be harder to read than a small atlas of separate tables.

## Parameters

**row_dims**

Dimensions that should form the row-key side inside each facet frame.

**column_dims**

Dimensions that should form the column side inside each facet frame.

**options**

The same render-time `ContingencyOptions` used by `to_frame(...)`, especially
`totals`, `separator`, and `null_label`.

## Returns

`Atlas`
    One `DataFrame` per leftover facet combination.

## When To Use It

Use `facet(...)` when:

- the retained object has more dimensions than you want to flatten into one frame
- you want one table per remaining level combination
- the leftover dimensions are better understood as panels than as encoded column names

## Examples

Example 1. Keep `weekend` as the facet dimension instead of flattening it into
the column labels.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto facets = df.contingency({"city", "channel", "weekend"})
                 .facet({"city"}, {"channel"});
std::cout << facets;
```

### Output

```text
=============================
[0] weekend=no
=======================
     | city  Card  Cash
-----------------------
0    | A     1     1   
1    | B     0     1   
-----------------------
[2 cs x 3 feat]

=============================
[1] weekend=yes
=======================
     | city  Card  Cash
-----------------------
0    | A     0     0   
1    | B     1     1   
-----------------------
[2 cs x 3 feat]
```

Example 2. Inspect the facet axis directly when you care about the slices
rather than the pretty printed atlas.

```cpp
auto facets = df.contingency({"city", "channel", "weekend"})
                 .facet({"city"}, {"channel"});

std::cout << facets.depth() << "\n";
for (auto label : facets.axis0_labels()->as_note().vector()) {
  std::cout << label << "\n";
}
```

### Output

```text
2
weekend=no
weekend=yes
```

## Notes

- Any retained dimension not assigned to `row_dims` or `column_dims` becomes part of
  the facet axis.
- If no leftover dimensions remain, `facet(...)` still returns an `Atlas`, but with a
  single frame.
- Duplicate row dimensions are rejected.
- Duplicate facet-column dimensions are rejected.
- A dimension cannot be named on both the row side and the facet-column side.
