# Timeline::set_hour

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::set_hour(int hour) const
```

## Replace The Local Hour

`set_hour()` replaces the hour in every defined element's local wall clock.
The accepted range is `[0, 23]`; `24` is not an alternate spelling for the
following midnight. Date, minute, second, nanoseconds, timezone, rank, and
shape remain unchanged.

Use this method when the hour itself is the rule, for example to place events
at local 06:xx while retaining their existing minutes and seconds. Use elapsed
duration arithmetic when the rule is instead "six hours later."

## Example: Retain The Smaller Clock Fields

```cpp
const auto t = pluss::timeline::make({
    {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
    {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
});

std::cout << pluss::note::print(
    pluss::timeline::format(t.set_hour(6), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-15T06:58:59.123456789+00:00", "2024-06-20T06:05:06.000000004+00:00"], 
  ["2024-09-28T06:30:45.900000000+00:00", "2024-12-01T06:00:00.000000001+00:00"]
]
```

The method changes `23`, `04`, `12`, and `00` to `06`; it does not zero the
smaller fields. Chain `set_minute()` and `set_second()` when the complete clock
must be replaced.

## Example: Local Time Is Not A Fixed UTC Shift

```cpp
const auto ny = pluss::timeline::make("2024-07-01T16:45:00Z")
                    .tz_convert("America/New_York");
std::cout << pluss::timeline::format(ny.set_hour(9), "%F %T %z").at(0) << '\n';
```

### Output

```text
2024-07-01 09:45:00 -04:00
```

The source displays as local `12:45`; replacement makes it local `09:45`.
The operation edits the wall clock under the retained timezone rather than
subtracting three hours from every UTC tick by definition.

## Daylight-Saving Gaps

Some local hours do not exist. Replacement is strict in that case:

```cpp
try {
  const auto ny = pluss::timeline::make("2024-03-10T06:30:00Z")
                      .tz_convert("America/New_York");
  (void)ny.set_hour(2);
} catch (const pluss::Error&) {
  std::cout << "nonexistent local time rejected\n";
}
```

### Output

```text
nonexistent local time rejected
```

On that date the local clock jumps from `01:59:59` to `03:00:00`, so `02:30`
cannot be represented. The method does not shift it forward silently. During a
fall-back overlap, the first occurrence is selected. `NaT` remains `NaT`.

## See Also

- [`hour`](hour.md): inspect the local hour.
- [`set_minute`](set_minute.md): replace the minute field.
- [`tz_convert`](tz_convert.md): choose the local calendar before editing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_day.html"><span class="label">Previous</span><span class="title">set_day</span></a>
  <a class="next-link" href="set_minute.html"><span class="label">Next</span><span class="title">set_minute</span></a>
</div>
<!-- pluss-hand-authored -->
