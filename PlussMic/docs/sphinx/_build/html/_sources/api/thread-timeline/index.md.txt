<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::index

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::index(bool require_sorted = true) const -> Thread
```

## What It Does

Replaces a `Timeline`-kind bound thread with an equivalent `TimeIndex` — same
length, same displayed values. `TimeIndex` is the monotonic time-axis type
other parts of the library (like duration-based `.ts().resample()`/
`.ts().rolling(duration)`) expect. A thread already `TimeIndex`-kind is passed
through unchanged.

## Parameters

**require_sorted** (`bool`, default `true`)

When `true` (the default), every value must be non-decreasing and non-`NaT`,
or the call throws. When `false`, the conversion happens without checking
order or `NaT` — but the result may then misbehave in code elsewhere that
assumes a genuinely monotonic `TimeIndex`.

## Returns

`Thread`
    A new `TimeIndex`-kind thread, same length as the bound thread.

## Examples

Example 1. Convert an already-sorted `Timeline` thread to `TimeIndex`.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-01T08:00:00Z",
    "2024-01-02T08:00:00Z",
    "2024-01-03T08:00:00Z",
}));
auto out = events.tl().index();
std::cout << out;
```

### Output

```text
0  2024-01-01 08:00:00
1  2024-01-02 08:00:00
2  2024-01-03 08:00:00

ptype: TimeIndex
```

The printed values look identical to the `Timeline` input — `TimeIndex` renders
exactly like `Timeline` — but the thread's underlying kind is now `TimeIndex`.

Example 2. Call `index()` on an out-of-order thread with the default
`require_sorted = true`, to see it throw.

```cpp
auto unsorted = Thread(pluss::make_timeline({
    "2024-01-03T00:00:00Z",
    "2024-01-01T00:00:00Z",
    "2024-01-02T00:00:00Z",
}));
try {
  auto bad = unsorted.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

Example 3. Call `index(false)` on the same out-of-order data, to see it pass
through without checking.

```cpp
auto out3 = unsorted.tl().index(false);
std::cout << out3;
```

### Output

```text
0  2024-01-03
1  2024-01-01
2  2024-01-02

ptype: TimeIndex
```

With `require_sorted = false`, the same three out-of-order rows convert
successfully — the ordering check is skipped entirely, and the rows keep
their original order.

Example 4. Call `index()` on a thread containing `NaT`, to see the distinct
"does not allow NaT" throw.

```cpp
auto with_nat = Thread(pluss::make_timeline({
    "2024-01-01T00:00:00Z", "", "2024-01-03T00:00:00Z"}));
try {
  auto bad2 = with_nat.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
TimeIndex does not allow NaT
```

## Notes

- With the default `require_sorted = true`, calling this on an *unsorted*
  thread (or one containing `NaT`) throws immediately — sort with
  [`Thread::arrange`](../thread/arrange.md) (or an equivalent) first if
  needed.
- `require_sorted = false` skips that check entirely — use it only when the
  order genuinely doesn't matter downstream, since duration-based operations
  generally *do* assume a monotonic axis.
- A thread that's already `TimeIndex`-kind is returned as-is, regardless of
  `require_sorted`.
- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.

## See Also

- [`DataFrame::ts`](../dataframe/ts.md): the accessor whose duration-based
  methods (`resample`, `rolling(duration)`, and similar) expect a `TimeIndex`
  axis like the one this method produces.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
