<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9e4f0d84257f -->

# DataFrame::contingency

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
ContingencyTable DataFrame::contingency(std::string_view row_feature,
                                        std::string_view column_feature) const
ContingencyTable DataFrame::contingency(std::string_view row_feature,
                                        std::string_view column_feature,
                                        const ContingencyOptions& options) const
ContingencyTable DataFrame::contingency(std::initializer_list<std::string_view> features) const
ContingencyTable DataFrame::contingency(std::initializer_list<std::string_view> features,
                                        const ContingencyOptions& options) const
ContingencyTable DataFrame::contingency(std::initializer_list<std::string_view> row_features,
                                        std::initializer_list<std::string_view> column_features) const
ContingencyTable DataFrame::contingency(std::initializer_list<std::string_view> row_features,
                                        std::initializer_list<std::string_view> column_features,
                                        const ContingencyOptions& options) const
ContingencyTable DataFrame::contingency(pluss::msharp::RangeView::Of<std::string_view> features) const
ContingencyTable DataFrame::contingency(pluss::msharp::RangeView::Of<std::string_view> features,
                                        const ContingencyOptions& options) const
ContingencyTable DataFrame::contingency(pluss::msharp::RangeView::Of<std::string_view> row_features,
                                        pluss::msharp::RangeView::Of<std::string_view> column_features) const
ContingencyTable DataFrame::contingency(pluss::msharp::RangeView::Of<std::string_view> row_features,
                                        pluss::msharp::RangeView::Of<std::string_view> column_features,
                                        const ContingencyOptions& options) const
```

## What It Does

Constructs a retained contingency table from one or more categorical dimensions.

`contingency(...)` is now frequency/distribution-first:

- it cross-classifies the requested dimensions once
- keeps the result as an N-dimensional `ContingencyTable`
- lets you transform that object mathematically before any 2D rendering step

That makes it broader than a plain crosstab render:

- `df.contingency({"gender", "age", "region", "outcome"})` keeps a 4D table
- `slice(...)` fixes levels and removes those dimensions
- `marginalize(...)` sums dimensions out
- `condition(...)` turns counts into conditional shares
- `to_frame(...)` is the explicit 2D projection step
- `facet(...)` renders the remaining dimensions as several 2D frames

The two calling styles serve different defaults:

- `contingency({"city", "channel", "weekend"})` keeps those three dimensions and,
  by default, renders the first one on rows and the rest on columns
- `contingency({"city"}, {"channel", "weekend"})` keeps the same dimensions, but
  remembers `"city"` as the default row side and `{"channel", "weekend"}` as the
  default column side

Use the [ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)
for the retained object itself.

The intended retained-object surface is deliberately compact:

- inspect with `ndim()`, `shape()`, `dims()`
- stay in count-space with `freq()`
- move to global shares with `share()`
- move to conditional shares with `condition(...)`
- slice away fixed levels with `slice(...)`
- sum dimensions out with `marginalize(...)`
- export as N-D data with `to_array()`
- render one 2D table with `to_frame()`
- render several 2D tables with `facet()`

That means the old builder-style methods such as `count()`, `rowshare()`,
`colshare()`, `sum()`, `mean()`, and friends are intentionally gone from this
surface. The retained table itself is the object now; rendering is explicit.

## Parameters

**features**

One or more dimensions to cross-classify and retain.

**row_feature / row_features**

Dimensions that should become the default row side when you later call
`to_frame()` or `facet()` without supplying explicit row/column dimensions.

**column_feature / column_features**

Dimensions that should become the default column side for later rendering.

**options** (`ContingencyOptions`, optional)

- `dropnull`: whether rows with null contingency dimensions are excluded at
  construction time
- `sort`: whether each retained dimension's levels are sorted instead of kept in
  first-seen order
- `totals`: a render-time hint used by `to_frame(...)` / `facet(...)`
- `separator`: how multi-dimension rendered column/facet labels are flattened
- `null_label`: the text used when retained null levels are rendered

## Returns

`ContingencyTable`
    A retained N-dimensional contingency object.

## Examples

Example 1. Build a classic two-way table, then render it.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto ct = df.contingency("city", "channel");
std::cout << ct.to_frame();
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

Example 2. Keep three dimensions, then choose a projection later.

```cpp
auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto ct = df.contingency({"city", "channel", "weekend"});
std::cout << ct.to_frame({"city"}, {"channel"});
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

Example 3. The row/column overload remembers a default projection.

```cpp
auto ct = df.contingency({"city"}, {"channel", "weekend"});
std::cout << ct.to_frame();
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

Because `"city"` was supplied on the row side and `{"channel", "weekend"}`
were supplied on the column side at construction time, plain `to_frame()`
reuses that remembered layout.

Example 4. Transform the retained object before rendering.

```cpp
auto ct = df.contingency({"city", "channel", "weekend"});
std::cout << ct.slice({{"weekend", "yes"}})
               .condition({"city"})
               .to_frame({"city"}, {"channel"});
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     0     0   
1    | B     0.5   0.5 
-----------------------
[2 cs x 3 feat]
```

Example 5. `ContingencyOptions` affect construction and later rendering, not
the retained method surface.

```cpp
DataFrame::ContingencyOptions options;
options.totals = true;

auto ct = df.contingency("city", "channel", options);
std::cout << ct.to_frame();
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

That is the architectural difference from a plain crosstab: the object stays
N-dimensional until you decide what 2D view you want, and options like totals
are applied at render time rather than through a separate builder method.

## Notes

- Sorting is lexicographic by full tuple within each rendered side. With
  `{"channel", "weekend"}`, `channel` is the primary key and `weekend` is the
  tie-breaker.
- The one-list overload remembers the first dimension as the default row side
  and the remaining dimensions as the default column side.
- The row/column overload remembers exactly the supplied row and column sides
  for later `to_frame()` / `facet()` calls.
- `share()` is the global-table share.
- `condition({...})` is the targeted conditional share surface. When you later
  render `to_frame({"city"}, {"channel"})`, `condition({"city"})` is the
  retained-object equivalent of a row-normalized contingency table.
- This surface is intentionally focused on joint frequencies/distributions.
  For arbitrary numeric value aggregation, use `group(...).agg(...)` or
  `pivot_table(...)`.

## See Also

- [ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)
- [`DataFrame::group`](group.md)
- [`DataFrame::pivot_table`](pivot_table.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conform.html">
    <span class="label">Previous</span>
    <span class="title">conform</span>
  </a>
  <a class="next-link" href="copy.html">
    <span class="label">Next</span>
    <span class="title">copy</span>
  </a>
</div>
