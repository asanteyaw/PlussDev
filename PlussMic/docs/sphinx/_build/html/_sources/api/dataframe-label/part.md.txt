<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 920dc1db6ab9 -->

# DataFrameLabelAccessor::part

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::part(std::string_view name) const -> Thread
DataFrameLabelAccessor::part(std::int64_t position) const -> Thread
```

## What It Does

Reads a single case-label level's own values, by name or by position, from a
structured (multi-level) frame. Requires `has()` to be `true`; calling this on
an unstructured frame throws.

## Parameters

**name** (`std::string_view`, one overload)

The level's name, as returned by [`names()`](names.md).

**position** (`std::int64_t`, one overload)

The level's position (0-based; negative counts back from the end).

## Returns

`Thread`
    That level's own values — not the composite case label, just this one
    level's contribution to it.

## Examples

Example 1. Read each level of a 2-level structured frame, by name and by
position.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Thread(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");

auto region_by_name = grouped.label().part("region");
std::cout << region_by_name;

auto city_by_position = grouped.label().part(1);
std::cout << city_by_position;
```

### Output (part("region"), same as part(0))

```text
0  east
1  east
2  west
3  west
ptype: Note
```

### Output (part(1), same as part("city"))

```text
0  Boston
1  NYC
2  LA
3  SF
ptype: Note
```

Each call returns just that one level's own values — not the joined
`"east/Boston"`-style composite label `values()` would return.

Example 2. Use a negative position to count back from the last level.

```cpp
auto city_by_negative_position = grouped.label().part(-1);
std::cout << city_by_negative_position;
```

### Output (part(-1), same as part(1) / part("city") on this 2-level frame)

```text
0  Boston
1  NYC
2  LA
3  SF
ptype: Note
```

`-1` is the last level regardless of how many levels exist — here that's
position `1` (`"city"`), the same result as Example 1's `part(1)`.

Example 3. Call `part()` on an unstructured frame, to see it throw.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales", Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
std::cout << orders.label().has();

try {
  auto out = orders.label().part("region");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
false
DataFrame::caselbl: current DataFrame has no case levels
```

`orders` never had its case labels promoted via `set()`, so `has()` is
`false` and there's no level named `"region"` to read.

Example 4. Ask for a level name that doesn't exist on an otherwise structured
frame.

```cpp
try {
  auto out = grouped.label().part("country");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::caselbl: unknown case level 'country'
```

`grouped` is structured, but its only levels are `"region"` and `"city"` —
asking for `"country"` throws rather than returning an empty or null result.

## Notes

- Throws if the frame isn't structured (`has()` is `false`) — there's no level
  to read from a flat or default case axis.
- The returned `Thread` carries no display labels of its own; it prints with a
  plain default integer index, same as any other freestanding `Thread`.

## See Also

- [`DataFrameLabelAccessor::values`](values.md): the full composite case label,
  as opposed to one level's contribution.
- [`DataFrameLabelAccessor::names`](names.md): the level names usable with the
  by-name overload.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="order.html">
    <span class="label">Previous</span>
    <span class="title">order</span>
  </a>
  <a class="next-link" href="pos.html">
    <span class="label">Next</span>
    <span class="title">pos</span>
  </a>
</div>
