<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: be8a298dd281 -->

# Shelf::fillnull

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::fillnull(double value) const
Shelf Shelf::fillnull(std::string_view value) const
Shelf Shelf::fillnull(const pluss::Timeline& value) const
Shelf Shelf::fillnull(const pluss::Duration& value) const
```

## What It Does

Replaces every `NULL` cell with `value`, leaving already-valid cells untouched. The
overload selects which payload kind is required: `double` requires a `Series`-kind
shelf, `std::string_view` requires `Note`-kind, `pluss::Timeline` requires
`Timeline`-kind, and `pluss::Duration` requires `Duration`-kind. Unlike the other
overloads, the `Duration` overload also clears any intrinsic `NaT` lanes (not just
lanes flagged by a validity mask) via `fill_nat`, since `Duration`'s own payload can
carry a sentinel-missing value independent of the shelf's validity mask. This is the
same operation [`replace(null_t, to)`](replace.md) delegates to internally when `from`
is `pluss::series::null`.

## Whole-number features and fractional fills

Filling a whole-number (integer) feature with a fractional value widens the feature to floating point, so the stored fill is exact: filling an integer feature with `78.75` stores `78.75`, never `78`. A fill that is itself a whole number keeps the feature's integer type.

## Parameters

**value**
    The replacement for `NULL` cells. Its C++ type selects the overload — `double`
    for `Series`, `std::string_view`/`const char*` for `Note`, `pluss::Timeline` for
    `Timeline`-kind shelves, `pluss::Duration` for `Duration`-kind shelves. The
    `Timeline` overload requires a scalar value (`numel() == 1`); the `Duration`
    overload requires exactly one value and that value must not itself be `NaT`.

## Returns

`Shelf`
    A new shelf, same length and kind as `*this`, with every `NULL` cell replaced
    by `value`.

## Examples

Example 1. Fill `NULL` numeric cells with a sentinel value.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out = readings.fillnull(0.0);
std::cout << out;
```

### Output

```text
0  10
1  0
2  30

ptype: Series
```

Example 2. Fill `NULL` text cells with a placeholder string.

```cpp
auto cities = Shelf(pluss::make_note({"Accra", "Tema", "Kumasi"}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out2 = cities.fillnull("Unknown");
std::cout << out2;
```

### Output

```text
0  Accra
1  Unknown
2  Kumasi

ptype: Note
```

Example 3. Fill `NULL` timestamps with a scalar `Timeline` value.

```cpp
auto events = Shelf(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out3 = events.fillnull(pluss::make_timeline({"2000-01-01T00:00:00Z"}));
std::cout << out3;
```

### Output

```text
0  2024-01-01
1  2000-01-01
2  2024-03-01

ptype: Timeline
```

Example 4. Fill a `Duration`-kind shelf. `fillnull(Duration)` clears intrinsic `NaT`
lanes in addition to any validity-mask-flagged ones.

```cpp
auto gaps = Shelf(pluss::duration::nanos({1'000'000'000LL, pluss::Duration::kNaT,
                                           -2'000'000'000LL}));
auto out4 = gaps.fillnull(pluss::duration::nanos(9));
assert(out4.as_duration().host() ==
       std::vector<std::int64_t>({1'000'000'000LL, 9, -2'000'000'000LL}));
assert(!out4.isnull().any().as_series().items<bool>().front());
```

The middle lane was `NaT` (not mask-flagged), yet it was still replaced, and the
result carries no validity mask once every absent lane has been filled — pinned by
`PlussMic/test/temporal_core_migration_tests.cpp:198-203`.

Example 5. Value-kind mismatches throw.

```cpp
try {
  auto bad = readings.fillnull("not a number");
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::fillnull(string) is only valid for Note payload
```

## Notes

- `fillnull` fills missing cells only; a `NaN` or an infinity is a value and is left as it is. Use [`fillnan`](fillnan.md), [`fillinf`](fillinf.md) / [`fillposinf`](fillposinf.md) / [`fillneginf`](fillneginf.md) or [`fillinvalid`](fillinvalid.md) for those, and chain them to give each kind its own value. See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
- Overload selection is by the C++ type of `value`, which also determines the
  required payload kind.
- `fillnull(double)` requires `Series`; message: `"Shelf::fillnull(double) is only
  valid for Series payload"`.
- `fillnull(string_view)` requires `Note`; message: `"Shelf::fillnull(string) is
  only valid for Note payload"`.
- `fillnull(Timeline)` requires `Timeline`; message: `"Shelf::fillnull(Timeline) is
  only valid for Timeline payload"`. It requires a scalar (single-element)
  replacement value, or throws `"Shelf::fillnull(Timeline): replacement value must
  be scalar (numel == 1)"`.
- `fillnull(Duration)` requires `Duration`; message: `"Shelf::fillnull(Duration) is
  only valid for Duration payload"`. It requires exactly one replacement value
  (`"Shelf::fillnull(Duration): replacement must contain exactly one value"`), and
  that value cannot itself be `NaT` (`"Shelf::fillnull(Duration): replacement cannot
  be NaT"`). Unlike the other overloads, it clears intrinsic `NaT` lanes even when
  the shelf carries no validity mask at all.
- A mutating sibling, [`fillnull_()`](fillnull_.md), applies the same fill in place.

## See Also

- [`Shelf::fillnull_`](fillnull_.md): the mutating (in-place) form of this method.
- [`Shelf::replace`](replace.md): `replace(pluss::series::null, to)` is equivalent to
  `fillnull(to)`.
- [`Shelf::isnull`](isnull.md): the validity check `fillnull()` acts on.
- [`Shelf::ffill`](ffill.md) / [`Shelf::bfill`](bfill.md): fill `NULL` cells from
  neighboring values instead of a fixed replacement.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillneginf.html">
    <span class="label">Previous</span>
    <span class="title">fillneginf</span>
  </a>
  <a class="next-link" href="fillnull_.html">
    <span class="label">Next</span>
    <span class="title">fillnull_</span>
  </a>
</div>
