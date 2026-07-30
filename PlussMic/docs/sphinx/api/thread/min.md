<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3ffd6635e9f0 -->

# Thread::min

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::min() const -> ThreadValue
```

## What It Does

Returns the smallest observed value in the thread, as a scalar `ThreadValue` —
and unlike [`sum()`](sum.md)/[`mean()`](mean.md)/[`median()`](median.md), this
works on **every** payload kind, not just numeric `Series`:

- **Numeric `Series`**: the smallest number.
- **`Note`-kind**: the alphabetically-first string.
- **`Timeline`/`TimeIndex`**: the earliest timestamp.

Missing entries are excluded first (via [`dropnull()`](dropnull.md) when a
validity mask is present). An empty thread, or one where every entry is
missing, throws.

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — the examples
below use `.to_dbl()` for numeric results and `.to_str()` for `Note`/temporal
results, rather than streaming the `ThreadValue` directly.

## Parameters

`None`

## Returns

`ThreadValue`
    A scalar holding the smallest observed value, in the source thread's own
    payload kind (a one-entry `Series`, `Note`, `Timeline`, or `TimeIndex`).

## Examples

Example 1. Smallest value in a numeric thread.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({30.0, 10.0, 50.0, 20.0, 40.0}));
auto out = scores.min();
std::cout << out.to_dbl();
```

### Output

```text
10
```

Example 2. Alphabetically-first value in a `Note`-kind thread.

```cpp
auto words = Thread(pluss::make_note({"banana", "apple", "cherry"}));
auto out2 = words.min();
std::cout << out2.to_str();
```

### Output

```text
apple
```

Example 3. Earliest timestamp in a `Timeline`-kind thread.

```cpp
auto events = Thread(pluss::make_timeline({
    "2024-03-01T00:00:00Z", "2024-01-15T00:00:00Z", "2024-02-10T00:00:00Z"
}));
auto out3 = events.min();
std::cout << out3.to_str();
```

### Output

```text
2024-01-15T00:00:00Z
```

Example 4. Missing entries are excluded before finding the minimum.

```cpp
auto with_gap = Thread(
    pluss::make_series({30.0, 5.0, 50.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out4 = with_gap.min();
std::cout << out4.to_dbl();
```

### Output

```text
30
```

The missing `5.0` — which would otherwise be the overall smallest — is
excluded entirely, so `30.0` (the smaller of the two *observed* values) wins.

Example 5. A thread with no observed values throws.

```cpp
auto all_missing = Thread(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.min();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::min on an empty (all-null) selection
```

## Notes

- Works on every payload kind — numeric `Series`, `Note`, `Timeline`,
  `TimeIndex` — unlike `sum()`/`mean()`/`median()`, which are `Series`-only.
- Missing entries are excluded first, via the same mechanism as
  [`dropnull()`](dropnull.md).
- An empty thread, or one where every entry is missing, throws `"Thread::min on
  an empty (all-null) selection"`.

## See Also

- [`Thread::max`](max.md): the largest-value counterpart.
- [`Thread::csmin`](csmin.md): the case *label* of the row holding the minimum,
  rather than the minimum value itself.
- [`Thread::bottom`](bottom.md): the `n` smallest entries, rather than just the
  single smallest.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="median.html">
    <span class="label">Previous</span>
    <span class="title">median</span>
  </a>
  <a class="next-link" href="mod.html">
    <span class="label">Next</span>
    <span class="title">mod</span>
  </a>
</div>
