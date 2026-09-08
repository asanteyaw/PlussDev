# Timeline::unique

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::unique() const
```

## Keep One Value From Each Encountered Group

`unique()` scans a one-dimensional Timeline from left to right and returns the
first occurrence of every distinct instant. It preserves encounter order; it
does not order the result chronologically.

Equality is based on the stored epoch-nanosecond instant. Repeated `NaT` values
form one duplicate group, so only the first absent entry survives. The result
keeps the receiver's timezone and full precision.

## Example 1: Preserve First-Seen Order

```cpp
const auto input = pluss::timeline::make({
    "2025-03-01T00:00:00Z", "", "2024-01-01T00:00:00Z",
    "2025-03-01T00:00:00Z", "", "2024-06-01T12:00:00.123456789Z",
});
std::cout << pluss::timeline::print(input.unique()) << '\n';
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2025-03-01 00:00:00, NaT, 2024-01-01 00:00:00, 2024-06-01 12:00:00 ]
```

The 2025 timestamp remains first because it was encountered first. The second
2025 timestamp and second `NaT` are removed.

## Example 2: Retain Timezone Context

```cpp
const auto local = input.tz_convert("America/New_York");
std::cout << pluss::timeline::print(local.unique()) << '\n';
```

### Output

```text
Timeline(shape=[4], tz=America/New_York)
[ 2025-02-28 19:00:00, NaT, 2023-12-31 19:00:00, 2024-06-01 08:00:00 ]
```

Timezone conversion changes wall-clock rendering, not duplicate identity.

## Example 3: Request Sorted Distinct Values

```cpp
std::cout << pluss::timeline::print(input.sort().unique()) << '\n';
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-01-01 00:00:00, 2024-06-01 12:00:00, 2025-03-01 00:00:00, NaT ]
```

Call `sort()` explicitly when chronology, rather than encounter order, is the
required output order.

## Gotchas And Failure Modes

- The receiver must be 1-D.
- `NaT` is retained once, not discarded.
- The receiver is not mutated.
- The normal printer omits fractional seconds; stored subsecond identity remains.

## Summary

`unique()` performs stable first-occurrence identity filtering, not sorting. It
treats `NaT` as one distinct absent instant and preserves timezone and precision.

## See Also

- [`sort`](sort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html"><span class="label">Previous</span><span class="title">sort</span></a>
  <a class="next-link" href="searchsorted_left.html"><span class="label">Next</span><span class="title">searchsorted_left</span></a>
</div>
