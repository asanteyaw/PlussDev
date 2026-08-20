<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e1052070763 -->

# ContingencyTable::condition

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable::condition(std::string_view dim) const -> ContingencyTable
ContingencyTable::condition(std::initializer_list<std::string_view> dims) const -> ContingencyTable
ContingencyTable::condition(pluss::msharp::RangeView::Of<std::string_view> dims) const -> ContingencyTable
```

## What It Does

Turns the retained count table into a conditional-share table with respect to
the supplied dimensions.

## Examples

Example 1. Condition on the row dimension.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto out = df.contingency("city", "channel")
             .condition({"city"})
             .to_frame();
std::cout << out;
```

### Output

```text
=============================
     | city  Card      Cash
-----------------------------
0    | A     0.5       0.5
1    | B     0.333333  0.666667
-----------------------------
[2 cs x 3 feat]
```

Example 2. Condition on the column dimension instead.

```cpp
auto out = df.contingency("city", "channel")
             .condition({"channel"})
             .to_frame();
std::cout << out;
```

### Output

```text
=============================
     | city  Card  Cash
-----------------------------
0    | A     0.5   0.333333
1    | B     0.5   0.666667
-----------------------------
[2 cs x 3 feat]
```

Example 3. Condition after slicing to normalize within the remaining subset
rather than the original whole table.

```cpp
auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto out = df.contingency({"city", "channel", "weekend"})
             .slice({{"weekend", "yes"}})
             .condition({"city"})
             .to_frame({"city"}, {"channel"});
std::cout << out;
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
