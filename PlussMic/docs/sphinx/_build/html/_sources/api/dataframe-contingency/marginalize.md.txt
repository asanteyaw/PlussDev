<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ae8651eb6750 -->

# ContingencyTable::marginalize

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable::marginalize(std::string_view dim) const -> ContingencyTable
ContingencyTable::marginalize(std::initializer_list<std::string_view> dims) const -> ContingencyTable
ContingencyTable::marginalize(pluss::msharp::RangeView::Of<std::string_view> dims) const -> ContingencyTable
```

## What It Does

Sums the requested dimensions out of the retained table and removes them from
the object.

This is the retained-object equivalent of moving from a joint distribution to a
lower-dimensional marginal distribution.

## Parameters

**dim / dims**

One or more retained dimensions to sum away.

## Returns

`ContingencyTable`
    A new retained table with the requested dimensions removed and their counts summed
    into the remaining cells.

## When To Use It

Use `marginalize(...)` when one or more retained dimensions are no longer analytically
important and should be collapsed out before rendering or normalization.

## Examples

Example 1. Sum `weekend` out of a three-way table to recover the ordinary
`city × channel` counts.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto out = df.contingency({"city", "channel", "weekend"})
             .marginalize({"weekend"})
             .to_frame({"city"}, {"channel"});
std::cout << out;
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

Example 2. Marginalize a different dimension and project the result on a
different pair of axes.

```cpp
auto out = df.contingency({"city", "channel", "weekend"})
             .marginalize({"channel"})
             .to_frame({"city"}, {"weekend"});
std::cout << out;
```

### Output

```text
=====================
     | city  no  yes
---------------------
0    | A     2   0
1    | B     1   2
---------------------
[2 cs x 3 feat]
```

Example 3. Marginalizing multiple dimensions at once keeps summing until only
the requested remainder is left.

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

- `marginalize(...)` changes the object itself: `ndim()`, `shape()`, `dims()`,
  `to_array()`, `to_frame()`, and `facet()` all see the reduced table afterward.
- The requested dimensions are removed from remembered default row/column hints as
  well, so later plain `to_frame()` calls adapt to the smaller object.
- If a marginalized dimension had previously been named in `condition(...)`, it is also
  removed from the conditioned-dimension set.
- Duplicate dimension names are rejected.
- Unknown dimension names are rejected.
