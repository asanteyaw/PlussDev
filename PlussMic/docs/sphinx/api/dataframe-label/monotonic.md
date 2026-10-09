<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ef58abf480e3 -->

# DataFrameLabelAccessor::monotonic

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::monotonic(bool ascending = true, bool strict = false, bool nulls_last = true) const -> bool
```

## What It Does

Checks whether the flat case-label slate is sorted, according to `ascending`
and `strict`. This always compares the full case label as one sequence — for
a structured (multi-level) frame it does **not** check per-level sortedness;
use [`depth()`](depth.md) for that.

## Parameters

**ascending** (`bool`, default `true`)

Check ascending order if `true`, descending if `false`.

**strict** (`bool`, default `false`)

If `true`, equal adjacent case labels fail the check (strictly increasing/
decreasing); if `false`, equal adjacent values are allowed.

**nulls_last** (`bool`, default `true`)

Where a null case label is expected to sort relative to real values.

## Returns

`bool`
    Whether the case labels satisfy the requested order.

## Examples

Example 1. An unsorted case-label axis, and a sorted rearrangement of the same
values.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

std::cout << labeled.label().monotonic();

auto sorted = labeled.label().sort();
std::cout << sorted.label().monotonic();
```

### Output

```text
false
true
```

`labeled`'s case labels (`"east"`, `"west"`, `"east"`, `"north"`) decrease from
`"west"` back down to `"east"` at row `2`, so they aren't sorted ascending.
After [`sort()`](sort.md) rearranges the rows to `"east"`, `"east"`,
`"north"`, `"west"`, `monotonic()` is `true`.

Example 2. Use `strict = true` to reject equal adjacent case labels.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "north", "west"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

std::cout << labeled.label().monotonic();
std::cout << labeled.label().monotonic(true, true);
```

### Output

```text
true
false
```

`labeled`'s case labels are already sorted ascending, with one repeated
adjacent pair (`"east"`, `"east"`). The default `strict = false` allows equal
neighbors, so `monotonic()` is `true`. Passing `strict = true` rejects that
same equal pair, so `monotonic(true, true)` is `false`.

Example 3. Pass `ascending = false` to check for descending order instead.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"west", "north", "east"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0}))}});
auto labeled = orders.label().set("region");

std::cout << labeled.label().monotonic();
std::cout << labeled.label().monotonic(false);
```

### Output

```text
false
true
```

`"west"`, `"north"`, `"east"` decreases at every step, so the default
`ascending = true` check fails. Passing `ascending = false` confirms the same
case labels are monotonically descending instead.

Example 4. Pass `nulls_last = false` to change where a null case label is
expected to sort.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0, 3.0}))}},
    Slate(pluss::make_series({10.0, 20.0, std::numeric_limits<double>::quiet_NaN()})));
std::cout << readings;

std::cout << readings.label().monotonic();
std::cout << readings.label().monotonic(true, false, false);
```

### Output (readings)

```text
============
     | value
------------
10   | 1    
20   | 2    
NULL | 3    
------------
[3 cs x 1 feat]
```

### Output

```text
true
false
```

With the default `nulls_last = true`, the trailing `NULL` case label is
treated as sorting after every real value, so the ascending check still
holds. Passing `nulls_last = false` instead expects nulls to sort *first*;
since the `NULL` here comes last, that expectation is violated and the check
fails.

## Notes

- Operates on the flat case label as a single sequence, even on a structured
  frame — see [`depth()`](depth.md) for a per-level, lexicographic version of
  this same check.

## See Also

- [`DataFrameLabelAccessor::depth`](depth.md): the multi-level generalization
  of this check.
- [`DataFrameLabelAccessor::sort`](sort.md): make the case labels monotonic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
