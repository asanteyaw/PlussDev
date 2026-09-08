# Timeline::max

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::max(bool skipna = true) const
```

## Find The Latest Valid Instant

`max()` searches the entire Timeline and returns the latest valid timestamp as
a scalar Timeline. All dimensions participate in one reduction, so the result
has `shape=[]` regardless of receiver rank. The method has no axis-wise or
shape-retaining overload.

`skipna` controls `NaT` propagation:

- `true` (the default) excludes `NaT` and returns the latest valid instant.
- `false` returns scalar `NaT` if any input element is `NaT`.

Chronology is determined from absolute epoch nanoseconds. Input order,
formatted timezone offsets, and duplicates do not affect which instant is
greatest. `max()` does not reorder or mutate the receiver.

## Absent-Instant Rule

With `skipna=true`, `NaT` positions are ignored whenever at least one valid instant exists.
An empty or all-`NaT` receiver returns scalar `NaT`. Use `isnat_scalar()` when
an absent maximum must trigger fallback or validation logic.

## Example 1: Reduce Every Matrix Element

```cpp
const auto observations = pluss::timeline::make({
    {"2025-06-15T08:30:00Z", ""},
    {"2024-01-02T18:45:00Z", "2026-09-30T03:15:00Z"},
});

pluss::timeline::print(std::cout, observations.max());
```

### Output

```text
Timeline(shape=[], tz=UTC)
2026-09-30 03:15:00
```

The result is one scalar, not one maximum per row or column. The receiver
remains `[2,2]` after the call.

## Example 2: Duplicates Do Not Change The Value

```cpp
const auto duplicated = pluss::timeline::make({
    "2025-06-15T08:30:00Z",
    "2024-01-02T18:45:00Z",
    "2026-09-30T03:15:00Z",
    "2026-09-30T03:15:00Z",
});

pluss::timeline::print(std::cout, duplicated.max());
```

### Output

```text
Timeline(shape=[], tz=UTC)
2026-09-30 03:15:00
```

Both latest entries represent the same value. Because `max()` returns the
timestamp rather than its position, tie order is intentionally irrelevant.

## Example 3: Empty And All-NaT Inputs

```cpp
const auto absent = pluss::timeline::make({"", ""});
const auto empty =
    pluss::timeline::from_epoch(std::vector<std::int64_t>{});

pluss::timeline::print(std::cout, absent.max());
std::cout << '\n';
pluss::timeline::print(std::cout, empty.max());
```

### Output

```text
Timeline(shape=[], tz=UTC)
NaT
Timeline(shape=[], tz=UTC)
NaT
```

## Example 4: Preserve The Receiver Timezone

```cpp
const auto utc = pluss::timeline::make({
    "2025-06-15T08:30:00Z",
    "2024-01-02T18:45:00Z",
    "2026-09-30T03:15:00Z",
});
const auto local = utc.tz_convert("America/New_York");

pluss::timeline::print(std::cout, local.max());
```

### Output

```text
Timeline(shape=[], tz=America/New_York)
2026-09-29 23:15:00
```

The local date is September 29 even though the UTC rendering is September 30.
Both identify the same latest instant; timezone retention changes presentation
and local calendar fields, not ordering.

## Example 5: Propagate Any NaT

```cpp
const auto observations = pluss::timeline::make({
    "2025-06-15T08:30:00Z", "",
    "2026-09-30T03:15:00Z",
});

pluss::timeline::print(std::cout, observations.max(false));
```

### Output

```text
Timeline(shape=[], tz=UTC)
NaT
```

This mode is useful when “latest” is meaningful only for a complete set of
observations.

## Choosing The Right Operation

- Use `max()` for the latest valid timestamp value.
- Use `argsort()` when the winning position matters.
- Use `sort()` when later work needs the complete chronological sequence.
- Do not confuse the maximum with the last stored element in unsorted data.

## Gotchas And Failure Modes

- Reduction is global; no dimension argument is accepted.
- `NaT` is skipped by default and propagated when `skipna=false`.
- The returned timezone matches the receiver.
- The scalar result remains a Timeline, so calendar extraction, formatting,
  and timezone conversion remain available.

## Summary

`max()` means “latest valid instant over all elements.” It returns a scalar,
retains timezone and nanosecond identity, ignores `NaT` positions by default,
can propagate them on request, and leaves the source Timeline unchanged.

## See Also

- [`min`](min.md): return the earliest valid instant.
- [`argsort`](argsort.md): obtain chronological positions.
- [`isnat_scalar`](isnat_scalar.md): detect an absent maximum.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html"><span class="label">Previous</span><span class="title">min</span></a>
  <a class="next-link" href="sort.html"><span class="label">Next</span><span class="title">sort</span></a>
</div>
<!-- pluss-hand-authored -->
