<!-- pluss-hand-authored -->

# Thread::tally

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::tally(std::int64_t top_n = 0, bool dropnull = true, bool render = false) const -> DataFrameView
```

## What It Does

Counts how often each distinct value occurs and returns the counts as a small
two-column `DataFrame` — `"value"` (the distinct value, as text) and `"count"`
(an `Int64` occurrence count) — sorted by count descending, with ties broken
alphabetically by the value's own text. Every value's string form (via the same
scalar-to-string conversion used across the library, so this works on any
payload kind, not just `Note`) becomes its grouping key.

This is the value-first counting tool for one thread: it answers "what are the
most common values in this column," rather than "how many distinct values are
there" (see [`ndistinct()`](ndistinct.md) for a plain count without the
per-value breakdown). [`DataFrame::freq()`](../dataframe/freq.md) is built on top
of this method for the single-feature case.

## Parameters

**top_n** (`std::int64_t`, default `0`)
    Keep only the `top_n` most frequent rows after sorting. `0` (the default)
    keeps every distinct value observed. Must be `>= 0`, or it throws.

**dropnull** (`bool`, default `true`)
    - `true` (the default): missing entries are excluded from the count
      entirely — they contribute no row.
    - `false`: missing entries are grouped under the literal key `"NULL"` and
      counted like any other value, competing for sort order and `top_n` the
      same as a real value would.

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only; has no effect on what an explicit `std::cout << ...` prints, since
    `DataFrameView` renders identically to a plain `DataFrame`.

## Returns

`DataFrameView`
    A two-feature frame — `"value"` and `"count"` — with one row per distinct
    value (subject to `top_n`), sorted most-frequent first and alphabetically on
    ties.

## Examples

Example 1. Count occurrences of each city.

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Accra", "Tema"}));
auto counts = cities.tally();
std::cout << counts;
```

### Output

```text
====================
     | value   count
--------------------
0    | Accra   3    
1    | Tema    2    
2    | Kumasi  1    
--------------------
[3 cs x 2 feat]
```

Example 2. A tie sorts alphabetically, and `top_n` truncates *after* sorting —
it always keeps the most frequent rows, never an arbitrary sample.

```cpp
auto tied = Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi"}));
auto top2 = tied.tally(2);
std::cout << top2;
```

### Output

```text
===================
     | value  count
-------------------
0    | Accra  2    
1    | Tema   2    
-------------------
[2 cs x 2 feat]
```

`"Accra"` and `"Tema"` are tied at `2` occurrences each; `"Accra"` sorts first
alphabetically. `"Kumasi"` (`1` occurrence) is excluded — `top_n = 2` kept only
the two most frequent rows.

Example 3. `dropnull = false` counts missing entries under the `"NULL"` key
instead of excluding them.

```cpp
auto with_gap = Thread(
    pluss::make_note({"Accra", "", "Tema"}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto with_nulls = with_gap.tally(0, false);
std::cout << with_nulls;
```

### Output

```text
===================
     | value  count
-------------------
0    | Accra  1    
1    | NULL   1    
2    | Tema   1    
-------------------
[3 cs x 2 feat]
```

All three keys tie at `1`, so they sort alphabetically — `"NULL"` (capital `N`)
lands between `"Accra"` and `"Tema"`, exactly where its text would sort among
any other value.

## Notes

- Sorting is always most-frequent first; ties break by ascending alphabetical
  order of the value's text, never by first-appearance order.
- `top_n = 0` keeps every distinct value; a positive `top_n` truncates after
  sorting, so it always keeps the most frequent entries. A negative `top_n`
  throws `"Thread::tally: top_n must be >= 0"`.
- With `dropnull = false`, the literal text `"NULL"` is just another candidate
  key — it can tie, sort, and be truncated by `top_n` like any real value.
- Works on every payload kind, since grouping is done by each value's string
  form, not by numeric equality.

## See Also

- [`DataFrame::freq`](../dataframe/freq.md): the frame-level counterpart,
  built on this method for the single-feature case and extended to
  multi-feature composite keys.
- [`Thread::ndistinct`](ndistinct.md): a plain distinct-value count, without
  the per-value frequency breakdown.
- [`Thread::distinct`](distinct.md): the distinct values themselves, in
  first-seen order rather than frequency order.
- [`Thread::describe`](describe.md): for `Note`-kind threads, reports this
  method's top value and its frequency as part of a wider summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
