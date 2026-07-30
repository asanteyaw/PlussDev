<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11c32841eede -->

# Thread::select

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              const Thread& default_value)
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              double default_value)
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              std::string_view default_value)
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              const char* default_value)
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              const pluss::Timeline& default_value)
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              const pluss::TimeIndex& default_value)
static Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                              pluss::msharp::RangeView::Of<Thread> choices,
                              pluss::null_t default_value)
// ...and a matching std::initializer_list<Thread> overload for `conditions`/`choices`
// for every default_value type above, forwarding to the RangeView overload.
```

## What It Does

**A `static` factory method, not an instance method** — you call it as `Thread::select(...)`,
not `thread.select(...)`. It's the numpy-`select`-style construct: given a parallel list of
boolean `conditions` and value `choices`, it builds a new thread where row `i` takes the
value from the **first** `choices[k]` whose `conditions[k]` is `true` at row `i` — falling
back to `default_value` if no condition matches that row at all. Conditions are evaluated
in order, and the first match wins even if a later condition would also match.

Passing `std::initializer_list<Thread>` for `conditions`/`choices` (the common case,
`{cond1, cond2}`) is just a convenience forward to the `RangeView`-based overload —
behavior is identical either way.

```{important} All choices and the default must share one payload kind
`choices[0]`'s payload kind becomes the required kind for every other choice *and* for
`default_value` — mixing, say, a `Series` choice with a `Note` default throws. Each
`condition` must itself be a `Series`-backed `Bool`-dtype thread (the direct output of
[`compare()`](compare.md)/[`eq()`](eq.md)/etc.), and every condition must share the same
length. Each `choice` (and `default_value`) may be either scalar-like (one value,
broadcasting to every selected row) or exactly that same length.
```

## Parameters

**conditions** (`RangeView<Thread>` or `std::initializer_list<Thread>`)
    Ordered boolean masks, evaluated in order — the first `true` condition at each row
    decides which `choices` entry that row takes. At least one condition is required, and
    all conditions must share the same length.

**choices** (`RangeView<Thread>` or `std::initializer_list<Thread>`)
    Values corresponding positionally to `conditions` — `choices[k]` is used wherever
    `conditions[k]` is the first `true` condition at a row. Must have the same count as
    `conditions`, and every entry must share one payload kind (see the important callout).

**default_value** (`Thread` / `double` / `std::string_view` / `const char*` /
`pluss::Timeline` / `pluss::TimeIndex` / `pluss::null_t`)
    The value used for any row where **no** condition is `true`. Must match `choices`'
    payload kind (or be `pluss::null`, which is always accepted).

## Returns

`Thread`
    A new thread, one row per condition's length, built by picking each row's value from
    the first matching condition's corresponding choice (or `default_value`).

## Examples

Example 1. Classic multi-branch "if/elif/elif/else" grading, numpy-`select` style — first
matching condition wins.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({45.0, 65.0, 82.0, 95.0}));
auto is_low = scores.compare(60.0, ThreadBinaryCompareOp::Lt);
auto is_mid = scores.compare(80.0, ThreadBinaryCompareOp::Lt);

auto grade_f = Thread(pluss::make_note(std::vector<std::string>{"F"}));
auto grade_c = Thread(pluss::make_note(std::vector<std::string>{"C"}));

auto out = Thread::select({is_low, is_mid}, {grade_f, grade_c}, std::string_view("A"));
std::cout << out;
```

### Output

```text
0  F
1  C
2  A
3  A
ptype: Note
```

Score `82.0` is neither `< 60` nor `< 80`, so neither condition matches and it falls
through to the `"A"` default — same for `95.0`.

Example 2. Rows where no condition matches at all fall back to `default_value` — here, a
numeric `-1.0` sentinel for "unclassified."

```cpp
auto values = Thread(pluss::make_series({1.0, 2.0, 3.0}));
auto is_one = values.compare(1.0, ThreadBinaryCompareOp::Eq);
auto is_two = values.compare(2.0, ThreadBinaryCompareOp::Eq);

auto choice_one = Thread(pluss::make_series({100.0}));
auto choice_two = Thread(pluss::make_series({200.0}));

auto out = Thread::select({is_one, is_two}, {choice_one, choice_two}, -1.0);
std::cout << out;
```

### Output

```text
0  100
1  200
2  -1
ptype: Series
```

Example 3. Condition/choice count mismatch throws immediately — the two lists must be
parallel.

```cpp
auto values = Thread(pluss::make_series({1.0, 2.0}));
auto c1 = values.compare(1.0, ThreadBinaryCompareOp::Eq);
auto c2 = values.compare(2.0, ThreadBinaryCompareOp::Eq);
auto choice_one = Thread(pluss::make_series({100.0}));

try {
  auto out = Thread::select({c1, c2}, {choice_one}, -1.0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::select: condition/choice count mismatch
```

## Notes

- `Thread::select` is `static` — call it on the class (`Thread::select(...)`), not on an
  instance.
- Evaluation order matters: the **first** `true` condition at each row wins, even if a
  later condition also matches that row.
- Every `choice` and the `default_value` must agree on payload kind; every `condition` must
  be a `Bool`-dtype `Series`-backed thread of matching length. See the important callout
  above.
- [`Thread::put`](put.md) is the single-condition specialization of this same construct —
  `put(mask, value)` is exactly `Thread::select({mask}, {value}, *this)`.

## See Also

- [`Thread::put`](put.md): the single-mask, single-value specialization of `select()`.
- [`Thread::compare`](compare.md) / [`eq`](eq.md): the usual way to build each `conditions`
  entry.
- [`DataFrame::select`](../dataframe/select.md): the same numpy-`select`-style construct,
  one level up — `choices` and `default_value` are whole `DataFrame`s instead of `Thread`s.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="se.html">
    <span class="label">Previous</span>
    <span class="title">se</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
