<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14af5d30e0aa -->

# Locus::fillnull

[Back to Locus API Reference](../../locus_api_reference)

## Signatures

```cpp
Locus Locus::fillnull(double value) const

Locus Locus::fillnull(std::string_view value) const

Locus Locus::fillnull(const char* value) const

Locus Locus::fillnull(const pluss::Timeline& value) const

Locus Locus::fillnull(const pluss::Duration& value) const
```

## What It Does

Replaces every missing (`NULL`) cell and leaves everything else alone. The value you pass decides which features are filled: a number fills numeric features, a string fills text features, a `Timeline` fills timestamp features and a `Duration` fills duration features.

A `NaN` or an infinity is a value, not a missing cell, so `fillnull` does not touch it; use `fillnan`, `fillposinf`, `fillneginf` or `fillinf` for those.

`Locus` applies the [`Catalog`](../catalog/fillnull.md) method to every catalog in the workspace and returns a new `Locus`; links and table metadata are kept.

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` / `const pluss::Duration&`)

    The replacement. Its C++ type selects which features are filled. A `Timeline` or `Duration` value must be a single value.

## Returns

`Locus`
    A workspace with the same names and links; every catalog is filled.

## Examples

Example 1. Fill every missing number with `0.0`.

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto a = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto b = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Locus({{"a", a}, {"b", b}});

auto out = x.fillnull(0.0);
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | NaN
2            | inf
3            | -inf
4            | 0
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 2
2            | 3
3            | 4
4            | 5
5            | 6
========================================================================================================================
```

Only the `NULL` at position 4 of `a` becomes `0`. The `NaN` and the infinities are values, not missing cells, so `fillnull` leaves them alone. The catalog `b` has no value other than ordinary numbers, so nothing changes.

Example 2. `fillnull` never fills a NaN: the NaN positions are still NaN afterwards.

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto a = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto b = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Locus({{"a", a}, {"b", b}});

auto out = x.fillnull(0.0);
auto nans = out.isnan();
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
std::cout << "isnan\n";
std::cout << nans.catalog("a").keys("a") << "\n" << nans.catalog("b").keys("a") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | NaN
2            | inf
3            | -inf
4            | 0
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 2
2            | 3
3            | 4
4            | 5
5            | 6
========================================================================================================================

isnan
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | false
1            | true
2            | false
3            | false
4            | false
5            | false
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | false
1            | false
2            | false
3            | false
4            | false
5            | false
========================================================================================================================
```

After `fillnull(0.0)` the `isnan` check still finds the same `NaN` cells. Filling `NaN` is the job of `fillnan`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto a = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto b = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Locus({{"a", a}, {"b", b}});

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | -1
2            | 100
3            | -100
4            | 0
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 2
2            | 3
3            | 4
4            | 5
5            | 6
========================================================================================================================
```

`NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100: every cell ends up an ordinary number with a value chosen for what it was.

Example 4. Fill missing text with a string (`std::string_view` / `const char*`).

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;
using namespace pluss::table;

auto frame = DataFrame({{"city", Shelf(pluss::make_note({"Accra", "", "Tema"}), pluss::make_series(std::vector<bool>{true, false, true}))}});
auto cat = frame.kindelize();
auto x = Locus({{"a", cat}, {"b", cat}});

auto out = x.fillnull("Unknown");
std::cout << out.catalog("a").keys("city") << "\n" << out.catalog("b").keys("city") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | Accra
1            | Unknown
2            | Tema
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | Accra
1            | Unknown
2            | Tema
========================================================================================================================
```

The empty entry at position 1 is marked missing, and the string fills it. A string only ever fills text features; numeric features are not touched by it.

Example 5. Fill missing timestamps with a `Timeline` value.

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;
using namespace pluss::table;

auto frame = DataFrame({{"opened", Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}), pluss::make_series(std::vector<bool>{true, false, true}))}});
auto cat = frame.kindelize();
auto x = Locus({{"a", cat}, {"b", cat}});

auto out = x.fillnull(pluss::make_timeline({"2000-01-01T00:00:00Z"}));
std::cout << out.catalog("a").keys("opened") << "\n" << out.catalog("b").keys("opened") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Timeline  dtype=Timeline  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 2024-01-01T00:00:00.000000000+00:00
1            | 2000-01-01T00:00:00.000000000+00:00
2            | 2024-03-01T00:00:00.000000000+00:00
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Timeline  dtype=Timeline  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 2024-01-01T00:00:00.000000000+00:00
1            | 2000-01-01T00:00:00.000000000+00:00
2            | 2024-03-01T00:00:00.000000000+00:00
========================================================================================================================
```

Position 1 is marked missing and takes the scalar `Timeline` value. A `Timeline` only fills timestamp features.

Example 6. Fill missing durations with a `Duration` value.

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;
using namespace pluss::table;

auto frame = DataFrame({{"elapsed", Shelf(pluss::duration::nanos({1'000'000'000LL, 2'000'000'000LL, 3'000'000'000LL}), pluss::make_series(std::vector<bool>{true, false, true}))}});
auto cat = frame.kindelize();
auto x = Locus({{"a", cat}, {"b", cat}});

auto out = x.fillnull(pluss::duration::nanos(9));
std::cout << out.catalog("a").keys("elapsed") << "\n" << out.catalog("b").keys("elapsed") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Duration  dtype=duration64[ns]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | PT1S
1            | PT0.000000009S
2            | PT3S
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Duration  dtype=duration64[ns]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | PT1S
1            | PT0.000000009S
2            | PT3S
========================================================================================================================
```

Position 1 is marked missing and takes the scalar `Duration` value. A `Duration` only fills duration features.

## Notes

- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the catalog `a` is built from the six-row data and the catalog `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Locus::isnull`](isnull.md)
- [`Locus::dropnull`](dropnull.md)
- [`Locus::fillnan`](fillnan.md)
- [`Locus::fillinf`](fillinf.md)
- [`Locus::fillinvalid`](fillinvalid.md)
- [`Catalog::fillnull`](../catalog/fillnull.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillneginf.html">
    <span class="label">Previous</span>
    <span class="title">fillneginf</span>
  </a>
  <a class="next-link" href="fillposinf.html">
    <span class="label">Next</span>
    <span class="title">fillposinf</span>
  </a>
</div>
