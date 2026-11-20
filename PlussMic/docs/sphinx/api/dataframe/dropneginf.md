<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c085f852180a -->

# DataFrame::dropneginf

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::dropneginf(pluss::RangeView<std::string_view> features = {}, bool all = false) const

template <detail::StringViewNameRange NameRange>
template <detail::StringViewNameRange NameRange> DataFrame DataFrame::dropneginf(const NameRange& features, bool all = false) const
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

`DataFrame` applies the one-dimensional rule of [`Shelf`](../shelf/dropneginf.md) to its numeric features. Features of the same numeric type are processed together in one batch, so the cost grows with the amount of data, not with the number of features.

## Parameters

**features** (range of names, optional)

    The features to look at. Omitted or empty means every feature. Only these features decide which rows are removed.

**all** (`bool`, default `false`)

    `false`: a row is removed when **any** selected feature holds the targeted kind. `true`: a row is removed only when **every** selected feature does (a selected feature that cannot hold the kind never matches).

## Returns

`DataFrame`
    A new frame holding the rows that were kept, with every feature present. Case labels follow their rows.

## Examples

Example 1. Remove the rows that hold -inf.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto out = x.dropneginf();
std::cout << out << "\n";
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
```

A row goes when **any** of its features holds the targeted kind: row 3 is removed and rows 0, 1, 2, 4, 5 remain, in their original order.

Example 2. `dropnull` and `dropneginf` remove different things.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto without_missing = x.dropnull();
auto without_this = x.dropneginf();
std::cout << "dropnull\n";
std::cout << without_missing << "\n";
std::cout << "dropneginf\n";
std::cout << without_this << "\n";
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
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropneginf()` removes only the -inf positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::table;

auto x = DataFrame({
    {"a", Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}))},
    {"b", Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}))},
});

auto out = x.dropneginf();
std::cout << out << "\n";
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
```

Every value is an ordinary number, so there is nothing to drop and every row is still there.

Example 4. Decide by one feature only.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto out = x.dropneginf({"a"});
std::cout << out << "\n";
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
```

Only feature `a` is examined, so only row 3 goes. Whatever `b` holds does not matter here (its `NaN` in row 2 stays), and `b` is still in the result.

Example 5. `all = true`: drop a row only when every selected feature holds the targeted kind.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = DataFrame({
    {"a", Shelf(values, present)},
    {"b", Shelf(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto out = x.dropneginf({"a", "b"}, true);
std::cout << out << "\n";
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
4    | NULL  50
5    |  5    60
-----------------
[6 cs x 2 feat]
```

With `all = true` a row goes only when **both** `a` and `b` hold it. That is true for rows none only, so rows 0, 1, 2, 3, 4, 5 remain.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- `features` accepts a brace list of names (`{"a", "b"}`), a `std::vector<std::string>` or any other range of names. An unknown name throws a `pluss::Error`.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`DataFrame::isneginf`](isneginf.md)
- [`DataFrame::fillneginf`](fillneginf.md)
- [`DataFrame::dropposinf`](dropposinf.md)
- [`DataFrame::dropinf`](dropinf.md)
- [`Shelf::dropneginf`](../shelf/dropneginf.md): the one-dimensional rule this type delegates to.

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
