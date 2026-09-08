<!-- pluss-hand-authored -->

# Timeline::time

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::time() const
```

## Retain The Local Time Of Day

`time()` removes each instant's calendar date and anchors its local clock value
to `1970-01-01` in the Timeline's timezone. Shape and subsecond precision are
preserved. The result is still a Timeline, so it can participate in temporal
comparison and Duration arithmetic without first becoming text.

```cpp
const auto times = pluss::timeline::make({
    "2024-11-01T09:00:00Z",
    "2024-12-31T23:59:59Z",
    "",
});
std::cout << pluss::timeline::print(times.time()) << '\n';
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 1970-01-01 09:00:00, 1970-01-01 23:59:59, NaT ]
```

The epoch date is an anchor, not retained information. Do not use the result
when the original calendar date must remain recoverable. Convert timezone
before calling `time()` when the desired clock fields belong to another zone.

## See Also

- [`Timeline::date`](date.md)
- [`Timeline::hour`](hour.md)
- [`Timeline::tz_convert`](tz_convert.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ticks.html"><span class="label">Previous</span><span class="title">ticks</span></a>
  <a class="next-link" href="to_note_iso.html"><span class="label">Next</span><span class="title">to_note_iso</span></a>
</div>
