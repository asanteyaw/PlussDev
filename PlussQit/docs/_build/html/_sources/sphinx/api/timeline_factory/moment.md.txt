<!-- pluss-hand-authored -->
# pluss::timeline::moment

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::moment(std::string_view expression,
                                 const MomentOptions& options = {})
Timeline pluss::timeline::moment(text_range expressions,
                                 const MomentOptions& options = {})
Timeline pluss::timeline::moment(nested_text expressions,
                                 const MomentOptions& options = {})
```

## Resolve Text At The Moment Of The Call

`moment()` turns either ordinary datetime text or a deliberately small calendar
expression into concrete epoch nanoseconds. The result stores the resolved
instant, not the expression. Calling `moment("today")` now and retaining the
returned scalar therefore preserves today's boundary; it does not change when
the value is read tomorrow.

Use `make()` when every input is datetime text. Use `moment()` when an input may
instead name a boundary relative to the current clock, such as the start of the
current month. `now()` and `today()` are shorter spellings for the two most
common scalar expressions.

The expression vocabulary is intentionally finite:

| Base | Resolved instant |
| --- | --- |
| `now` | the current system-clock instant |
| `today` | midnight at the start of the current calendar date |
| `yesterday` | midnight at the start of the preceding calendar date |
| `tomorrow` | midnight at the start of the following calendar date |
| `start_of_month` | midnight on day 1 of the current calendar month |
| `end_of_month` | midnight on the final day of the current calendar month |

Names are ASCII case-insensitive and whitespace may surround or separate their
parts. Each base may have one signed integer offset with unit `s`, `m`, `h`,
`d`, or `w`, for example `today + 6h`, `now-15m`, or `end_of_month - 1d`.
Offsets are fixed elapsed durations: `1d` is exactly 24 hours and `1w` is
exactly seven such days. They are not month arithmetic or business-calendar
rules.

## Parsing Modes

`MomentOptions::mode` decides which language is accepted:

| Mode | Accepted input |
| --- | --- |
| `MomentMode::Auto` | expressions and strict datetime text |
| `MomentMode::ExpressionOnly` | only the expression vocabulary above |
| `MomentMode::Strict` | only ordinary datetime text, with the same parsing contract as `make()` |

`Auto` is useful at an interactive boundary that genuinely accepts both forms.
Prefer `Strict` for persisted records and protocol input: accepting `today`
there would make the same record mean different instants on different runs.
Prefer `ExpressionOnly` for configuration fields that are explicitly rules,
because a misspelled rule cannot silently become ordinary datetime input.

## UTC And Local Calendar Boundaries

`MomentOptions::clock` affects calendar expressions, not absolute datetime text
and not `now`. `MomentClock::UTC` resolves `today`, `yesterday`, `tomorrow`, and
month boundaries at UTC midnight. `MomentClock::Local` first obtains the
process-local calendar date and then resolves midnight in the process-local
timezone, including the applicable daylight-saving offset.

The stored value is still an absolute instant. A local midnight can therefore
display as a non-midnight UTC clock time until the Timeline is projected to the
desired named timezone. This distinction matters: selecting the local date but
attaching UTC midnight would describe a different instant, and `moment()` does
not do that.

## Example 1: Strict Parsing Preserves Nanoseconds

```cpp
const auto fixed = pluss::timeline::moment(
    "2024-02-29T12:34:56.123456789Z",
    {pluss::timeline::MomentMode::Strict,
     pluss::timeline::MomentClock::UTC});

std::cout << pluss::note::print(
    pluss::timeline::format(fixed, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-02-29T12:34:56.123456789+00:00"
```

## Example 2: A Range Keeps Its Shape

```cpp
const std::vector<std::string> text{
    "2024-01-01", "2024-06-30T23:59:59Z"};
const auto values = pluss::timeline::moment(
    text,
    {pluss::timeline::MomentMode::Strict,
     pluss::timeline::MomentClock::UTC});

std::cout << pluss::note::print(
    pluss::timeline::format(values, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-01-01T00:00:00.000000000+00:00", "2024-06-30T23:59:59.000000000+00:00"]
```

Nested text preserves rectangular shape up to Timeline's rank-three limit in
the same way. Every lane is resolved during this call, so do not call
`moment()` separately for lanes that must share one exact `now`; capture a
scalar once and reuse it instead.

## Example 3: Verify A Dynamic Boundary Without Inventing A Date

```cpp
const auto today = pluss::timeline::moment(
    "today",
    {pluss::timeline::MomentMode::ExpressionOnly,
     pluss::timeline::MomentClock::UTC});
const auto plus_three_hours = pluss::timeline::moment(
    " TODAY + 3H ",
    {pluss::timeline::MomentMode::ExpressionOnly,
     pluss::timeline::MomentClock::UTC});

const auto exact_offset =
    plus_three_hours.epoch_nanos().item<std::int64_t>() -
    today.epoch_nanos().item<std::int64_t>();

std::cout << (today.hour().item<std::int32_t>() == 0) << '\n';
std::cout << (exact_offset == 3LL * 60 * 60 * 1'000'000'000) << '\n';
```

### Output

```text
true
true
```

This checks the contract while remaining correct on every day the example is
run. Printing a hard-coded date for `today` would be false documentation.

## Example 4: Calendar Words Describe Adjacent Boundaries

```cpp
const auto options = pluss::timeline::MomentOptions{
    pluss::timeline::MomentMode::ExpressionOnly,
    pluss::timeline::MomentClock::UTC};

const auto yesterday = pluss::timeline::moment("yesterday", options);
const auto today = pluss::timeline::moment("today", options);
const auto tomorrow = pluss::timeline::moment("tomorrow", options);
const auto first = pluss::timeline::moment("start_of_month", options);
const auto last = pluss::timeline::moment("end_of_month", options);

const auto day_ns = 86'400LL * 1'000'000'000;
std::cout << (today.epoch_nanos().item<std::int64_t>() -
                  yesterday.epoch_nanos().item<std::int64_t>() == day_ns) << '\n';
std::cout << (tomorrow.epoch_nanos().item<std::int64_t>() -
                  today.epoch_nanos().item<std::int64_t>() == day_ns) << '\n';
std::cout << (first.day().item<std::int32_t>() == 1) << '\n';
std::cout << ((last + pluss::duration::days(1)).day().item<std::int32_t>() == 1)
          << '\n';
```

### Output

```text
true
true
true
true
```

## Failures And Limits

`moment()` throws when an expression has an unknown base, more than one
offset, a fractional or missing offset magnitude, an unknown unit, or an
offset that exceeds the representable nanosecond range. `Strict` throws on an
expression, while `ExpressionOnly` throws on ordinary datetime text. Invalid
calendar text follows the same rejection rules as `make()`.

The local clock mode depends on the process timezone and the host timezone
database. Use UTC when reproducibility across machines is more important than
the host's civil calendar. Use a local expression only when process-local
midnight is specifically the boundary you mean.

## See Also

- [`now`](now.md): capture the current instant without expression text.
- [`today`](today.md): capture the start of the current UTC or local date.
- [`make`](make.md): parse datetime text without relative expressions.
