<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 57ba909bab30 -->

# Atlas::fillnull

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas Atlas::fillnull(double value) const

Atlas Atlas::fillnull(std::string_view value) const

Atlas Atlas::fillnull(const char* value) const

Atlas Atlas::fillnull(const pluss::Timeline& value) const

Atlas Atlas::fillnull(const pluss::Duration& value) const
```

## What It Does

Replaces every missing (`NULL`) cell and leaves everything else alone. The value you pass decides which features are filled: a number fills numeric features, a string fills text features, a `Timeline` fills timestamp features and a `Duration` fills duration features.

A `NaN` or an infinity is a value, not a missing cell, so `fillnull` does not touch it; use `fillnan`, `fillposinf`, `fillneginf` or `fillinf` for those.

`Atlas` applies the [`DataFrame`](../dataframe/fillnull.md) method to every frame in the stack and returns a new `Atlas` with the same depth.

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` / `const pluss::Duration&`)

    The replacement. Its C++ type selects which features are filled. A `Timeline` or `Duration` value must be a single value.

## Returns

`Atlas`
    A stack of the same depth; every frame is filled.

## Examples

Example 1. Fill every missing number with `0.0`.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto frame = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto other = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Atlas(std::vector<DataFrame>{frame, other});

auto out = x.fillnull(0.0);
for (std::int64_t i = 0; i < out.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << out.frame(i) << "\n";
}
```

### Output

```text
frame[0]
=================
     | a     b
-----------------
0    |  1    10
1    |  NaN  NaN
2    |  inf  NaN
3    | -inf  40
4    |  0    50
5    |  5    60
-----------------
[6 cs x 2 feat]
frame[1]
=================
     | a     b
-----------------
0    | 1     10
1    | 2     20
2    | 3     30
3    | 4     40
4    | 5     50
5    | 6     60
-----------------
[6 cs x 2 feat]
```

Only the `NULL` at position 4 of `a` becomes `0`. The `NaN` and the infinities are values, not missing cells, so `fillnull` leaves them alone. The second frame has no value other than ordinary numbers, so nothing changes.

Example 2. `fillnull` never fills a NaN: the NaN positions are still NaN afterwards.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto frame = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto other = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Atlas(std::vector<DataFrame>{frame, other});

auto out = x.fillnull(0.0);
auto nans = out.isnan();
for (std::int64_t i = 0; i < out.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << out.frame(i) << "\n";
}
std::cout << "isnan\n";
for (std::int64_t i = 0; i < nans.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << nans.frame(i) << "\n";
}

```

### Output

```text
frame[0]
=================
     | a     b
-----------------
0    |  1    10
1    |  NaN  NaN
2    |  inf  NaN
3    | -inf  40
4    |  0    50
5    |  5    60
-----------------
[6 cs x 2 feat]
frame[1]
=================
     | a     b
-----------------
0    | 1     10
1    | 2     20
2    | 3     30
3    | 4     40
4    | 5     50
5    | 6     60
-----------------
[6 cs x 2 feat]
isnan
frame[0]
===================
     | a      b
-------------------
0    | false  false
1    | true   true
2    | false  true
3    | false  false
4    | false  false
5    | false  false
-------------------
[6 cs x 2 feat]
frame[1]
===================
     | a      b
-------------------
0    | false  false
1    | false  false
2    | false  false
3    | false  false
4    | false  false
5    | false  false
-------------------
[6 cs x 2 feat]
```

After `fillnull(0.0)` the `isnan` check still finds the same `NaN` cells. Filling `NaN` is the job of `fillnan`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto frame = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto other = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Atlas(std::vector<DataFrame>{frame, other});

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
for (std::int64_t i = 0; i < out.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << out.frame(i) << "\n";
}
```

### Output

```text
frame[0]
=================
     | a     b
-----------------
0    |  1     10
1    | -1    -1
2    |  100  -1
3    | -100   40
4    |  0     50
5    |  5     60
-----------------
[6 cs x 2 feat]
frame[1]
=================
     | a     b
-----------------
0    | 1     10
1    | 2     20
2    | 3     30
3    | 4     40
4    | 5     50
5    | 6     60
-----------------
[6 cs x 2 feat]
```

`NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100: every cell ends up an ordinary number with a value chosen for what it was.

Example 4. Fill missing text with a string (`std::string_view` / `const char*`).

```cpp
using namespace pluss::table;

auto frame = DataFrame({{"city", Shelf(pluss::make_note({"Accra", "", "Tema"}), pluss::make_series(std::vector<bool>{true, false, true}))}});
auto x = Atlas(std::vector<DataFrame>{frame, frame});

auto out = x.fillnull("Unknown");
for (std::int64_t i = 0; i < out.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << out.frame(i) << "\n";
}
```

### Output

```text
frame[0]
==============
     | city
--------------
0    | Accra
1    | Unknown
2    | Tema
--------------
[3 cs x 1 feat]
frame[1]
==============
     | city
--------------
0    | Accra
1    | Unknown
2    | Tema
--------------
[3 cs x 1 feat]
```

The empty entry at position 1 is marked missing, and the string fills it. A string only ever fills text features; numeric features are not touched by it.

Example 5. Fill missing timestamps with a `Timeline` value.

```cpp
using namespace pluss::table;

auto frame = DataFrame({{"opened", Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}), pluss::make_series(std::vector<bool>{true, false, true}))}});
auto x = Atlas(std::vector<DataFrame>{frame, frame});

auto out = x.fillnull(pluss::make_timeline({"2000-01-01T00:00:00Z"}));
for (std::int64_t i = 0; i < out.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << out.frame(i) << "\n";
}
```

### Output

```text
frame[0]
=================
     | opened
-----------------
0    | 2024-01-01
1    | 2000-01-01
2    | 2024-03-01
-----------------
[3 cs x 1 feat]
frame[1]
=================
     | opened
-----------------
0    | 2024-01-01
1    | 2000-01-01
2    | 2024-03-01
-----------------
[3 cs x 1 feat]
```

Position 1 is marked missing and takes the scalar `Timeline` value. A `Timeline` only fills timestamp features.

Example 6. Fill missing durations with a `Duration` value.

```cpp
using namespace pluss::table;

auto frame = DataFrame({{"elapsed", Shelf(pluss::duration::nanos({1'000'000'000LL, 2'000'000'000LL, 3'000'000'000LL}), pluss::make_series(std::vector<bool>{true, false, true}))}});
auto x = Atlas(std::vector<DataFrame>{frame, frame});

auto out = x.fillnull(pluss::duration::nanos(9));
for (std::int64_t i = 0; i < out.depth(); ++i) {
    std::cout << "frame[" << i << "]\n" << out.frame(i) << "\n";
}
```

### Output

```text
frame[0]
=====================
     | elapsed
---------------------
0    | PT1S
1    | PT0.000000009S
2    | PT3S
---------------------
[3 cs x 1 feat]
frame[1]
=====================
     | elapsed
---------------------
0    | PT1S
1    | PT0.000000009S
2    | PT3S
---------------------
[3 cs x 1 feat]
```

Position 1 is marked missing and takes the scalar `Duration` value. A `Duration` only fills duration features.

## Notes

- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the first frame is built from the six-row data and the second frame from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Atlas::isnull`](isnull.md)
- [`Atlas::dropnull`](dropnull.md)
- [`Atlas::fillnan`](fillnan.md)
- [`Atlas::fillinf`](fillinf.md)
- [`Atlas::fillinvalid`](fillinvalid.md)
- [`DataFrame::fillnull`](../dataframe/fillnull.md): the one-dimensional rule this type delegates to.

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
