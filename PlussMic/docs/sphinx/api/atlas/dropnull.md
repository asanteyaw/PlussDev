<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ada690403abc -->

# Atlas::dropnull

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas Atlas::dropnull() const
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

`Atlas` applies the one-dimensional rule through [`DataFrame`](../dataframe/dropnull.md), but a drop has to keep the stack a strict cube (every frame the same height). So the decision is made across the whole stack: a row is removed from **every** frame when **any** frame holds the targeted kind anywhere in that row.

## Parameters

`None`

## Returns

`Atlas`
    A stack of the same depth. A row is removed from **every** frame when **any** frame holds the targeted kind anywhere in that row, so the frames keep the same number of rows and stay aligned row by row (an `Atlas` is a strict cube).

## Examples

Example 1. Remove the rows that are missing.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto frame = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto other = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Atlas(std::vector<DataFrame>{frame, other});

auto out = x.dropnull();
std::cout << out << "\n";
```

### Output

```text
Atlas(depth=2, rows=5, cols=2)
frame[0]
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
frame[1]
=================
     | a     b
-----------------
0    | 1     10
1    | 2     20
2    | 3     30
3    | 4     40
5    | 6     60
-----------------
[5 cs x 2 feat]
```

Only the `NULL` position (4) is removed. The `NaN` and the two infinities are values, so they stay. The second frame has no flagged cell of its own, but it loses the same rows: an `Atlas` is a strict cube, so every frame must keep the same number of rows and the drop is decided across the whole stack.

Example 2. Drop the missing cells and then the not-ordinary values: chain the three single-kind drops.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto frame = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});
auto other = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Atlas(std::vector<DataFrame>{frame, other});

auto out = x.dropnull().dropnan().dropinf();
std::cout << out << "\n";
```

### Output

```text
Atlas(depth=2, rows=1, cols=2)
frame[0]
=================
     | a     b
-----------------
0    | 1     10
-----------------
[1 cs x 2 feat]
frame[1]
=================
     | a     b
-----------------
0    | 1     10
-----------------
[1 cs x 2 feat]
```

`dropnull` takes out `NULL`, `dropnan` takes out `NaN` and `dropinf` takes out the infinities, leaving only ordinary numbers. `dropinvalid()` does the same in one call.

Example 3. Data with no missing cells comes back unchanged.

```cpp
using namespace pluss::table;

auto frame = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto other = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});
auto x = Atlas(std::vector<DataFrame>{frame, other});

auto out = x.dropnull();
std::cout << out << "\n";
```

### Output

```text
Atlas(depth=2, rows=6, cols=2)
frame[0]
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
frame[1]
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
- An `Atlas` requires every frame to have the same number of rows, so a drop cannot shorten one frame on its own. Row `r` goes from all frames when any frame has a flagged cell in row `r`; the frames stay aligned.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Atlas::isnull`](isnull.md)
- [`Atlas::fillnull`](fillnull.md)
- [`Atlas::dropnan`](dropnan.md)
- [`Atlas::dropinf`](dropinf.md)
- [`Atlas::dropinvalid`](dropinvalid.md)
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
