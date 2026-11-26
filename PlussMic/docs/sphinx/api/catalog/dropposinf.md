<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 014fa297c719 -->

# Catalog::dropposinf

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::dropposinf() const
```

## What It Does

Removes every record that holds `+inf` and returns the rest, in their original order. Nothing is repaired or replaced: the offending records are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropposinf()` does to each kind of cell:

| A cell holding | After `dropposinf()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | kept |
| `+inf` | removed |
| `-inf` | kept |
| `NULL` (missing) | kept |

It is the exact counterpart of the matching `is...` method: `x.dropposinf()` keeps the positions where `x.isposinf()` is `false`.

`Catalog` applies the [`Stream`](../stream/dropposinf.md) method to every stream. Streams may have different lengths; nothing is padded.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog in which every stream keeps the same positions; the streams stay aligned with one another.

## Examples

Example 1. Remove the records that hold +inf.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto out = x.dropposinf();
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
0  1
1  NaN
2  -inf
3  NULL
4  5
ptype: Series

0  10
1  NaN
2  40
3  50
4  60
ptype: Series
```

A row goes when **any** of its features holds the targeted kind: row 2 is removed and rows 0, 1, 3, 4, 5 remain, in their original order.

Example 2. `dropnull` and `dropposinf` remove different things.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto without_missing = x.dropnull();
auto without_this = x.dropposinf();
std::cout << "dropnull\n";
std::cout << without_missing.keys("a") << "\n" << without_missing.keys("b") << "\n";
std::cout << "dropposinf\n";
std::cout << without_this.keys("a") << "\n" << without_this.keys("b") << "\n";
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

0  10
1  NaN
2  NaN
3  40
4  60
ptype: Series

dropposinf
0  1
1  NaN
2  -inf
3  NULL
4  5
ptype: Series

0  10
1  NaN
2  40
3  50
4  60
ptype: Series
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropposinf()` removes only the +inf positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::kindle;

auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});

auto out = x.dropposinf();
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
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

0  10
1  20
2  30
3  40
4  50
5  60
ptype: Series
```

Every value is an ordinary number, so there is nothing to drop and every row is still there.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A catalog's streams may differ in length. For `drop...` the decision is made per position: a position is removed from **every** stream when **any** stream holds the targeted kind there, which keeps the streams aligned with one another.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Catalog::isposinf`](isposinf.md)
- [`Catalog::fillposinf`](fillposinf.md)
- [`Catalog::dropneginf`](dropneginf.md)
- [`Catalog::dropinf`](dropinf.md)
- [`Stream::dropposinf`](../stream/dropposinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
