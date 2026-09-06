# Timeline::week

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::week() const
```

Aliases: `weekofyear()`, `woy()`, and `week_()`.

## Read The ISO Week Number

`week()` returns the ISO week number of each local date as shape-preserving
`int32`. ISO weeks begin on Monday and are numbered `1` through `53`. Week 1 is
the week containing the year's first Thursday, so the week-number boundary
does not always coincide with 1 January. Missing timestamps return `-1`.

This convention explains an initially surprising but correct result: Friday,
1 January 2021 belongs to ISO week 53 of the preceding ISO week-year.

## Example: A Civil-Year Boundary Inside One ISO Week

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.week());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[53, 53, 9, -1]
```

The first two timestamps have different `year()` values but the same ISO week
number. That contrast is the central rule to remember.

## Constructing Week-Based Keys Safely

Do not assume `(year(), week())` is a valid ISO year-week key around New Year:
the civil year returned by `year()` and the ISO week-numbering year can differ.
If your application needs a globally unambiguous weekly key, define and retain
the week boundary itself, or explicitly compute the matching week-year under
the application's calendar policy.

For weekday position within the ISO week, `weekday()` uses the compatible
Monday-zero convention.

## Gotchas And Failure Modes

- Weeks begin on Monday, not Sunday.
- A year can contain week `53`.
- The local timezone determines the date and therefore the week.
- `week()` returns only the number, not the corresponding week-year.
- `-1` means the source was `NaT`.

## Summary

Use `week()` for ISO-style weekly grouping after choosing the correct local
timezone. Treat New Year as a real calendar boundary case, not an exception to
ignore, and never pair the number blindly with the civil year.

## See Also

- [`weekday`](weekday.md)
- [`year`](year.md)
- [`floor`](floor.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html"><span class="label">Previous</span><span class="title">quarter</span></a>
  <a class="next-link" href="is_leap_year.html"><span class="label">Next</span><span class="title">is_leap_year</span></a>
</div>
<!-- pluss-hand-authored -->
