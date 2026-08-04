<!-- pluss-hand-authored -->

# Duration construction

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
pluss::Duration pluss::duration::nanos(std::int64_t count);
pluss::Duration pluss::duration::nanos(std::initializer_list<NestedInt64Literal> nested);
pluss::Duration pluss::duration::nanos(pluss::RangeView<std::int64_t> flat, pluss::RangeView<std::int64_t> sizes);
// microseconds/milliseconds/seconds/minutes/hours/days/weeks each have the
// same three overloads as nanos above.

enum class pluss::duration::Unit { Nanos, Microseconds, Milliseconds, Seconds, Minutes, Hours, Days, Weeks };
pluss::Duration pluss::duration::make(std::int64_t count, Unit unit);
pluss::Duration pluss::duration::make(std::initializer_list<NestedInt64Literal> nested, Unit unit);
pluss::Duration pluss::duration::make(pluss::RangeView<std::int64_t> flat, pluss::RangeView<std::int64_t> sizes, Unit unit);

pluss::Duration pluss::duration::range(const Duration& start, const Duration& end, const Duration& step);
```

## What It Does

Builds a `Duration` — an elapsed-time value or array — from scratch, with no
`Timeline` required.

`Duration` (capital D, the type) has no public constructors and no public
construction statics of its own — `Duration()` is deleted, and there is no
`Duration::from_scalar_nanos`/`Duration::range` to call directly. Construction
lives entirely in its own namespace, `pluss::duration` (lowercase), the same
split as `pluss::make_timeline`/`pluss::from_epoch_nanos` being kept separate
from `Timeline` itself:

- `pluss::duration::nanos(...)`: the base unit. `microseconds`/`milliseconds`/
  `seconds`/`minutes`/`hours`/`days`/`weeks` all scale up from it, so every
  unit factory produces an indistinguishable `Duration` for an equal elapsed
  time — `days(1)` and `hours(24)` are the same value.
- **Every one of those eight functions has three overloads**, not just a
  scalar one — see Example 2. A single count builds a scalar (0D) `Duration`;
  a braced list builds 1D; a nested braced list (up to 3D) or an explicit
  `(flat, shape)` pair builds any N-D array — mirroring `pluss::make_timeline`'s
  own scalar/1D/nested/flat+shape overload set exactly. Plain `{...}` braces
  work directly for the `flat`/`shape` arguments too (see Example 3) — no
  explicit `RangeView` wrapping needed.
- `pluss::duration::make(values, unit)`: the same three-overload shape again,
  generalized to take the unit as a runtime `Unit` value instead of picking a
  function by name — for call sites where the unit isn't known until runtime
  (read from config, chosen generically). See Example 4.
- `pluss::duration::range(start, end, step)`: a pandas-`timedelta_range`-style
  constructor that always builds a 1D array, regardless of how `start`/`end`/
  `step` themselves were built.

## Parameters

**count** (`std::int64_t`)

A single count in the named unit, for the scalar overload. Negative values
are valid and represent "backward in time" — see [`abs`](abs.md).

**nested** (nested `std::initializer_list<std::int64_t>`, up to 3 levels deep)

A braced literal — `{1, 2, 3}` for 1D, `{{1, 2}, {3, 4}}` for 2D, and so on —
of counts in the named unit.

**flat**, **sizes** (`pluss::RangeView<std::int64_t>`)

An explicit flat buffer of counts plus the target shape, for constructing any
rank without going through nested braces — the same explicit-shape escape
hatch `pluss::from_epoch_nanos(flat, sizes)` offers for `Timeline`. Accepts a
plain `{...}` brace list directly; `pluss::RangeView<T>` converts implicitly
from `std::initializer_list<T>`.

**unit** (`pluss::duration::Unit`, `make` only)

Which unit `count`/`nested`/`flat` are expressed in.

**start**, **end**, **step** (`const Duration&`, `range` only)

Inclusive start, exclusive-past-end stop, and step size, exactly like
`pluss::date_range` for `Timeline` or Python's `range()`. All three are
themselves `Duration` values (typically scalar, built from one of the unit
factories above), not raw integers.

## Returns

`Duration`
    A scalar (0D), 1D, or N-D `Duration` depending on which overload is used.
    `range` always returns 1D.

## Examples

Example 1. The friendly scalar unit factories, including a negative count.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
auto neg_fifteen_min = pluss::duration::minutes(-15);
std::cout << pluss::print_series(ninety_min.total_nanoseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5.4000e+12
```

