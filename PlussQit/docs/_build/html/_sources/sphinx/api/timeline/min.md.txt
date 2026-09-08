# Timeline::min

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::min(bool skipna = true) const
```

## Find The Earliest Valid Instant

`min()` searches every element and returns the earliest valid timestamp as a
scalar Timeline. It is a global reduction: a vector, matrix, or higher-rank
Timeline is reduced to `shape=[]`. There is no dimension argument and no
`keepdim` form.

`skipna` controls `NaT` propagation:

- `true` (the default) excludes `NaT` and returns the earliest valid instant.
- `false` returns scalar `NaT` if any input element is `NaT`.

The comparison uses absolute epoch nanoseconds, not formatted date strings or
array position. Input order and duplicate timestamps therefore do not change
the result. The receiver is not sorted or mutated.

## Absent-Instant Rule

With `skipna=true`, `NaT` values are skipped while at least one valid timestamp exists. If the
Timeline is empty or every element is `NaT`, `min()` returns a scalar `NaT`
instead of throwing. This gives the reduction a stable return type, but callers
that require a valid result should check `isnat_scalar()`.

## Example 1: Reduce A Matrix And Ignore `NaT`

```cpp
const auto observations = pluss::timeline::make({
    {"2025-06-15T08:30:00Z", ""},
    {"2024-01-02T18:45:00Z", "2026-09-30T03:15:00Z"},
});

pluss::timeline::print(std::cout, observations.min());
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-01-02 18:45:00
```

The earliest value sits in the second row. `min()` examines the whole matrix;
it does not reduce rows independently, and the absent first-row value is not
allowed to erase the valid result.

## Example 2: Empty And All-NaT Inputs

```cpp
const auto absent = pluss::timeline::make({"", ""});
const auto empty =
    pluss::timeline::from_epoch(std::vector<std::int64_t>{});

pluss::timeline::print(std::cout, absent.min());
std::cout << '\n';
pluss::timeline::print(std::cout, empty.min());
```

### Output

```text
Timeline(shape=[], tz=UTC)
NaT
Timeline(shape=[], tz=UTC)
NaT
```

An empty Timeline has no candidate; an all-`NaT` Timeline has no valid
candidate. Both cases deliberately converge on the same scalar `NaT` result.

## Example 3: Timezone Changes Rendering, Not The Winner

```cpp
const auto utc = pluss::timeline::make({
    "2025-06-15T08:30:00Z",
    "2024-01-02T18:45:00Z",
    "2026-09-30T03:15:00Z",
});
const auto local = utc.tz_convert("America/New_York");

pluss::timeline::print(std::cout, local.min());
```

### Output

```text
Timeline(shape=[], tz=America/New_York)
2024-01-02 13:45:00
```

The selected instant is still `2024-01-02T18:45:00Z`; the result simply keeps
the receiver's New York timezone and renders that instant locally.

## Example 4: Require A Completely Valid Input

```cpp
const auto observations = pluss::timeline::make({
    "2025-06-15T08:30:00Z", "",
    "2024-01-02T18:45:00Z",
});

pluss::timeline::print(std::cout, observations.min(false));
```

### Output

```text
Timeline(shape=[], tz=UTC)
NaT
```

Use `skipna=false` when an absent instant should invalidate the reduction
instead of being excluded.

## Choosing The Right Operation

- Use `min()` for the earliest timestamp value anywhere in the Timeline.
- Use `argsort()` when you need the positions that establish ordering.
- Use `sort()` when you need all timestamps arranged chronologically.
- Do not use `min()` to obtain the first stored element; chronological minimum
  and first position are unrelated unless the data is already sorted.

## Gotchas And Failure Modes

- The reduction is global and always returns a scalar Timeline.
- `NaT` is ignored by default and propagated when `skipna=false`.
- Ties do not expose which occurrence won because the method returns a value,
  not an index.
- The default Timeline printer displays whole seconds; the stored timestamp
  and comparison retain nanosecond precision.

## Summary

`min()` means “earliest valid instant over all elements.” It preserves temporal
type and timezone, skips `NaT` by default, can propagate it on
request, returns scalar `NaT` when no valid candidate exists, and never changes
the receiver.

## See Also

- [`max`](max.md): return the latest valid instant.
- [`argsort`](argsort.md): obtain chronological positions.
- [`isnat_scalar`](isnat_scalar.md): validate the scalar result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz_localize.html"><span class="label">Previous</span><span class="title">tz_localize</span></a>
  <a class="next-link" href="max.html"><span class="label">Next</span><span class="title">max</span></a>
</div>
<!-- pluss-hand-authored -->
