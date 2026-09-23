<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 421d90862373 -->

# Thread::freq

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::freq() const -> Thread
Thread::freq(ThreadFreqOptions options) const -> Thread
```

## What It Does

Counts how often each distinct value occurs in the thread, and returns the
counts as a new `Thread` whose case labels are the distinct source values —
the 1D value-count surface. Counts are sorted by frequency descending by
default; ties keep the values' original **first-occurrence** relative order
(a stable sort), not alphabetical order.

Use [`DataFrame::freq`](../dataframe/freq.md) for grouped frequencies across
one or more `DataFrame` features instead of a single thread.

## Parameters

**options** (`ThreadFreqOptions`, optional)

```cpp
struct ThreadFreqOptions {
  ThreadFreqKind kind{ThreadFreqKind::Count};
  bool dropnull{true};
  bool sort{true};
  bool ascending{false};
  std::int64_t top{0};
};
```

**options.kind** (`ThreadFreqKind`, default `Count`)

`Count` returns raw occurrence counts (`Int64`). `Prop` returns each value's
count divided by the total number of counted rows (`Float64`).

**options.dropnull** (`bool`, default `true`)

When `true`, a missing (invalid) row is excluded entirely before counting.
When `false`, missing rows are tallied together under the label `"NULL"`.

**options.sort** (`bool`, default `true`)

When `true`, results are ordered by count, descending unless `ascending` is
set. When `false`, results stay in first-occurrence order and `ascending` has
no effect.

**options.ascending** (`bool`, default `false`)

Only applies when `sort = true`. `true` sorts smallest-count-first instead of
the default largest-first.

**options.top** (`std::int64_t`, default `0`)

Keeps only the first `top` rows after ordering. `0` keeps every distinct
value. Must be `>= 0` — a negative `top` throws `"Thread::freq: top must be
>= 0"`.

## Returns

`Thread`
    A numeric thread — `Int64` when `kind = Count`, `Float64` when
    `kind = Prop` — with the distinct source values as its case labels, named
    `"count"` or `"prop"` to match `kind`.

## Examples

Example 1. Default counts, sorted descending, for a `Note`-kind thread with a
repeated value.

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Tema", "Accra"}));
auto counts = cities.freq();
std::cout << counts;
```

### Output

```text
Accra  3
Tema   2

Name: count, ptype: Series
```

`"Accra"` occurs three times, `"Tema"` twice — no tie to break here, but see
Example 4 for how ties are actually resolved.

Example 2. `ThreadFreqKind::Prop` returns proportions instead of raw counts.

```cpp
auto prop = cities.freq({.kind = ThreadFreqKind::Prop});
std::cout << prop;
```

### Output

```text
Accra  0.6
Tema   0.4

Name: prop, ptype: Series
```

Example 3. Keep missing values in the tally (under `"NULL"`) and skip
sorting, so the result stays in first-occurrence order.

```cpp
auto with_gap = Thread(
    pluss::make_note({"Accra", "x", "Tema"}),
    pluss::make_series(std::vector<bool>{true, false, true}));

auto out = with_gap.freq({.dropnull = false, .sort = false});
std::cout << out;
```

### Output

```text
Accra  1
NULL   1
Tema   1

Name: count, ptype: Series
```

With `sort = false`, the three (here all tied at `1`) rows print in the
order their values were first encountered, not by any count-based ordering.

Example 4. A genuine tie, to show the tie-break is first-occurrence order,
not alphabetical.

```cpp
auto regions = Thread(pluss::make_note({"east", "west", "east", "north"}));
auto tied = regions.freq();
std::cout << tied;
```

### Output

```text
east   2
west   1
north  1

Name: count, ptype: Series
```

`"west"` and `"north"` are both tied at count `1`. `"west"` (first seen at
row `1`) comes before `"north"` (first seen at row `3`) — alphabetically
`"north"` would sort first, but that's not what happens here.

## Notes

- The default sort is stable: values tied at the same count keep their
  original first-occurrence relative order. This is different from
  [`DataFrame::tally`](../dataframe/tally.md), which breaks ties
  alphabetically by key.
- `top` truncates *after* sorting (when `sort = true`), so it keeps the
  highest-count rows, never an arbitrary prefix of the unsorted data.
- A negative `top` throws.
- [`Thread::describe()`](describe.md) uses `freq()` internally to compute a
  `Note`-kind thread's `top`/`freq` summary fields, and inherits this same
  tie-break behavior.

## See Also

- [`DataFrame::freq`](../dataframe/freq.md): the grouped, multi-feature
  counterpart.
- [`Thread::distinct`](distinct.md): the same set of unique values, without
  counts, in first-occurrence order.
- [`Thread::describe`](describe.md): a fuller summary that uses `freq()`
  internally for `Note`-kind threads.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fold.html">
    <span class="label">Previous</span>
    <span class="title">fold</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
