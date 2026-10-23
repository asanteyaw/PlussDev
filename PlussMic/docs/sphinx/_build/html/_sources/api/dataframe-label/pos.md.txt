<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 70508d84bd02 -->

# DataFrameLabelAccessor::pos

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::pos(double value) const -> Slate
DataFrameLabelAccessor::pos(std::string_view value) const -> Slate
DataFrameLabelAccessor::pos(const char* value) const -> Slate
```

## What It Does

Returns **every** position where the case label equals `value`. Throws if
`value` never occurs. Unlike [`find()`](find.md), which returns only the first
match for each of several targets, `pos()` returns *all* matches for a single
target.

## Parameters

**value** (`double` / `std::string_view` / `const char*`)

The value to locate every occurrence of.

## Returns

`Slate`
    `Int64` positions where the case label equals `value`, one entry per
    match, `display_name` `"pos"`, plain default integer index.

## Examples

Example 1. Locate every occurrence of a repeated case-label value.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

auto out = labeled.labels().pos("east");
std::cout << out;
```

### Output

```text
0  0
1  2
Name: pos, ptype: Series
```

`"east"` occurs at rows `0` and `2` — both are returned, in ascending order.
The plain `0`/`1` on the left are just `pos()`'s own default index (the "1st
match", "2nd match"), not the matched positions themselves — those are the
values on the right.

Example 2. Locate a value on a numeric (`Series`) case-label axis, using the
`double` overload.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}},
    Slate(pluss::make_series({10.0, 20.0, 10.0, 30.0})));
std::cout << readings;

auto out = readings.labels().pos(10.0);
std::cout << out;
```

### Output (readings)

```text
============
     | sales
------------
10   | 100  
20   | 200  
10   | 150  
30   | 90   
------------
[4 cs x 1 feat]
```

### Output (pos(10.0))

```text
0  0
1  2
Name: pos, ptype: Series
```

`10.0` occurs at rows `0` and `2`, the same shape of result as the `Note`-kind
case above, just matched by numeric equality instead of string equality.

Example 3. Look up a value that doesn't occur, to see the call throw.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

try {
  auto out = labeled.labels().pos("south");
} catch (const std::exception& e) {
  std::cout << "threw\n";
}
```

### Output

```text
threw
```

`"south"` never appears among the case labels, so the call throws instead of
returning an empty `Slate` — check [`has("south")`](has.md) first if the
value's presence isn't already guaranteed.

## Notes

- Throws if `value` doesn't occur at all — check
  [`has(value)`](has.md) first if absence is possible.
- Returns *every* match, unlike [`find()`](find.md)'s first-match-only lookup.

## See Also

- [`DataFrameLabelAccessor::find`](find.md): first-match lookup for several
  target values at once, instead of every match for one value.
- [`DataFrameLabelAccessor::has`](has.md): a membership check that never
  throws, as a safe precondition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="part.html">
    <span class="label">Previous</span>
    <span class="title">part</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
