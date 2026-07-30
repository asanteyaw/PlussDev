<!-- pluss-hand-authored -->

# ThreadLabelAccessor::freq

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::freq(std::int64_t top_n = 0, bool dropnull = true) const -> Thread
```

## What It Does

Counts how many times each distinct case-label value occurs, and returns
those counts sorted by frequency descending (a stable sort, so values tied
at the same count keep their original **first-occurrence** relative order,
not alphabetical order) — displayed against the value each count belongs to.
`top_n` optionally caps the result to the `top_n` most frequent values; `0`
(the default) keeps every distinct value.

## Parameters

**top_n** (`std::int64_t`, default `0`)

Maximum number of distinct values to keep, ordered by descending count. `0`
means no cap.

**dropnull** (`bool`, default `true`)

Whether to exclude a null case label from the tally.

## Returns

`Thread`
    `Int64` counts, one per distinct case-label value, sorted descending by
    count (ties keep first-occurrence order), `display_name` `"freq"`.

## Examples

Example 1. Tally case labels with one repeated value.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

auto out = tagged.label().freq();
std::cout << out;
```

### Output

```text
east   2
west   1
north  1

ptype: Series
```

`"east"` occurs twice, so it leads with count `2`; `"west"` and `"north"` are
tied at `1` — the tie is broken by which one was **first seen** in the
original data (`"west"` at row `1`, before `"north"` at row `3`), not by
alphabetical order (which would instead put `"north"` first).

Example 2. Cap the result to the 2 most frequent values with `top_n = 2`.

```cpp
auto wide = Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))
                .label().set(Thread(pluss::make_note(
                    {"east", "west", "east", "north", "east", "south"})));

std::cout << wide.label().freq(2);
```

### Output

```text
east  3
west  1

ptype: Series
```

`top_n = 2` keeps only the 2 most frequent rows *after* sorting — `"east"`
(count `3`) and then `"west"` (the first of the three-way tie at count `1`
to appear in the original data, ahead of `"north"` and `"south"`).

Example 3. Pass `dropnull = false` to include a null case label in the tally.

```cpp
auto with_gap = Thread(pluss::make_series({1.0, 2.0, 3.0}))
                    .label().set(Thread(pluss::make_note({"10", "10", "x"}),
                                        pluss::make_series(std::vector<bool>{true, true, false})));

std::cout << with_gap.label().freq();
std::cout << with_gap.label().freq(0, false);
```

### Output (default dropnull = true)

```text
10  2

ptype: Series
```

### Output (dropnull = false)

```text
10    2
NULL  1

ptype: Series
```

## Notes

- The sort is stable: values tied at the same count keep their original
  first-occurrence relative order — this is **not** an alphabetical
  tie-break (contrast with [`DataFrame::tally`](../dataframe/tally.md),
  which does break ties alphabetically).
- `top_n` truncates *after* sorting, so it always keeps the most frequent
  values, never an arbitrary prefix of the unsorted data.

## See Also

- [`ThreadLabelAccessor::distinct`](distinct.md): the same set of unique
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
