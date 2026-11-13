<!-- pluss-hand-authored -->

# SlateLabelAccessor::pos

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signatures

```cpp
Slate SlateLabelAccessor::pos(double value) const
Slate SlateLabelAccessor::pos(std::string_view value) const
```

## What It Does

Returns **every** position where the bound slate's case label equals
`value`. Throws if `value` never occurs. Unlike [`find()`](find.md), which
returns only the first match for each of several targets, `pos()` returns
*all* matches for a single target.

The `double` overload converts `value` to its display-string form and
dispatches to the `std::string_view` overload internally — both end up
comparing against the case labels' own string representation.

## Parameters

**value** (`double` / `std::string_view`)

The value to locate every occurrence of.

## Returns

`Slate`
    `Int64` positions where the case label equals `value`, one entry per
    match, `display_name` `"pos"`, plain default integer index.

## Examples

Example 1. Locate every occurrence of a repeated case-label value.

```cpp
using namespace pluss::table;

auto tagged = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Slate(pluss::make_note({"east", "west", "east", "north"})));

auto out = tagged.label().pos("east");
std::cout << out;
```

### Output

```text
0  0
1  2

ptype: Series
```

`"east"` occurs at rows `0` and `2` — both are returned, in ascending order.
The plain `0`/`1` on the left are just `pos()`'s own default index (the "1st
match", "2nd match"), not the matched positions themselves — those are the
values on the right.

Example 2. Locate a value on a numeric case-label axis, using the `double`
overload.

```cpp
auto readings = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                     .label().set(Slate(pluss::make_series({10.0, 20.0, 10.0, 30.0})));

auto out2 = readings.label().pos(10.0);
std::cout << out2;
```

### Output

```text
0  0
1  2

ptype: Series
```

`10.0` occurs at rows `0` and `2`, the same shape of result as the `Note`-kind
case above, just matched by numeric equality instead of string equality.

Example 3. Look up a value that doesn't occur, to see the call throw.

```cpp
try {
  auto bad = tagged.label().pos("south");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::label().pos: label not found
```

`"south"` never appears among the case labels, so the call throws instead of
returning an empty `Slate` — check [`has(...)`](has.md) first if the
value's presence isn't already guaranteed.

## Notes

- Throws if `value` doesn't occur at all.
- Returns *every* match, unlike [`find()`](find.md)'s first-match-only
  lookup.

## See Also

- [`SlateLabelAccessor::find`](find.md): first-match lookup for several
  target values at once, instead of every match for one value.
- [`SlateLabelAccessor::has`](has.md): a membership check that never
  throws, as a safe precondition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="range.html">
    <span class="label">Next</span>
    <span class="title">range</span>
  </a>
</div>
