<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8154e7971d5f -->

# DataFrame::fillinvalid

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::fillinvalid(double value) const

DataFrame DataFrame::fillinvalid(double value, pluss::RangeView<std::string_view> features) const
```

## What It Does

Returns the same structure with every `NULL`, `NaN` and infinity cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillinvalid(value)` does to each kind of cell:

| A cell holding | After `fillinvalid(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | `value` |
| `+inf` | `value` |
| `-inf` | `value` |
| `NULL` (missing) | `value` |

`fillinvalid` is the **all-in-one** filler: it puts the same value everywhere. If a missing reading, an invalid computation and an unbounded one should become different values, chain the single-kind fills instead: `x.fillnull(a).fillnan(b).fillposinf(c).fillneginf(d)`.

`DataFrame` applies the one-dimensional rule of [`Shelf`](../shelf/fillinvalid.md) to its numeric features. Features of the same numeric type are processed together in one batch, so the cost grows with the amount of data, not with the number of features.

## Parameters

**value** (`double`)

    The number written into every matching cell.

**features** (range of names, optional)

    The features to look at. Omitted or empty means every feature. Other features are left as they are.

## Returns

`DataFrame`
    A frame of the same shape; the matching cells of the selected numeric features hold `value`. Other features are untouched.

## Examples

Example 1. Replace every NULL, NaN and infinity with `0.0`.

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

auto out = x.fillinvalid(0.0);
std::cout << out << "\n";
```

### Output

```text
=================
     | a     b
-----------------
0    | 1     10
1    | 0     0
2    | 0     0
3    | 0     40
4    | 0     50
5    | 5     60
-----------------
[6 cs x 2 feat]
```

Every matching cell now holds `0.0`: feature `a` is [1, 0, 0, 0, 0, 5] and feature `b` is [10, 0, 0, 40, 50, 60]. The `NULL` is filled too, because `fillinvalid` covers all three kinds.

Example 2. A different value for each kind: chain the single-kind fills instead.

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

`fillinvalid(value)` puts the **same** value everywhere, which is rarely what an analysis wants: a missing reading, an invalid computation and an unbounded one mean different things. Chaining `fillnull`, `fillnan`, `fillposinf` and `fillneginf` lets each kind get its own value: here `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100.

Example 3. Integer data holds nothing to fill (and has no NULL here), so it is unchanged.

```cpp
using namespace pluss::table;

auto x = DataFrame({
    {"a", Shelf(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
    {"b", Shelf(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}))},
});

auto out = x.fillinvalid(0.0);
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

An integer cell cannot be `NaN` or infinite, and this data has no missing entries, so the call changes nothing.

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

auto out = x.fillinvalid(0.0, {"a"});
std::cout << out << "\n";
```

### Output

```text
=================
     | a     b
-----------------
0    | 1     10
1    | 0     NaN
2    | 0     NaN
3    | 0     40
4    | 0     50
5    | 5     60
-----------------
[6 cs x 2 feat]
```

With `features` given, only those features are filled. The `NaN` cells of `b` stay `NaN`.

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- `features` accepts a brace list of names (`{"a", "b"}`), a `std::vector<std::string>` or any other range of names. An unknown name throws a `pluss::Error`.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`DataFrame::invalid`](invalid.md)
- [`DataFrame::dropinvalid`](dropinvalid.md)
- [`DataFrame::fillnull`](fillnull.md)
- [`DataFrame::fillnan`](fillnan.md)
- [`DataFrame::fillinf`](fillinf.md)
- [`Shelf::fillinvalid`](../shelf/fillinvalid.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillinf.html">
    <span class="label">Previous</span>
    <span class="title">fillinf</span>
  </a>
  <a class="next-link" href="fillnan.html">
    <span class="label">Next</span>
    <span class="title">fillnan</span>
  </a>
</div>
