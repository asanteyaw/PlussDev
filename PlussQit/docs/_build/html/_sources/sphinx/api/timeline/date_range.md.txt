# pluss::timeline::date_range

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
pluss::timeline::date_range(const Timeline& start, const Timeline& end, std::string_view freq, std::int64_t step = 1) -> Timeline
```

Builds a regular temporal sequence between two scalar endpoints.

This is the constructor-style temporal grid builder. Use it when you want to
create the timeline first and attach external data later, or when a model,
index, or schedule needs a clean regular backbone.

## Parameters

**start**

The scalar lower endpoint of the range.

The generated sequence begins here.

**end**

The scalar upper endpoint of the range.

The sequence advances until it reaches this endpoint under the supplied
frequency and step configuration.

**freq**

The base frequency unit.

Common practical values include:

- `"D"`
  Day spacing.
- `"H"`
  Hour spacing.
- `"min"`
  Minute spacing.
- `"S"`
  Second spacing.

Choose `freq` based on the natural unit of the grid you are building. For
example:

- daily calendars use `"D"`
- hourly schedules use `"H"`
- intraday sampling grids often use `"min"` or `"S"`

**step**

How many `freq` units to advance each time.

The default `step = 1` means every unit. Larger values coarsen the grid:

- `15` with `freq = "min"` gives a 15-minute grid
- `2` with `freq = "H"` gives a 2-hour grid

## Returns

`Timeline`
    A regular temporal sequence running from `start` toward `end` using the
    requested frequency and step.

## Notes

- `date_range(...)` returns a `Timeline`, not a `TimeIndex`.
- Promote the result to `TimeIndex` when the range is going to become an
  ordered lookup axis.
- `start` and `end` are scalar endpoints. This is not a pairwise
  vectorized-range constructor.
- The main toggle in practice is the combination of `freq` and `step`.

## Examples

Example 1. Build a daily calendar range.

```cpp
auto out = pluss::timeline::date_range(
    pluss::timeline::make("2026-03-14T00:00:00Z"),
    pluss::timeline::make("2026-03-17T00:00:00Z"),
    "D");
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2026-03-14 00:00:00, 2026-03-15 00:00:00, 2026-03-16 00:00:00, 2026-03-17 00:00:00 ]
```

Example 2. Keep the same endpoint style but switch to a 15-minute intraday
grid by changing `freq` and `step`.

```cpp
auto out = pluss::timeline::date_range(
    pluss::timeline::make("2026-03-14T09:00:00Z"),
    pluss::timeline::make("2026-03-14T10:00:00Z"),
    "min",
    15);
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2026-03-14 09:00:00, 2026-03-14 09:15:00, 2026-03-14 09:30:00, 2026-03-14 09:45:00, 2026-03-14 10:00:00 ]
```

Example 3. Use hour units with a coarser step to build a 2-hour schedule.

```cpp
auto out = pluss::timeline::date_range(
    pluss::timeline::make("2026-03-14T00:00:00Z"),
    pluss::timeline::make("2026-03-14T06:00:00Z"),
    "H",
    2);
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2026-03-14 00:00:00, 2026-03-14 02:00:00, 2026-03-14 04:00:00, 2026-03-14 06:00:00 ]
```

## See Also

- [`TimeIndex API Reference`](../../timeindex_api_reference.md)
- [`Timeline::floor`](floor.md)
- [`Timeline::searchsorted_left`](searchsorted_left.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_sunday.html">
    <span class="label">Previous</span>
    <span class="title">is_sunday</span>
  </a>
  <a class="next-link" href="../../timeline_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Timeline API Reference</span>
  </a>
</div>
