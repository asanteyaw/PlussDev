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

## Examples

Example 1. Sum `weekend` out of a three-way table to recover the ordinary
`city × channel` counts.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
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
