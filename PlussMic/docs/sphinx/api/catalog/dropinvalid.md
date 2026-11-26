<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a9a2e91b85af -->

# Catalog::dropinvalid

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::dropinvalid() const
```

## What It Does

Removes every record that is `NULL`, `NaN` or an infinity and returns the rest, in their original order. Nothing is repaired or replaced: the offending records are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropinvalid()` does to each kind of cell:

| A cell holding | After `dropinvalid()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | removed |
| `+inf` | removed |
| `-inf` | removed |
| `NULL` (missing) | removed |

`Catalog` applies the [`Stream`](../stream/dropinvalid.md) method to every stream. Streams may have different lengths; nothing is padded.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog in which every stream keeps the same positions; the streams stay aligned with one another.

## Examples

Example 1. Remove the records that hold a NULL, a NaN or an infinity.

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

auto out = x.dropinvalid();
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
0  1
1  5
ptype: Series

0  10
1  60
ptype: Series
```

A row goes when **any** of its features holds the targeted kind: rows 1, 2, 3, 4 are removed and rows 0, 5 remain, in their original order.

Example 2. One call instead of three: the result is the same as chaining the three single-kind drops.

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

auto chained = x.dropnull().dropnan().dropinf();
auto at_once = x.dropinvalid();
std::cout << "dropnull().dropnan().dropinf()\n";
std::cout << chained.keys("a") << "\n" << chained.keys("b") << "\n";
std::cout << "dropinvalid\n";
std::cout << at_once.keys("a") << "\n" << at_once.keys("b") << "\n";
```

### Output

```text
dropnull().dropnan().dropinf()
0  1
1  5
ptype: Series

0  10
1  60
ptype: Series

dropinvalid
0  1
1  5
ptype: Series

0  10
1  60
ptype: Series
```

`dropinvalid()` is the catch-all. Chaining the three single-kind drops removes the same cells, but it makes three passes over the data and has to be written out each time; the printed results above are identical.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::kindle;

auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});

auto out = x.dropinvalid();
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

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A catalog's streams may differ in length. For `drop...` the decision is made per position: a position is removed from **every** stream when **any** stream holds the targeted kind there, which keeps the streams aligned with one another.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Catalog::invalid`](invalid.md)
- [`Catalog::fillinvalid`](fillinvalid.md)
- [`Catalog::dropnull`](dropnull.md)
- [`Catalog::dropnan`](dropnan.md)
- [`Catalog::dropinf`](dropinf.md)
- [`Stream::dropinvalid`](../stream/dropinvalid.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropinf.html">
    <span class="label">Previous</span>
    <span class="title">dropinf</span>
  </a>
  <a class="next-link" href="dropnan.html">
    <span class="label">Next</span>
    <span class="title">dropnan</span>
  </a>
</div>
