<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1be63272371c -->

# ContingencyTable::slice

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable::slice(std::string_view dim, std::string_view value) const -> ContingencyTable
ContingencyTable::slice(std::string_view dim, const char* value) const -> ContingencyTable
ContingencyTable::slice(std::string_view dim, bool value) const -> ContingencyTable
ContingencyTable::slice(std::string_view dim, std::int64_t value) const -> ContingencyTable
ContingencyTable::slice(std::string_view dim, double value) const -> ContingencyTable
ContingencyTable::slice(std::initializer_list<SliceSpec> specs) const -> ContingencyTable
```

## What It Does

Fixes one or more dimensions at concrete levels, removes those dimensions from
the retained object, and keeps the remaining contingency tensor.

`slice(...)` is the explicit "condition on concrete level values first, render
later" step.

## Examples

Example 1. Fix one dimension and then render the remaining two-way table.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto out = df.contingency({"city", "channel", "weekend"})
             .slice({{"weekend", "yes"}})
             .to_frame({"city"}, {"channel"});
std::cout << out;
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     0     0   
1    | B     1     1   
-----------------------
[2 cs x 3 feat]
```

Example 2. Slice more than one dimension and let the remaining dimension render
against an implicit scalar value column.

```cpp
auto out = df.contingency({"city", "channel", "weekend"})
             .slice({{"city", "B"}, {"weekend", "yes"}})
             .to_frame({"channel"}, {});
std::cout << out;
```

### Output

```text
=================
     | channel  freq
-----------------
0    | Card     1
1    | Cash     1
-----------------
[2 cs x 2 feat]
```

The one-spec overloads are convenience entry points. The retained object still
has the full type-directed overload set, so if a dimension's levels are
numeric, boolean, or floating, you should slice it with the matching overload
rather than converting everything to strings by hand.
