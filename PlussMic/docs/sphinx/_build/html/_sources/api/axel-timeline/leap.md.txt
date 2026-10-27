<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 167e26f0b893 -->

# AxelTimelineAccessor::leap

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::leap() const
```

## What It Does

Flags whether each bound field's timestamp's calendar year is a leap
year, in every registry frame, replacing each `Timeline` field with a
`Bool` `Series` field. Applies
[`RegistryTimelineAccessor::leap`](../registry-timeline/leap.md) to
every frame via [`Axel::map`](../axel/map.md).

```{important} Reflects the row's YEAR, not whether the date is February 29th
`leap()` answers "is this row's *year* a leap year," not "is this row
February 29th." Any row dated anywhere in 2024 flags `true`, since 2024
as a whole is a leap year — see Example 1, where a January date and a
February-29th date both flag `true` for the same reason.
```

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field: `true` when the row's calendar year is a
    leap year, `false` otherwise.

## Examples

Example 1. Two timestamps from the same leap year both flag `true`,
even though only one falls on February 29th.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().leap();
auto ts = out.registry(0).fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<bool>()[0];
```

### Output

```text
true true
```

Example 2. A 2024 timestamp flags `true`; a 2023 timestamp flags
`false`, since 2023 isn't a leap year.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().leap();
auto ts = out.registry(0).fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<bool>()[0];
```

### Output

```text
true false
```

## Notes

- See the important callout above — reflects the row's year, not its
  exact date.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::month_end`](month_end.md): where leap-year
  status actually changes February's last valid day.
- [`AxelTimelineAccessor::yearday`](yearday.md): where leap years shift
  every day-of-year number from March onward.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
