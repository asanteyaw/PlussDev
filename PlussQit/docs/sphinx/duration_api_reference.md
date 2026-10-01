<!-- pluss-hand-authored -->

# Duration API Reference

[Back to PlussQit API Reference](../api_reference.md)

[Open the Duration Factory API Reference](duration_factory_api_reference.md)

This page documents `pluss::Duration` one method at a time, with complete
behavioral prose and executable-derived outputs.

## What `Duration` Is

`Duration` is the elapsed-time payload object in PlussQit — the counterpart to
[`Timeline`](timeline_api_reference.md)'s instants.

Where `Timeline` answers "when" (canonical storage: epoch nanoseconds in UTC),
`Duration` answers "how long" (canonical storage: elapsed nanoseconds, with no
anchor to any calendar date). Elapsed nanoseconds and calendar positions are
separate types because they obey different arithmetic rules.

`Duration` is a genuine standalone N-D array type, not a helper that only
exists inside a `Timeline` expression — it has its own shape (`dim()`,
`numel()`, `sizes()`), its own factories (including `range()`, which builds an
array with no `Timeline` involved at all), and a full arithmetic/comparison/
conversion surface. The one real asymmetry: `Duration` is not one of the four
`ThreadKind` payloads a DataFrame column can hold (`Series`, `Note`,
`Timeline`, `TimeIndex`) — to store elapsed time in a column, convert to a
plain `Series` first via one of the [`total()`](api/duration/total_nanoseconds.md)
accessor's unit methods.

Use `Duration` when you have elapsed time that isn't anchored to a particular
calendar date:

- the result of `Timeline - Timeline`
- an offset to add to or subtract from a `Timeline` (`t + Duration`)
- a fixed-width step for building a regular sequence
  (`pluss::duration::range`)
- a magnitude to compare, scale, or convert to a human unit

