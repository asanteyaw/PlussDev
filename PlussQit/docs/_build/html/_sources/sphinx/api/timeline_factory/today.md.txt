<!-- pluss-hand-authored -->
# pluss::timeline::today

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::today(
    MomentClock clock = MomentClock::UTC)
```

## Capture The Current Calendar Boundary

`today()` returns a rank-zero, one-element Timeline at the beginning of the
calendar date that is current when the function is called. The result is a
captured instant. It does not retain a live “today” rule and does not change at
the next midnight.

This differs from `now()`, which captures the current clock instant without
rounding to a date boundary. It is equivalent to resolving the `today`
expression with `moment()`, but avoids expression text when no offset or other
calendar word is needed.

## Choosing The Calendar Clock

`MomentClock::UTC` is the default. It obtains the current UTC date and returns
UTC midnight at its start. This is deterministic with respect to the process
timezone and is usually the right boundary for globally coordinated storage,
partitions, and protocols.

`MomentClock::Local` obtains the process-local calendar date and resolves
midnight in the process-local timezone. The resulting epoch nanoseconds include
the local UTC offset and any daylight-saving rule in effect at that midnight.
It does not merely take the local date and attach UTC midnight.

Timeline stores an absolute instant plus an explicit display-zone label. A
process-local clock does not necessarily expose a portable named-zone identifier,
so the returned local-boundary Timeline retains UTC display metadata. Project it
to a known named zone with `tz_convert()` when local wall-clock presentation is
required. The stored instant already denotes the correct process-local midnight.

## Example 1: Verify UTC Midnight Without Inventing Today's Date

```cpp
const auto day = pluss::timeline::today();
const auto tick = day.epoch_nanos().item<std::int64_t>();

std::cout << (day.dim() == 0 && day.numel() == 1 && day.tz() == "UTC") << '\n';
std::cout << (tick % (86'400LL * 1'000'000'000LL) == 0) << '\n';
```

### Output

```text
true
true
```

The invariant remains valid whenever the example runs; a hard-coded date would
not.

## Example 2: Verify Process-Local Midnight As An Instant

```cpp
const auto local_day = pluss::timeline::today(
    pluss::timeline::MomentClock::Local);
const auto seconds = static_cast<std::time_t>(
    local_day.epoch_nanos().item<std::int64_t>() / 1'000'000'000LL);

std::tm local_fields{};
#if defined(_WIN32)
localtime_s(&local_fields, &seconds);
#else
localtime_r(&seconds, &local_fields);
#endif

std::cout << (local_fields.tm_hour == 0 &&
              local_fields.tm_min == 0 &&
              local_fields.tm_sec == 0) << '\n';
```

### Output

```text
true
```

Converting the stored instant through the process-local clock is the meaningful
test. Requiring its UTC hour to be zero would incorrectly test UTC midnight.

## Example 3: Capture Once For A Consistent Window

```cpp
const auto anchor = pluss::timeline::today();
const auto first = anchor - pluss::duration::days(6);
const auto days = pluss::timeline::arange(
    first, anchor, pluss::duration::days(1));

std::cout << (days.numel() == 7) << '\n';
std::cout << (days.ticks().back() ==
              anchor.epoch_nanos().item<std::int64_t>()) << '\n';
```

### Output

```text
true
true
```

Capturing `anchor` once ensures every derived boundary refers to the same day,
even if later work crosses midnight. Fixed 24-hour durations are appropriate
for this UTC example. For named-zone calendar stepping across daylight-saving
transitions, use the calendar-aware `date_range()` day frequency.

## Operational Limits

The result depends on the host wall clock. Local mode additionally depends on
the process timezone and host timezone rules. A clock adjustment during the
call can affect which date is observed near midnight, just as it can for any
wall-clock API. Use UTC when host-local policy is not part of the requirement.

## See Also

- [`now`](now.md): capture the current instant.
- [`moment`](moment.md): resolve today, adjacent days, month boundaries, or offsets.
- [`date_range`](date_range.md): advance through calendar days in a named timezone.
