<!-- pluss-hand-authored -->

# Timeline::from_ymdhms

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
static Timeline Timeline::from_ymdhms(
    const Series& year,
    const Series& month,
    const Series& day,
    const Series& hour,
    const Series& minute,
    const Series& second,
    std::string_view tz_name = "UTC")
```

## Assemble Local Date And Clock Fields

`from_ymdhms()` extends [`from_ymd()`](from_ymd.md) with whole-second clock
components. It forms each local civil timestamp in `tz_name`, resolves that
timestamp to UTC storage, and preserves the shared component shape.

```cpp
const auto year = pluss::make_series(2024LL);
const auto month = pluss::make_series({2LL, 3LL});
const auto day = pluss::make_series({29LL, 1LL});
const auto hour = pluss::make_series({6LL, 7LL});
const auto minute = pluss::make_series(30LL);
const auto second = pluss::make_series(45LL);

const auto times = pluss::Timeline::from_ymdhms(
    year, month, day, hour, minute, second, "UTC");
std::cout << pluss::timeline::print(times) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-02-29 06:30:45, 2024-03-01 07:30:45 ]
```

Here `year`, `minute`, and `second` are scalars and broadcast; the other
components establish shape `[2]`. Valid clock ranges are hour `0..23`, minute
`0..59`, and second `0..59`. Invalid fields throw. If any component at a
position is `Timeline::kNaT`, that position becomes `NaT`.

The constructor has whole-second precision. Begin with epoch nanoseconds when
subsecond fields must be represented exactly.

## Timezone Boundaries

Components describe local wall time, not UTC fields wearing a timezone label.
Offset rules therefore affect the stored epoch tick. Ambiguous and nonexistent
local clocks are resolved by the constructor's timezone policy; avoid such
wall times when an explicit fold or gap policy is required.

## See Also

- [`Timeline::from_ymd`](from_ymd.md)
- [`Timeline::from_epoch_seconds`](from_epoch_seconds.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_ymd.html"><span class="label">Previous</span><span class="title">from_ymd</span></a>
  <a class="next-link" href="operators.html"><span class="label">Next</span><span class="title">operators</span></a>
</div>
