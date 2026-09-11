# Timeline::time

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::time() const
```

## Project Onto A Local Clock Value

`time()` preserves local hour, minute, second, and nanosecond fields while replacing the date with `1970-01-01` in the same timezone. The anchor keeps the result temporal and comparable, but the original date is intentionally lost.

A `Timeline` cannot contain a truly date-less clock. The anchor is therefore
part of the public representation, not an accidental implementation value.
Every defined instant has clock fields; `NaT` is the only lane without them and
remains `NaT` rather than causing the complete result to become empty.

## Result Contract

The result is a newly allocated Timeline with unchanged rank, shape, timezone, ordering, and `NaT` positions. The receiver is not modified. Scalar, vector, matrix, three-dimensional, and empty shapes are all preserved.

## Example 1: Shape And Absence

```cpp
const auto times = pluss::timeline::make({
    "2024-07-15T18:34:56.123456789Z", "",
}).tz_convert("America/Edmonton");
std::cout << pluss::timeline::print(times.time()) << '\n';
```

```text
Timeline(shape=[2], tz=America/Edmonton)
[ 1970-01-01 12:34:56, NaT ]
```

## Example 2: Timezone And Precision

```cpp
const auto scalar = pluss::timeline::make(
    "2024-07-15T18:34:56.123456789Z").tz_convert("America/Edmonton");
std::cout << scalar.time().isoformat() << '\n';
```

```text
1970-01-01T12:34:56.123456789-07:00
```

## Choosing The Projection

The compact printer hides fractional digits; `isoformat()` proves that nanosecond precision survives. Clock fields are re-localized on the anchor date because its offset may differ from the source date's offset.

Convert to the intended timezone before calling `time()` when another zone's clock fields are required. Two source dates with equal local clocks become equal anchored values.

## See Also

- [`date`](date.md)
- [`hour`](hour.md)
- [`tz_convert`](tz_convert.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ticks.html"><span class="label">Previous</span><span class="title">ticks</span></a>
  <a class="next-link" href="to_note_iso.html"><span class="label">Next</span><span class="title">to_note_iso</span></a>
</div>
<!-- pluss-hand-authored -->
