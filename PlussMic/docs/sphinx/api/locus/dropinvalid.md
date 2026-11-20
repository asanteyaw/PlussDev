<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: db06b6a0ca54 -->

# Locus::dropinvalid

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::dropinvalid() const
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

`Locus` applies the [`Catalog`](../catalog/dropinvalid.md) method to every catalog in the workspace and returns a new `Locus`; links and table metadata are kept.

## Parameters

`None`

## Returns

`Locus`
    A workspace with the same names and links; each catalog is filtered on its own.

## Examples

Example 1. Remove the records that hold a NULL, a NaN or an infinity.

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

auto out = x.dropinvalid();
std::cout << out.catalog("a").keys("a") << "\n" << out.catalog("b").keys("a") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 5
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

A row goes when **any** of its features holds the targeted kind: rows 1, 2, 3, 4 are removed and rows 0, 5 remain, in their original order. The catalog `b` has no value other than ordinary numbers, so nothing is removed.

Example 2. One call instead of three: the result is the same as chaining the three single-kind drops.

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

auto chained = x.dropnull().dropnan().dropinf();
auto at_once = x.dropinvalid();
std::cout << "dropnull().dropnan().dropinf()\n";
std::cout << chained.catalog("a").keys("a") << "\n" << chained.catalog("b").keys("a") << "\n";
std::cout << "dropinvalid\n";
std::cout << at_once.catalog("a").keys("a") << "\n" << at_once.catalog("b").keys("a") << "\n";
```

### Output

```text
dropnull().dropnan().dropinf()
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 5
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

dropinvalid
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 5
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

`dropinvalid()` is the catch-all. Chaining the three single-kind drops removes the same cells, but it makes three passes over the data and has to be written out each time; the printed results above are identical.

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

auto out = x.dropinvalid();
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
1            | 2
2            | 3
3            | 4
4            | 5
5            | 6
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

Every value is an ordinary number, so there is nothing to drop and every row is still there.

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the catalog `a` is built from the six-row data and the catalog `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Locus::invalid`](invalid.md)
- [`Locus::fillinvalid`](fillinvalid.md)
- [`Locus::dropnull`](dropnull.md)
- [`Locus::dropnan`](dropnan.md)
- [`Locus::dropinf`](dropinf.md)
- [`Catalog::dropinvalid`](../catalog/dropinvalid.md): the one-dimensional rule this type delegates to.

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
