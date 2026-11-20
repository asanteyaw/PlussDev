<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c94c0cfd33a1 -->

# Nexus::notinf

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::notinf() const
```

## What It Does

Returns a Bool result of the same shape as the input that is `true` wherever a cell does **not** hold an infinity. The answer is about the **value** in the cell: a missing cell has no value, so it is never a `NaN` and never infinite.

Three different things can sit in a numeric cell, and PlussMic keeps them apart on purpose:

| A cell can be | It means | Ask about it with |
|---|---|---|
| `NULL` | no value was recorded (the validity mask says so) | `isnull`, `notnull`, `dropnull`, `fillnull` |
| `NaN` | a value that is not a number: `0/0`, `x % 0`, `sqrt(-1)` | `isnan`, `notnan`, `dropnan`, `fillnan` |
| `+inf` / `-inf` | a value that is unbounded: `5/0`, `log(0)`, overflow | `isinf`, `notinf`, `isposinf`, `isneginf`, `dropinf`, `dropposinf`, `dropneginf`, `fillinf`, `fillposinf`, `fillneginf` |
| any of the three | not an ordinary number | `invalid`, `dropinvalid`, `fillinvalid` |

`notinf()` answers one precise question. Here is what it says for each kind of cell:

| A cell holding | `notinf()` |
|---|---|
| An ordinary number (`1`, `5`) | `true` |
| `NaN` | `true` |
| `+inf` | `false` |
| `-inf` | `false` |
| `NULL` (missing) | `true` |

`Nexus` applies the [`DataFrame`](../dataframe/notinf.md) method to every table in the workspace and returns a new `Nexus`; relationships and table metadata are kept.

## Parameters

`None`

## Returns

`Nexus`
    A workspace with the same table names and relationships; every table replaced by its Bool frame.

## Examples

Example 1. Mark every row that does **not** hold an infinity, in data that holds all four kinds of cell.

```cpp
using namespace pluss::bundle;
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto a = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto b = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Nexus({{"a", a}, {"b", b}});

auto out = x.notinf();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
==================
     | a      b
------------------
0    | true   true
1    | true   true
2    | false  true
3    | false  true
4    | true   true
5    | true   true
------------------
[6 cs x 2 feat]
=================
     | a     b
-----------------
0    | true  true
1    | true  true
2    | true  true
3    | true  true
4    | true  true
5    | true  true
-----------------
[6 cs x 2 feat]
```

Position 0 and 5 are ordinary numbers, position 1 is a `NaN`, positions 2 and 3 are `+inf` and `-inf`, and position 4 is `NULL`. For feature `a` the answers are [true, true, false, false, true, true]; feature `b` has `NaN` at positions 1 and 2 only, so its answers are [true, true, true, true, true, true]. The table `b` has no value other than ordinary numbers, so every answer is `true`.

Example 2. Ask the three questions on the same data, side by side.

```cpp
using namespace pluss::bundle;
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto a = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto b = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Nexus({{"a", a}, {"b", b}});

auto missing = x.isnull();
auto asked = x.notinf();
auto either = x.invalid();
std::cout << "isnull\n";
std::cout << missing.frame("a") << "\n" << missing.frame("b") << "\n";
std::cout << "notinf\n";
std::cout << asked.frame("a") << "\n" << asked.frame("b") << "\n";
std::cout << "invalid\n";
std::cout << either.frame("a") << "\n" << either.frame("b") << "\n";
```

### Output

```text
isnull
===================
     | a      b
-------------------
0    | false  false
1    | false  false
2    | false  false
3    | false  false
4    | true   false
5    | false  false
-------------------
[6 cs x 2 feat]
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
notinf
==================
     | a      b
------------------
0    | true   true
1    | true   true
2    | false  true
3    | false  true
4    | true   true
5    | true   true
------------------
[6 cs x 2 feat]
=================
     | a     b
-----------------
0    | true  true
1    | true  true
2    | true  true
3    | true  true
4    | true  true
5    | true  true
-----------------
[6 cs x 2 feat]
invalid
===================
     | a      b
-------------------
0    | false  false
1    | true   true
2    | true   true
3    | true   false
4    | true   false
5    | false  false
-------------------
[6 cs x 2 feat]
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

`isnull` is `true` only for the `NULL` position. `notinf` answers the question about an infinity and is never `true` for `NULL`. `invalid` is `true` for all three kinds of not-an-ordinary-number at once: `NULL`, `NaN` and the infinities. The three are different questions; none of them is a synonym for another.

Example 3. Data that cannot hold NaN or infinity: integers give the same answer everywhere.

```cpp
using namespace pluss::bundle;
using namespace pluss::table;

auto a = DataFrame({
    {"a", Shelf(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
    {"b", Shelf(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}))},
});
auto b = DataFrame({
    {"a", Shelf(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
    {"b", Shelf(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}))},
});
auto x = Nexus({{"a", a}, {"b", b}});

auto out = x.notinf();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
=================
     | a     b
-----------------
0    | true  true
1    | true  true
2    | true  true
3    | true  true
4    | true  true
5    | true  true
-----------------
[6 cs x 2 feat]
=================
     | a     b
-----------------
0    | true  true
1    | true  true
2    | true  true
3    | true  true
4    | true  true
5    | true  true
-----------------
[6 cs x 2 feat]
```

An integer row is never a `NaN` and never infinite, so `notinf` is `true` for all of them. There is no error and no conversion to a floating type: the question simply has the answer "no".

## Notes

- `NULL` is **not** `NaN`. A missing cell has no value, so `isnan`/`isinf`/`isposinf`/`isneginf` are `false` for it and the `not...` forms are `true`. Ask `isnull` for missing cells and `invalid` for "missing or not an ordinary number".
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the table `a` is built from the six-row data and the table `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Nexus::isinf`](isinf.md)
- [`Nexus::notnan`](notnan.md)
- [`Nexus::notnull`](notnull.md)
- [`DataFrame::notinf`](../dataframe/notinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="neighbors.html">
    <span class="label">Previous</span>
    <span class="title">neighbors</span>
  </a>
  <a class="next-link" href="notnan.html">
    <span class="label">Next</span>
    <span class="title">notnan</span>
  </a>
</div>