`90 * 60 * 1e9 = 5.4e12` nanoseconds — `duration::minutes` scales up from
`duration::nanos` exactly as documented above.

Example 2. N-D construction: a 1D list and a nested 2D literal, for
`minutes`/`hours` respectively.

```cpp
auto minutes_1d = pluss::duration::minutes({15, 30, 45, 60});
auto hours_2d = pluss::duration::hours({{1, 2}, {3, 4}, {5, 6}});
std::cout << "1d.dim=" << minutes_1d.dim() << " 1d.numel=" << minutes_1d.numel() << "\n";
std::cout << "2d.dim=" << hours_2d.dim() << " 2d.numel=" << hours_2d.numel() << "\n";
std::cout << pluss::print_series(hours_2d.total_hours());
```

### Output

```text
1d.dim=1 1d.numel=4
2d.dim=2 2d.numel=6
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎢ 3.0000 4.0000 ⎥
⎣ 5.0000 6.0000 ⎦
```

`hours_2d` is a genuine 2D `Duration` with shape `[3, 2]`, holding `1`
through `6` hours in row-major order — not six independent scalar `Duration`s.

Example 3. Explicit `(flat, shape)` construction with plain braces — no
`RangeView` wrapping needed, it converts implicitly.

```cpp
auto days_2x2 = pluss::duration::days({1, 2, 3, 4}, {2, 2});
std::cout << pluss::print_series(days_2x2.total_days());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎣ 3.0000 4.0000 ⎦
```

Example 4. `pluss::duration::make` — the same construction, with the unit picked at runtime.

```cpp
auto made_scalar = pluss::duration::make(3, pluss::duration::Unit::Days);
auto made_1d = pluss::duration::make({1, 2, 3}, pluss::duration::Unit::Hours);
std::cout << "scalar=" << pluss::print_series(made_scalar.total_days()) << "\n";
std::cout << "1d=" << pluss::print_series(made_1d.total_hours());
```

### Output

```text
scalar=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.0000
1d=Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.0000, 3.0000]
```

Example 5. `pluss::duration::range` builds a real 1D array with no `Timeline`
involved.

```cpp
auto steps = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
std::cout << "dim=" << steps.dim() << " numel=" << steps.numel() << "\n";
std::cout << pluss::print_series(steps.total_days());
```

### Output

```text
dim=1 numel=6
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 2.0000, 4.0000, 6.0000, 8.0000, 10.0000]
```

Six values from `0` up to (but not including) `10`, stepping by `2` — the same
half-open range convention as `pluss::date_range`.

## Notes

- `Duration()` has no default constructor, and there is no public
  `Duration::from_scalar_nanos`/`Duration::range` either — always go through
  `pluss::duration::*`.
- The unit factories are pure convenience: nothing they produce is
  distinguishable afterward from an equivalent `duration::nanos` call at the
  same elapsed magnitude — `Duration` has no internal notion of "the unit it
  was constructed in."
- Every unit factory's N-D overloads (Examples 2 and 3) exist precisely so you
  never have to fall back to building an array of scalar `Duration`s by hand
  and combining them — construct the array directly, in one call. `make`
  (Example 4) gets the same N-D overload set, for when the unit itself is a
  runtime value.
- `Timeline - Timeline` is an implicit way to obtain a `Duration` outside this
  factory entirely — see [`Timeline::diff`](../timeline/diff.md).

## See Also

- [`arithmetic`](arithmetic.md): combining `Duration` values once constructed.
- [`total`](total.md): converting a `Duration` back into a plain `Series` in a
  chosen unit.
- [`Timeline::diff`](../timeline/diff.md): the `Timeline`-side source of a
  `Duration`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../duration_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Duration API Reference</span>
  </a>
  <a class="next-link" href="arithmetic.html">
    <span class="label">Next</span>
    <span class="title">arithmetic</span>
  </a>
</div>
