<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0b581c91d5b2 -->

# Locus::dropnan

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::dropnan() const
```

## What It Does

Removes every record that holds a `NaN` and returns the rest, in their original order. Nothing is repaired or replaced: the offending records are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropnan()` does to each kind of cell:

| A cell holding | After `dropnan()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | removed |
| `+inf` | kept |
| `-inf` | kept |
| `NULL` (missing) | kept |

It is the exact counterpart of the matching `is...` method: `x.dropnan()` keeps the positions where `x.isnan()` is `false`.

`Locus` applies the [`Catalog`](../catalog/dropnan.md) method to every catalog in the workspace and returns a new `Locus`; links and table metadata are kept.

## Parameters

`None`

## Returns

`Locus`
    A workspace with the same names and links; each catalog is filtered on its own.

## Examples

Example 1. Remove the records that hold NaN.

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

auto out = x.dropnan();
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
```

### Output

```text
0  1
1  -inf
2  NULL
3  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

A row goes when **any** of its features holds the targeted kind: rows 1, 2 are removed and rows 0, 3, 4, 5 remain, in their original order. The catalog `b` has no value other than ordinary numbers, so nothing is removed.

Example 2. `dropnull` and `dropnan` remove different things.

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

auto without_missing = x.dropnull();
auto without_this = x.dropnan();
std::cout << "dropnull\n";
std::cout << without_missing.catalog("a").keys("a") << "\n" << without_missing.catalog("b").keys("a") << "\n";
std::cout << "dropnan\n";
std::cout << without_this.catalog("a").keys("a") << "\n" << without_this.catalog("b").keys("a") << "\n";
```

### Output

```text
dropnull
0  1
1  NaN
2  inf
3  -inf
4  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series

dropnan
0  1
1  -inf
2  NULL
3  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropnan()` removes only the NaN positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::bundle;
using namespace pluss::kindle;

auto a = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto b = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Locus({{"a", a}, {"b", b}});

auto out = x.dropnan();
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
```

### Output

```text
0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

Every value is an ordinary number, so there is nothing to drop and every row is still there.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the catalog `a` is built from the six-row data and the catalog `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Locus::isnan`](isnan.md)
- [`Locus::fillnan`](fillnan.md)
- [`Locus::dropnull`](dropnull.md)
- [`Locus::dropinf`](dropinf.md)
- [`Locus::dropinvalid`](dropinvalid.md)
- [`Catalog::dropnan`](../catalog/dropnan.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropinvalid.html">
    <span class="label">Previous</span>
    <span class="title">dropinvalid</span>
  </a>
  <a class="next-link" href="dropneginf.html">
    <span class="label">Next</span>
    <span class="title">dropneginf</span>
  </a>
</div>
