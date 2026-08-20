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
