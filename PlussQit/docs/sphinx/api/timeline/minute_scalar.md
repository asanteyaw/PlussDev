<!-- pluss-hand-authored -->

# Timeline::minute_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::minute_scalar() const
```

## Read One Minute Within The Hour

`minute_scalar()` extracts the local clock's minute field, `0..59`, from one
rank-zero Timeline. It is a component, not a duration: `30` means the instant
is thirty minutes into its displayed hour, not that it is thirty minutes from
another instant.

The method is useful where a single C++ integer is the desired result. Its
array counterpart, [`minute()`](minute.md), returns a shape-preserving `Series`.
No implicit squeeze occurs, so shape `[1]` remains non-scalar and is rejected.

## Example: Clock Components Remain Separate

```cpp
const auto t = pluss::timeline::make("2025-01-01T01:30:45Z");
std::cout << "hour=" << t.hour_scalar()
          << ", minute=" << t.minute_scalar()
          << ", second=" << t.second_scalar() << '\n';
```

### Output

```text
hour=1, minute=30, second=45
```

## Example: Timezone Conversion

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:45Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << "UTC=" << utc.minute_scalar()
          << ", Edmonton=" << local.minute_scalar() << '\n';
```

### Output

```text
UTC=30, Edmonton=30
```

Here the offset is an exact number of hours, so the minute happens to remain
unchanged. That is a result of this zone and instant, not a promise of timezone
conversion; zones with sub-hour offsets can change the minute field.

## Example: Absent Instant

```cpp
std::cout << pluss::timeline::make("").minute_scalar() << '\n';
```

### Output

```text
-1
```

## Gotchas And Failure Modes

- Valid output is `0..59`; `-1` denotes `NaT`.
- Rank-one and higher Timelines throw, even with one element.
- The method reads local civil time and does not round the instant.
- Nanoseconds below the second remain intact in the receiver.

## See Also

- [`minute()`](minute.md): shape-preserving extraction.
- [`minutes_between()`](minutes_between.md): elapsed whole-minute distance.
- [`set_minute()`](set_minute.md): replace the local minute field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html"><span class="label">Previous</span><span class="title">minute</span></a>
  <a class="next-link" href="minutes_between.html"><span class="label">Next</span><span class="title">minutes_between</span></a>
</div>
