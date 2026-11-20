<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3ffe74fd8a9c -->

# Nexus::dropneginf

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::dropneginf() const
```

## What It Does

Removes every row that holds `-inf` and returns the rest, in their original order. Nothing is repaired or replaced: the offending rows are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropneginf()` does to each kind of cell:

| A cell holding | After `dropneginf()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | kept |
| `+inf` | kept |
| `-inf` | removed |
| `NULL` (missing) | kept |

It is the exact counterpart of the matching `is...` method: `x.dropneginf()` keeps the positions where `x.isneginf()` is `false`.

`Nexus` applies the [`DataFrame`](../dataframe/dropneginf.md) method to every table in the workspace and returns a new `Nexus`; relationships and table metadata are kept.

## Parameters

`None`

## Returns

`Nexus`
    A workspace with the same names and relationships; each table is filtered on its own, so tables may end up with different numbers of rows. Relationships are by key, so they stay declared.

## Examples

Example 1. Remove the rows that hold -inf.

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

auto out = x.dropneginf();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
=================
     | a     b
-----------------
0    | 1     10
1    | NaN   NaN
2    | inf   NaN
4    | NULL  50
5    | 5     60
-----------------
[5 cs x 2 feat]
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

A row goes when **any** of its features holds the targeted kind: row 3 is removed and rows 0, 1, 2, 4, 5 remain, in their original order. The table `b` has no value other than ordinary numbers, so nothing is removed.

Example 2. `dropnull` and `dropneginf` remove different things.

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

auto without_missing = x.dropnull();
auto without_this = x.dropneginf();
std::cout << "dropnull\n";
std::cout << without_missing.frame("a") << "\n" << without_missing.frame("b") << "\n";
std::cout << "dropneginf\n";
std::cout << without_this.frame("a") << "\n" << without_this.frame("b") << "\n";
```

### Output

```text
dropnull
=================
     | a     b
-----------------
0    |  1    10
1    |  NaN  NaN
2    |  inf  NaN
3    | -inf  40
5    |  5    60
-----------------
[5 cs x 2 feat]
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
dropneginf
=================
     | a     b
-----------------
0    | 1     10
1    | NaN   NaN
2    | inf   NaN
4    | NULL  50
5    | 5     60
-----------------
[5 cs x 2 feat]
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

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropneginf()` removes only the -inf positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::bundle;
using namespace pluss::table;

auto a = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto b = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Nexus({{"a", a}, {"b", b}});

auto out = x.dropneginf();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
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

Every value is an ordinary number, so there is nothing to drop and every row is still there.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the table `a` is built from the six-row data and the table `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Nexus::isneginf`](isneginf.md)
- [`Nexus::fillneginf`](fillneginf.md)
- [`Nexus::dropposinf`](dropposinf.md)
- [`Nexus::dropinf`](dropinf.md)
- [`DataFrame::dropneginf`](../dataframe/dropneginf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnan.html">
    <span class="label">Previous</span>
    <span class="title">dropnan</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
