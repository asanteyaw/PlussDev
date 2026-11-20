<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3ad5abb41b25 -->

# DataFrame::fillneginf

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::fillneginf(double value) const

DataFrame DataFrame::fillneginf(double value, pluss::RangeView<std::string_view> features) const

template <detail::StringViewNameRange NameRange>
template <detail::StringViewNameRange NameRange> DataFrame DataFrame::fillneginf(double value, const NameRange& features) const
```

## What It Does

Returns the same structure with every -inf cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillneginf(value)` does to each kind of cell:

| A cell holding | After `fillneginf(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | unchanged |
| `+inf` | unchanged |
| `-inf` | `value` |
| `NULL` (missing) | `NULL` (unchanged) |

A missing cell is never filled by this method, because a missing cell holds no -inf to replace; use `fillnull` for that.

`DataFrame` applies the one-dimensional rule of [`Shelf`](../shelf/fillneginf.md) to its numeric features. Features of the same numeric type are processed together in one batch, so the cost grows with the amount of data, not with the number of features.

## Parameters

**value** (`double`)

    The number written into every matching cell.

**features** (range of names, optional)

    The features to look at. Omitted or empty means every feature. Other features are left as they are.

## Returns

`DataFrame`
    A frame of the same shape; the matching cells of the selected numeric features hold `value`. Other features are untouched.

## Examples

Example 1. Replace every -inf with `-1000.0`.

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

auto out = x.fillneginf(-1000.0);
std::cout << out << "\n";
```

### Output

```text
==================
     | a      b
------------------
0    |  1     10
1    |  NaN   NaN
2    |  inf   NaN
3    | -1000  40
4    | NULL   50
5    |  5     60
------------------
[6 cs x 2 feat]
```

Every matching cell now holds `-1000.0`: feature `a` is [1, NaN, inf, -1000, NULL, 5] and feature `b` is [10, NaN, NaN, 40, 50, 60]. The `NULL` position is not touched: it is still `NULL`.

Example 2. A fill never invents a value for a missing cell.

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

auto out = x.fillneginf(-1000.0);
auto still_null = out.isnull();
std::cout << out << "\n";
std::cout << "isnull\n";
std::cout << still_null << "\n";
```

### Output

```text
==================
     | a      b
------------------
0    |  1     10
1    |  NaN   NaN
2    |  inf   NaN
3    | -1000  40
4    | NULL   50
5    |  5     60
------------------
[6 cs x 2 feat]
isnull
===================
     | a      b
-------------------
0    | false  false
1    | false  false
2    | false  false
3    | false  false
4    | true   false
5    | false  false
-------------------
[6 cs x 2 feat]
```

`fillneginf` only touches -inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

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

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out << "\n";
```

### Output

```text
=================
     | a     b
-----------------
0    |  1     10
1    | -1    -1
2    |  100  -1
3    | -100   40
4    |  0     50
5    |  5     60
-----------------
[6 cs x 2 feat]
```

`fillneginf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

Example 4. Fill one feature only.

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

auto out = x.fillneginf(-1000.0, {"a"});
std::cout << out << "\n";
```

### Output

```text
==================
     | a      b
------------------
0    |  1     10
1    |  NaN   NaN
2    |  inf   NaN
3    | -1000  40
4    | NULL   50
5    |  5     60
------------------
[6 cs x 2 feat]
```

With `features` given, only those features are filled. The `NaN` cells of `b` stay `NaN`.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- `features` accepts a brace list of names (`{"a", "b"}`), a `std::vector<std::string>` or any other range of names. An unknown name throws a `pluss::Error`.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`DataFrame::isneginf`](isneginf.md)
- [`DataFrame::dropneginf`](dropneginf.md)
- [`DataFrame::fillposinf`](fillposinf.md)
- [`DataFrame::fillinf`](fillinf.md)
- [`Shelf::fillneginf`](../shelf/fillneginf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnan.html">
    <span class="label">Previous</span>
    <span class="title">fillnan</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
