<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 04dee8de14be -->

# Stream::fillnan

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::fillnan(double value) const
```

## What It Does

Returns the same structure with every NaN cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillnan(value)` does to each kind of cell:

| A cell holding | After `fillnan(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | `value` |
| `+inf` | unchanged |
| `-inf` | unchanged |
| `NULL` (missing) | `NULL` (unchanged) |

A missing cell is never filled by this method, because a missing cell holds no NaN to replace; use `fillnull` for that.

`Stream` is a `Shelf` with an optional label, so it asks the question of its shelf and carries the label (and any metadata) across to the result.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Stream`
    A stream of the same length; label and mask are kept.

## Examples

Example 1. Replace every NaN with `-1.0`.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Stream(values, present);

auto out = x.fillnan(-1.0);
std::cout << out << "\n";
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
1            | -1
2            | inf
3            | -inf
4            | NULL
5            | 5
========================================================================================================================
```

Every matching position now holds `-1.0`; the result is [1, -1, inf, -inf, NULL, 5]. The `NULL` position is not touched: it is still `NULL`.

Example 2. A fill never invents a value for a missing cell.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Stream(values, present);

auto out = x.fillnan(-1.0);
auto still_null = out.isnull();
std::cout << out << "\n";
std::cout << "isnull\n";
std::cout << still_null << "\n";
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
1            | -1
2            | inf
3            | -inf
4            | NULL
5            | 5
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
```

`fillnan` only touches NaN cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Stream(values, present);

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out << "\n";
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
```

`fillnan` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Stream::isnan`](isnan.md)
- [`Stream::dropnan`](dropnan.md)
- [`Stream::fillnull`](fillnull.md)
- [`Stream::fillinf`](fillinf.md)
- [`Stream::fillinvalid`](fillinvalid.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillinvalid.html">
    <span class="label">Previous</span>
    <span class="title">fillinvalid</span>
  </a>
  <a class="next-link" href="fillneginf.html">
    <span class="label">Next</span>
    <span class="title">fillneginf</span>
  </a>
</div>
