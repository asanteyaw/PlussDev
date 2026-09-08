<!-- pluss-hand-authored -->
# pluss::timeline::now

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::now()
```

## Capture One Absolute Instant

`now()` reads the system clock once and returns that reading as a rank-zero,
one-element Timeline with UTC timezone metadata. It is the direct choice for a
measurement boundary, event timestamp, or reproducible cutoff captured at the
call site.

There is no local-versus-UTC argument. An instant is absolute; changing a
timezone changes only its civil representation. Use `tz_convert()` when the
same captured instant should be displayed in another zone. Use `today()` when
the desired value is a calendar midnight rather than the current instant.

## Example 1: Verify The Clock Capture

Dynamic examples must test bounds rather than claim a fixed timestamp:

```cpp
const auto before = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
const auto observed = pluss::timeline::now();
const auto after = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

const auto tick = observed.epoch_nanos().item<std::int64_t>();
std::cout << (observed.dim() == 0 && observed.numel() == 1 &&
              observed.tz() == "UTC") << '\n';
std::cout << (before <= tick && tick <= after) << '\n';
```

### Output

```text
true
true
```

The bound assumes the system clock is not adjusted backward between the three
reads, which is the same operational limitation as any wall-clock timestamp.
`now()` is not a monotonic elapsed-time clock.

## Example 2: Capture Once When Consistency Matters

```cpp
const auto batch_time = pluss::timeline::now();
const auto opened_at = batch_time;
const auto audit_time = batch_time;

std::cout << opened_at.eq(audit_time).item<bool>() << '\n';
```

### Output

```text
true
```

Two separate calls are two separate clock reads and normally differ. Reuse one
captured scalar when multiple records must carry an identical boundary.

## Example 3: Project The Same Instant For Display

```cpp
const auto captured = pluss::timeline::now();
const auto local_view = captured.tz_convert("America/Edmonton");

std::cout << (captured.epoch_nanos().item<std::int64_t>() ==
              local_view.epoch_nanos().item<std::int64_t>()) << '\n';
```

### Output

```text
true
```

The timezone conversion changes wall-clock fields and formatting, not the
captured epoch nanoseconds.

## See Also

- [`moment`](moment.md): resolve `now` with an optional fixed elapsed offset.
- [`today`](today.md): resolve a UTC or process-local calendar midnight.
- [`format`](format.md): produce shaped text from a captured Timeline.
