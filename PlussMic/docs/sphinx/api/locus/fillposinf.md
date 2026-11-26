<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5e3f78691abf -->

# Locus::fillposinf

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::fillposinf(double value) const
```

## What It Does

Returns the same structure with every +inf cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillposinf(value)` does to each kind of cell:

| A cell holding | After `fillposinf(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | unchanged |
| `+inf` | `value` |
| `-inf` | unchanged |
| `NULL` (missing) | `NULL` (unchanged) |

A missing cell is never filled by this method, because a missing cell holds no +inf to replace; use `fillnull` for that.

`Locus` applies the [`Catalog`](../catalog/fillposinf.md) method to every catalog in the workspace and returns a new `Locus`; links and table metadata are kept.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Locus`
    A workspace with the same names and links; every catalog is filled.

## Examples

Example 1. Replace every +inf with `1000.0`.

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

auto out = x.fillposinf(1000.0);
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
```

### Output

```text
0  1
1  NaN
2  1000
3  -inf
4  NULL
5  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

Every matching cell now holds `1000.0`: stream `a` is [1, NaN, 1000, -inf, NULL, 5] and stream `b` is [10, NaN, NaN, 40, 50, 60]. The `NULL` position is not touched: it is still `NULL`. The catalog `b` has no value other than ordinary numbers, so nothing changes.

Example 2. A fill never invents a value for a missing cell.

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

auto out = x.fillposinf(1000.0);
auto still_null = out.isnull();
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
std::cout << "isnull\n";
std::cout << still_null.catalog("a").keys("a") << "\n" << still_null.catalog("b").keys("a") << "\n";
```

### Output

```text
0  1
1  NaN
2  1000
3  -inf
4  NULL
5  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series

isnull
0  false
1  false
2  false
3  false
4  true
5  false
ptype: Series

0  false
1  false
2  false
3  false
4  false
5  false
ptype: Series
```

`fillposinf` only touches +inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

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
0  1
1  -1
2  100
3  -100
4  0
5  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

`fillposinf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the catalog `a` is built from the six-row data and the catalog `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Locus::isposinf`](isposinf.md)
- [`Locus::dropposinf`](dropposinf.md)
- [`Locus::fillneginf`](fillneginf.md)
- [`Locus::fillinf`](fillinf.md)
- [`Catalog::fillposinf`](../catalog/fillposinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="fingerprint.html">
    <span class="label">Next</span>
    <span class="title">fingerprint</span>
  </a>
</div>
