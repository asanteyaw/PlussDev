<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4ea09bb6d7ec -->

# ContingencyTable::freq

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
ContingencyTable::freq() const -> ContingencyTable
```

## What It Does

Returns the same retained table in raw frequency space.

`freq()` does not rebuild the contingency table. It simply clears any active
share/conditioning state and keeps the same retained dimensions, levels, and
default render hints.

## Examples

Example 1. Convert a global-share table back to raw counts before rendering.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto out = df.contingency("city", "channel")
             .share()
             .freq()
             .to_frame();
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

Example 2. `freq()` also clears conditional normalization.

```cpp
auto out = df.contingency("city", "channel")
             .condition({"city"})
             .freq()
             .to_frame();
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