Use `Timeline`'s own calendar-unit methods (`add_months`, `add_years`, `add_days`,
not yet on their own reference page) instead when the operation should respect
calendar irregularities (a month isn't a fixed number of days) — `Duration`
arithmetic is always a fixed nanosecond offset, with no awareness of month
length or leap years. [`Timeline::add_duration`](api/timeline/add_duration.md)
is the `Duration`-based sibling documented here.

```{note} Duration (capital D) is the type; pluss::duration (lowercase) is its factory
Construction lives in its own namespace, kept separate from the type itself —
the same split as `pluss::timeline::make`/`pluss::timeline::from_epoch` being
separate from `Timeline`. `pluss::duration::seconds(5)`, not
`pluss::duration_seconds(5)`; `pluss::duration::range(...)`, not
`pluss::timedelta_range(...)`. See [`construction`](api/duration/construction.md)
for the full factory surface. `pluss::make_duration(...)` is the intentional
root-level forwarding spelling for `pluss::duration::make(...)`.
```

## Includes

```cpp
#include <pluss/pluss.h>
// or the narrower temporal surface
#include <pluss/codeh/codeh.h>
#include <pluss/codeh/duration_factory.h>
// timeline_factory.h also includes duration_factory.h transitively, since
// Timeline arithmetic consumes Duration -- but Duration does not depend on
// Timeline, so duration_factory.h stands alone as its own header.
```

## Practical Construction Patterns

Example 1. The friendly unit factories — the normal way to build a scalar
`Duration`.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
auto neg_fifteen_min = pluss::duration::minutes(-15);
```

Negative counts are valid and mean "backward in time" — see
[`abs`](api/duration/abs.md) for recovering a magnitude.

Example 2. Every unit factory also builds 1D and N-D arrays — not just
scalars — the same three ways `pluss::timeline::make` does.

```cpp
auto one_d = pluss::duration::minutes({15, 30, 45, 60});          // 1D, 4 elements
auto nested_2d = pluss::duration::hours({{1, 2}, {3, 4}, {5, 6}}); // 2D, shape [3,2]
```

Explicit flat-plus-shape construction is also available for any rank, mirroring
`pluss::timeline::from_epoch(flat, sizes)` — plain braces convert implicitly to
`pluss::RangeView<int64_t>`, no explicit wrapping needed:

```cpp
auto explicit_2x2 = pluss::duration::days({1, 2, 3, 4}, {2, 2});
```

Example 3. `pluss::duration::make(values, unit)` — the same construction
surface again, with the unit picked at runtime via `pluss::duration::Unit`
instead of by function name (`Unit::Nanos`, `::Seconds`, `::Minutes`, `::Hours`,
`::Days`, `::Weeks`, `::Microseconds`, `::Milliseconds`).

```cpp
auto d = pluss::duration::make(5'400, pluss::duration::Unit::Seconds);
// equivalently: pluss::duration::seconds(5'400);
```

Example 4. A regular sequence of durations, with no `Timeline` involved.

```cpp
auto steps = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
// steps.dim() == 1, steps.numel() == 6
```

Example 5. As the byproduct of subtracting two `Timeline`s.

```cpp
auto gap = end_timeline - start_timeline;  // pluss::Duration
```

## Coverage

This reference covers the canonical public surface:

- construction: `pluss::duration::nanos`/`seconds`/`minutes`/`hours`/`days`
  (each with scalar, 1D, nested-N-D, and flat+shape overloads),
  `pluss::duration::range`
- arithmetic: [`add`](api/duration/add.md), [`sub`](api/duration/sub.md),
  [`neg`](api/duration/neg.md), [`mul`](api/duration/mul.md),
  [`div`](api/duration/div.md), [`floordiv`](api/duration/floordiv.md),
  [`mod`](api/duration/mod.md)
- comparisons (elementwise, return `Series[bool]`): [`eq`](api/duration/eq.md),
  [`ne`](api/duration/ne.md), [`lt`](api/duration/lt.md),
  [`le`](api/duration/le.md), [`gt`](api/duration/gt.md),
  [`ge`](api/duration/ge.md)
- unary transforms: [`abs`](api/duration/abs.md), [`sign`](api/duration/sign.md),
  [`clip`](api/duration/clip.md)
- fixed-width rounding: [`ceil`](api/duration/ceil.md),
  [`floor`](api/duration/floor.md), and [`round`](api/duration/round.md)
- reductions and ordering: [`min`](api/duration/min.md),
  [`max`](api/duration/max.md), [`sort`](api/duration/sort.md),
  [`argsort`](api/duration/argsort.md), and [`unique`](api/duration/unique.md)
- text projection: [`to_note`](api/duration/to_note.md) and
  [`to_note_iso`](api/duration/to_note_iso.md)
- every public operator call form: [`operators`](api/duration/operators.md)
- unit conversion (all return a plain `Series`):
  [`total().nanoseconds()`](api/duration/total_nanoseconds.md),
  [`total().microseconds()`](api/duration/total_microseconds.md),
  [`total().milliseconds()`](api/duration/total_milliseconds.md),
  [`total().seconds()`](api/duration/total_seconds.md),
  [`total().minutes()`](api/duration/total_minutes.md),
  [`total().hours()`](api/duration/total_hours.md),
  [`total().days()`](api/duration/total_days.md),
  [`total().weeks()`](api/duration/total_weeks.md)
- missing-value handling: [`isnat`](api/duration/isnat.md),
  [`isvalid`](api/duration/isvalid.md), [`fill_nat`](api/duration/fill_nat.md),
  [`where`](api/duration/where.md)
- `Timeline` integration: `Timeline ± Duration`, `Timeline - Timeline`
- structural introspection: [`defined`](api/duration/defined.md),
  [`dim`](api/duration/dim.md), [`numel`](api/duration/numel.md), and
  [`sizes`](api/duration/sizes.md)
- explicit host extraction: [`host`](api/duration/host.md)
- normalized decomposition: [`days`](api/duration/days.md),
  [`hours`](api/duration/hours.md), [`minutes`](api/duration/minutes.md),
  [`seconds`](api/duration/seconds.md),
  [`milliseconds`](api/duration/milliseconds.md),
  [`microseconds`](api/duration/microseconds.md), and
  [`nanoseconds`](api/duration/nanoseconds.md)

Each distinct method has its own page. Redundant behavioral method aliases are
excluded rather than receiving duplicate chapters; the deliberate root-level
`make_*` factory spellings remain public.

## Using This Reference

- Use the left sidebar to open one `Duration` method page at a time.
- Each page embeds verified output directly under the example that produced
  it, captured from `PlussQit/examples/CodeH/duration_docverify_demo.cpp`.

```{toctree}
:hidden:
:maxdepth: 1

api/duration/abs
api/duration/add
api/duration/argsort
api/duration/ceil
api/duration/clip
api/duration/construction
api/duration/days
api/duration/defined
api/duration/dim
api/duration/div
api/duration/eq
api/duration/fill_nat
api/duration/floor
api/duration/floordiv
api/duration/ge
api/duration/gt
api/duration/host
api/duration/hours
api/duration/isnat
api/duration/isvalid
api/duration/le
api/duration/lt
api/duration/max
api/duration/microseconds
api/duration/milliseconds
api/duration/min
api/duration/minutes
api/duration/mod
api/duration/mul
api/duration/nanoseconds
api/duration/ne
api/duration/neg
api/duration/numel
api/duration/operators
api/duration/round
api/duration/seconds
api/duration/sign
api/duration/sizes
api/duration/sort
api/duration/sub
api/duration/to_note
api/duration/to_note_iso
api/duration/total_days
api/duration/total_hours
api/duration/total_microseconds
api/duration/total_milliseconds
api/duration/total_minutes
api/duration/total_nanoseconds
api/duration/total_seconds
api/duration/total_weeks
api/duration/unique
api/duration/where
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/duration/abs.html">
    <span class="label">Next</span>
    <span class="title">abs</span>
  </a>
</div>
