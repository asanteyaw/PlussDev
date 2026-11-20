<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 61e2f0e99dd7 -->

# Catalog::fillneginf

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::fillneginf(double value) const
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

`Catalog` applies the [`Stream`](../stream/fillneginf.md) method to every stream. Streams may have different lengths; nothing is padded.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Catalog`
    A catalog of the same shape; every stream is filled.

## Examples

Example 1. Replace every -inf with `-1000.0`.

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

auto out = x.fillneginf(-1000.0);
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | NaN
2            | inf
3            | -1000
4            | NULL
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 10
1            | NaN
2            | NaN
3            | 40
4            | 50
5            | 60
========================================================================================================================
```

Every matching cell now holds `-1000.0`: stream `a` is [1, NaN, inf, -1000, NULL, 5] and stream `b` is [10, NaN, NaN, 40, 50, 60]. The `NULL` position is not touched: it is still `NULL`.

Example 2. A fill never invents a value for a missing cell.

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

auto out = x.fillneginf(-1000.0);
auto still_null = out.isnull();
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
std::cout << "isnull\n";
std::cout << still_null.keys("a") << "\n" << still_null.keys("b") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | NaN
2            | inf
3            | -1000
4            | NULL
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 10
1            | NaN
2            | NaN
3            | 40
4            | 50
5            | 60
========================================================================================================================

isnull
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | false
1            | false
2            | false
3            | false
4            | true
5            | false
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | false
1            | false
2            | false
3            | false
4            | false
5            | false
========================================================================================================================
```

`fillneginf` only touches -inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

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

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
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
1            | -1
2            | 100
3            | -100
4            | 0
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 10
1            | -1
2            | -1
3            | 40
4            | 50
5            | 60
========================================================================================================================
```

`fillneginf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A catalog's streams may differ in length. For `drop...` the decision is made per position: a position is removed from **every** stream when **any** stream holds the targeted kind there, which keeps the streams aligned with one another.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Catalog::isneginf`](isneginf.md)
- [`Catalog::dropneginf`](dropneginf.md)
- [`Catalog::fillposinf`](fillposinf.md)
- [`Catalog::fillinf`](fillinf.md)
- [`Stream::fillneginf`](../stream/fillneginf.md): the one-dimensional rule this type delegates to.

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
