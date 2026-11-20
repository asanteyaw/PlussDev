<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 36f1e18eaf7c -->

# Nexus::fillneginf

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::fillneginf(double value) const
```

## What It Does

Returns the same structure with every -inf cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillneginf(value)` does to each kind of cell:

| A cell holding | After `fillneginf(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | unchanged |
| `+inf` | unchanged |
| `-inf` | `value` |
| `NULL` (missing) | `NULL` (unchanged) |

A missing cell is never filled by this method, because a missing cell holds no -inf to replace; use `fillnull` for that.

`Nexus` applies the [`DataFrame`](../dataframe/fillneginf.md) method to every table in the workspace and returns a new `Nexus`; relationships and table metadata are kept.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Nexus`
    A workspace with the same names and relationships; every table is filled.

## Examples

Example 1. Replace every -inf with `-1000.0`.

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

auto out = x.fillneginf(-1000.0);
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
==================
     | a      b
------------------
0    |  1     10
1    |  NaN   NaN
2    |  inf   NaN
3    | -1000  40
4    | NULL   50
5    |  5     60
------------------
[6 cs x 2 feat]
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

Every matching cell now holds `-1000.0`: feature `a` is [1, NaN, inf, -1000, NULL, 5] and feature `b` is [10, NaN, NaN, 40, 50, 60]. The `NULL` position is not touched: it is still `NULL`. The table `b` has no value other than ordinary numbers, so nothing changes.

Example 2. A fill never invents a value for a missing cell.

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

auto out = x.fillneginf(-1000.0);
auto still_null = out.isnull();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
std::cout << "isnull\n";
std::cout << still_null.frame("a") << "\n" << still_null.frame("b") << "\n";
```

### Output

```text
==================
     | a      b
------------------
0    |  1     10
1    |  NaN   NaN
2    |  inf   NaN
3    | -1000  40
4    | NULL   50
5    |  5     60
------------------
[6 cs x 2 feat]
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
```

`fillneginf` only touches -inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

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

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
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

`fillneginf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the table `a` is built from the six-row data and the table `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Nexus::isneginf`](isneginf.md)
- [`Nexus::dropneginf`](dropneginf.md)
- [`Nexus::fillposinf`](fillposinf.md)
- [`Nexus::fillinf`](fillinf.md)
- [`DataFrame::fillneginf`](../dataframe/fillneginf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnan.html">
    <span class="label">Previous</span>
    <span class="title">fillnan</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
