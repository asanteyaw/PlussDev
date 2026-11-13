<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1be63272371c -->

# ContingencyTable::slice

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable ContingencyTable::slice(std::string_view dim, std::string_view value) const
ContingencyTable ContingencyTable::slice(std::string_view dim, const char* value) const
ContingencyTable ContingencyTable::slice(std::string_view dim, bool value) const
ContingencyTable ContingencyTable::slice(std::string_view dim, std::int64_t value) const
ContingencyTable ContingencyTable::slice(std::string_view dim, double value) const
ContingencyTable ContingencyTable::slice(std::initializer_list<SliceSpec> specs) const
```

## What It Does

Fixes one or more dimensions at concrete levels, removes those dimensions from
the retained object, and keeps the remaining contingency tensor.

`slice(...)` is the explicit "condition on concrete level values first, render
later" step.

## Parameters

**dim / value**

The one-dimension convenience overloads for string, boolean, integer, and double levels.

**specs**

One or more `SliceSpec` items, each pairing a retained dimension name with one concrete
level value.

## Returns

`ContingencyTable`
    A new retained table containing only the requested level slice, with the sliced
    dimensions removed from the object.

## When To Use It

Use `slice(...)` when the question is "show me the contingency table inside this fixed
subpopulation" rather than "sum over this dimension."

That is the conceptual difference:

- `slice({"weekend" = "yes"})` -> keep only the `weekend = yes` slice
- `marginalize({"weekend"})` -> add `weekend = no` and `weekend = yes` together

## Examples

Example 1. Fix one dimension and then render the remaining two-way table.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
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

Example 3. Use the typed boolean overload instead of stringifying a logical level.

```cpp
auto flags = DataFrame({
    {"segment", Slate(pluss::make_note({"A", "A", "B", "B"}))},
    {"active",  Slate(pluss::make_series({true, false, true, false}))},
    {"channel", Slate(pluss::make_note({"Cash", "Cash", "Card", "Cash"}))},
});

auto out = flags.contingency({"segment", "active", "channel"})
             .slice("active", true)
             .to_frame({"segment"}, {"channel"});
std::cout << out;
```

### Output

```text
=======================
     | segment  Card  Cash
-----------------------
0    | A        0     1
1    | B        1     0
-----------------------
[2 cs x 3 feat]
```

## Notes

- Each slice removes the sliced dimension from the retained object.
- Slicing several dimensions is order-independent from the user's perspective; each
  named dimension is resolved on the current retained object as the slice proceeds.
- Unknown dimensions are rejected.
- Unknown levels inside a known dimension are rejected.
- If you want to keep all levels but change their weights, use `share()` or
  `condition(...)` instead. `slice(...)` is structural, not merely numeric.
