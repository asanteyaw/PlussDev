<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6ad0f4c7c034 -->

# Nexus::dropnull

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::dropnull() const
```

## What It Does

Removes every row that is `NULL` and returns the rest, in their original order. Nothing is repaired or replaced: the offending rows are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropnull()` does to each kind of cell:

| A cell holding | After `dropnull()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | kept |
| `+inf` | kept |
| `-inf` | kept |
| `NULL` (missing) | removed |

`Nexus` applies the [`DataFrame`](../dataframe/dropnull.md) method to every table in the workspace and returns a new `Nexus`; relationships and table metadata are kept.

## Parameters

`None`

## Returns

`Nexus`
    A workspace with the same names and relationships; each table is filtered on its own, so tables may end up with different numbers of rows. Relationships are by key, so they stay declared.

## Examples

Example 1. Remove the rows that are missing.

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

auto out = x.dropnull();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
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
```

Only the `NULL` position (4) is removed. The `NaN` and the two infinities are values, so they stay. The table `b` has no value other than ordinary numbers, so nothing is removed.

Example 2. Drop the missing cells and then the not-ordinary values: chain the three single-kind drops.

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

auto out = x.dropnull().dropnan().dropinf();
std::cout << out.frame("a") << "\n" << out.frame("b") << "\n";
```

### Output

```text
=================
     | a     b
-----------------
0    | 1     10
5    | 5     60
-----------------
[2 cs x 2 feat]
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

`dropnull` takes out `NULL`, `dropnan` takes out `NaN` and `dropinf` takes out the infinities, leaving only ordinary numbers. `dropinvalid()` does the same in one call.

Example 3. Data with no missing cells comes back unchanged.

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

auto out = x.dropnull();
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

There is no `NULL` anywhere, so nothing is dropped.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the table `a` is built from the six-row data and the table `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Nexus::isnull`](isnull.md)
- [`Nexus::fillnull`](fillnull.md)
- [`Nexus::dropnan`](dropnan.md)
- [`Nexus::dropinf`](dropinf.md)
- [`Nexus::dropinvalid`](dropinvalid.md)
- [`DataFrame::dropnull`](../dataframe/dropnull.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropneginf.html">
    <span class="label">Previous</span>
    <span class="title">dropneginf</span>
  </a>
  <a class="next-link" href="dropposinf.html">
    <span class="label">Next</span>
    <span class="title">dropposinf</span>
  </a>
</div>
