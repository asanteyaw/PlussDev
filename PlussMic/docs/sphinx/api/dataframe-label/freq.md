<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::freq

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::freq(std::int64_t top_n = 0, bool dropnull = true) const -> Thread
```

## What It Does

Counts how many times each distinct case-label value occurs, and returns those
counts sorted by frequency descending (ties broken by the value itself,
ascending) — displayed against the value each count belongs to. `top_n`
optionally caps the result to the `top_n` most frequent values; `0` (the
default) keeps every distinct value.

## Parameters

**top_n** (`std::int64_t`, default `0`)

Maximum number of distinct values to keep, ordered by descending count. `0`
means no cap — keep every distinct value.

**dropnull** (`bool`, default `true`)

Whether to exclude a null case label from the tally.

## Returns

`Thread`
    `Int64` counts, one per distinct case-label value, sorted descending by
    count (ties broken ascending by value), `display_name` `"freq"`.

## Examples

Example 1. Tally a case-label thread with one repeated value.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto out = labeled.label().freq();
std::cout << out;
```

### Output

```text
 east  2
north  1
 west  1
Name: freq, ptype: Series
```

`"east"` occurs twice, so it leads with count `2`. `"north"` and `"west"` are
tied at `1`; the tie is broken alphabetically, so `"north"` comes before
`"west"`.

Example 2. Cap the result to the 2 most frequent values with `top_n = 2`.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))}},
    Thread(pluss::make_note({"east", "west", "east", "north", "east", "south"})));
std::cout << orders;

auto full = orders.label().freq();
std::cout << full;

auto top2 = orders.label().freq(2);
std::cout << top2;
```

### Output (orders)

```text
=============
      | sales
-------------
east  | 1    
west  | 2    
east  | 3    
north | 4    
east  | 5    
south | 6    
-------------
[6 cs x 1 feat]
```

### Output (full tally)

```text
 east  3
north  1
south  1
 west  1
Name: freq, ptype: Series
```

### Output (top_n = 2)

```text
 east  3
north  1
Name: freq, ptype: Series
```

`top_n = 2` keeps only the 2 most frequent rows *after* sorting — `"east"`
(count `3`) and then `"north"` (the alphabetically-first of the three-way tie
at count `1`) — never an arbitrary prefix of the unsorted labels.

Example 3. Pass `dropnull = false` to include a null case label in the tally.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sales", Thread(pluss::make_series({1.0, 2.0, 3.0}))}},
    Thread(pluss::make_series({10.0, 10.0, std::numeric_limits<double>::quiet_NaN()})));
std::cout << readings;

auto without_null = readings.label().freq();
std::cout << without_null;

auto with_null = readings.label().freq(0, false);
std::cout << with_null;
```

### Output (readings)

```text
============
     | sales
------------
10   | 1    
10   | 2    
NULL | 3    
------------
[3 cs x 1 feat]
```

### Output (default dropnull = true)

```text
10  2
Name: freq, ptype: Series
```

### Output (dropnull = false)

```text
  10  2
NULL  1
Name: freq, ptype: Series
```

With the default `dropnull = true`, the single `NULL` case label never
appears in the tally at all. With `dropnull = false`, it's counted like any
other distinct value — here landing after `10` since its count (`1`) is
lower.

## Notes

- The tie-break is on the value's own string form, always ascending —
  independent of the `top_n`/`dropnull` settings.
- `top_n` truncates *after* sorting, so it always keeps the most frequent
  values, never an arbitrary prefix of the unsorted data.

## See Also

- [`DataFrameLabelAccessor::distinct`](distinct.md): the same set of unique
  values, without counts, in first-occurrence rather than frequency order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find.html">
    <span class="label">Previous</span>
    <span class="title">find</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
