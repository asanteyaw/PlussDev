<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9cf289680447 -->

# Nexus::dropinvalid

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::dropinvalid() const
```

## What It Does

Removes every row that is `NULL`, `NaN` or an infinity and returns the rest, in their original order. Nothing is repaired or replaced: the offending rows are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropinvalid()` does to each kind of cell:

| A cell holding | After `dropinvalid()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | removed |
| `+inf` | removed |
| `-inf` | removed |
| `NULL` (missing) | removed |

`Nexus` applies the [`DataFrame`](../dataframe/dropinvalid.md) method to every table in the workspace and returns a new `Nexus`; relationships and table metadata are kept.

## Parameters

`None`

## Returns

`Nexus`
    A workspace with the same names and relationships; each table is filtered on its own, so tables may end up with different numbers of rows. Relationships are by key, so they stay declared.

## Examples

Example 1. Remove the rows that hold a NULL, a NaN or an infinity.

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

auto out = x.dropinvalid();
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

A row goes when **any** of its features holds the targeted kind: rows 1, 2, 3, 4 are removed and rows 0, 5 remain, in their original order. The table `b` has no value other than ordinary numbers, so nothing is removed.

Example 2. One call instead of three: the result is the same as chaining the three single-kind drops.

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

auto chained = x.dropnull().dropnan().dropinf();
auto at_once = x.dropinvalid();
std::cout << "dropnull().dropnan().dropinf()\n";
std::cout << chained.frame("a") << "\n" << chained.frame("b") << "\n";
std::cout << "dropinvalid\n";
std::cout << at_once.frame("a") << "\n" << at_once.frame("b") << "\n";
```

### Output

```text
dropnull().dropnan().dropinf()
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
dropinvalid
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

`dropinvalid()` is the catch-all. Chaining the three single-kind drops removes the same cells, but it makes three passes over the data and has to be written out each time; the printed results above are identical.

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

auto out = x.dropinvalid();
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

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the table `a` is built from the six-row data and the table `b` from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Nexus::invalid`](invalid.md)
- [`Nexus::fillinvalid`](fillinvalid.md)
- [`Nexus::dropnull`](dropnull.md)
- [`Nexus::dropnan`](dropnan.md)
- [`Nexus::dropinf`](dropinf.md)
- [`DataFrame::dropinvalid`](../dataframe/dropinvalid.md): the one-dimensional rule this type delegates to.

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
