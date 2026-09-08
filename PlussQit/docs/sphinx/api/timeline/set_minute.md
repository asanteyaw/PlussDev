# Timeline::set_minute

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::set_minute(int minute) const
```

## Replace One Clock Field

`set_minute()` replaces the local minute of every defined timestamp with a
value in `[0, 59]`. It returns a new `Timeline`; the receiver is not modified.
Year, month, day, hour, second, fractional nanoseconds, timezone, rank, and
shape are retained.

Use it when the minute number is the desired invariant, such as moving a set
of local appointment times to quarter past their existing hours. It is not a
minute offset: `set_minute(15)` means `HH:15`, not "15 minutes later."

## Example: Set Every Lane To Quarter Past

```cpp
const auto t = pluss::timeline::make({
    {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
    {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
});

std::cout << pluss::note::print(
    pluss::timeline::format(t.set_minute(15), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-15T23:15:59.123456789+00:00", "2024-06-20T04:15:06.000000004+00:00"], 
  ["2024-09-28T12:15:45.900000000+00:00", "2024-12-01T00:15:00.000000001+00:00"]
]
```

Seconds and nanoseconds are deliberately nonzero in this example. They prove
that `set_minute()` edits only its named field rather than rounding to minute
precision.

## Example: Compose A Complete Local Clock

```cpp
const auto t = pluss::timeline::make("2024-08-19T21:47:36.250000001Z");
const auto opening = t.set_hour(9).set_minute(30).set_second(0);

std::cout << pluss::timeline::format(opening, "%FT%T.%f%z").at(0) << '\n';
```

### Output

```text
2024-08-19T09:30:00.250000001+00:00
```

Chaining is evaluated from left to right, and every call returns a new
timeline. The fractional component remains `250000001` because none of these
three methods replaces nanoseconds.

## Bounds, Absence, And Timezones

`-1` and `60` throw; there is no wrapping into the adjacent hour. Validation
also occurs for empty input. `NaT` lanes remain `NaT`.

The minute is interpreted in the timeline's current timezone. Although most
timezone transitions occur on an hour boundary, the complete reconstructed
local timestamp is still checked. A nonexistent local result throws, and the
first occurrence is selected if the result is ambiguous.

## See Also

- [`minute`](minute.md): inspect the local minute.
- [`set_hour`](set_hour.md): replace the enclosing hour.
- [`set_second`](set_second.md): replace seconds without losing nanoseconds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_hour.html"><span class="label">Previous</span><span class="title">set_hour</span></a>
  <a class="next-link" href="set_month.html"><span class="label">Next</span><span class="title">set_month</span></a>
</div>
<!-- pluss-hand-authored -->
