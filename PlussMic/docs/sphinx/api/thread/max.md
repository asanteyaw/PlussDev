<!-- pluss-hand-authored -->

# Thread::max

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::max() const -> ThreadValue
```

## What It Does

Returns the largest observed value in the thread, as a scalar `ThreadValue` —
the mirror image of [`min()`](min.md), and just like `min()`, this works on
**every** payload kind:

- **Numeric `Series`**: the largest number.
- **`Note`-kind**: the alphabetically-last string.
- **`Timeline`/`TimeIndex`**: the latest timestamp.

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
    A scalar holding the largest observed value, in the source thread's own
    payload kind (a one-entry `Series`, `Note`, `Timeline`, or `TimeIndex`).

## Examples

Example 1. Largest value in a numeric thread.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({30.0, 10.0, 50.0, 20.0, 40.0}));
auto out = scores.max();
std::cout << out.to_dbl();
```

### Output

```text
50
```

Example 2. Alphabetically-last value in a `Note`-kind thread.

```cpp
auto words = Thread(pluss::make_note({"banana", "apple", "cherry"}));
auto out2 = words.max();
std::cout << out2.to_str();
```

### Output

```text
cherry
```

Example 3. Latest timestamp in a `Timeline`-kind thread.

```cpp
auto events = Thread(pluss::make_timeline({
    "2024-03-01T00:00:00Z", "2024-01-15T00:00:00Z", "2024-02-10T00:00:00Z"
}));
auto out3 = events.max();
std::cout << out3.to_str();
```

### Output

```text
2024-03-01T00:00:00Z
```

Example 4. Missing entries are excluded before finding the maximum.

```cpp
auto with_gap = Thread(
    pluss::make_series({30.0, 100.0, 50.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out4 = with_gap.max();
std::cout << out4.to_dbl();
```

### Output

```text
50
```

The missing `100.0` — which would otherwise be the overall largest — is
excluded entirely, so `50.0` (the larger of the two *observed* values) wins.

Example 5. A thread with no observed values throws.

```cpp
auto all_missing = Thread(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.max();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::max on an empty (all-null) selection
```

## Notes

- Works on every payload kind — numeric `Series`, `Note`, `Timeline`,
  `TimeIndex` — unlike `sum()`/`mean()`/`median()`, which are `Series`-only.
- Missing entries are excluded first, via the same mechanism as
  [`dropnull()`](dropnull.md).
- An empty thread, or one where every entry is missing, throws `"Thread::max on
  an empty (all-null) selection"`.

## See Also

- [`Thread::min`](min.md): the smallest-value counterpart.
- [`Thread::csmax`](csmax.md): the case *label* of the row holding the maximum,
  rather than the maximum value itself.
- [`Thread::top`](top.md): the `n` largest entries, rather than just the
  single largest.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matmul.html">
    <span class="label">Previous</span>
    <span class="title">matmul</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
