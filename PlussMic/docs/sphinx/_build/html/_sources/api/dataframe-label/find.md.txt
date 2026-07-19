<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::find

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::find(const Thread& targets) const -> Thread
```

## What It Does

For each value in `targets`, looks up the position of its **first** occurrence
among the case labels — `-1` if that value never appears. Unlike
[`pos()`](pos.md), which returns every match for a single value, `find()`
returns exactly one position (or `-1`) per target, for potentially many
targets at once.

## Parameters

**targets** (`const Thread&`)

The values to look up, in the order you want positions back.

## Returns

`Thread`
    One `Int64` position per target (`-1` if not found), `display_name`
    `"find"`, displayed against `targets` themselves.

## Examples

Example 1. Look up three target values, one of which isn't present.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto targets = Thread(pluss::make_note({"west", "north", "south"}));
auto out = labeled.label().find(targets);
std::cout << out;
```

### Output

```text
 west  1
north  3
south  -1
Name: find, ptype: Series
```

`"west"` first appears at row `1`, `"north"` at row `3`; `"south"` never
appears in the case labels at all, so it comes back `-1`. `"east"`'s repeat at
row `2` is irrelevant here since it wasn't among `targets`.

## Notes

- Only the **first** matching position is ever returned per target, even if
  the case labels have more than one match.
- The result is displayed against `targets`, not against this accessor's own
  case labels — so a target with no match still gets a row, labeled by that
  target value, holding `-1`.

## See Also

- [`DataFrameLabelAccessor::pos`](pos.md): all matching positions for a single
  value, rather than the first match for each of several targets.
- [`DataFrameLabelAccessor::slice`](slice.md): a label-value-bounded range
  instead of individual lookups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
