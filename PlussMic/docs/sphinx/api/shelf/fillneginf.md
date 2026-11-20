<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 52d1ec4bfc0d -->

# Shelf::fillneginf

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::fillneginf(double value) const
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

`Shelf` holds the rule: the question is asked of the shelf's numeric payload directly, and the answer is combined with the validity mask so that a missing entry never counts as a value.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Shelf`
    A shelf of the same length; the matching entries hold `value`. A validity mask on the input is kept, so `NULL` entries stay `NULL`.

## Examples

Example 1. Replace every -inf with `-1000.0`.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

auto out = x.fillneginf(-1000.0);
std::cout << out << "\n";
```

### Output

```text
0  1
1  NaN
2  inf
3  -1000
4  NULL
5  5
ptype: Series
```

Every matching position now holds `-1000.0`; the result is [1, NaN, inf, -1000, NULL, 5]. The `NULL` position is not touched: it is still `NULL`.

Example 2. A fill never invents a value for a missing cell.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

auto out = x.fillneginf(-1000.0);
auto still_null = out.isnull();
std::cout << out << "\n";
std::cout << "isnull\n";
std::cout << still_null << "\n";
```

### Output

```text
0  1
1  NaN
2  inf
3  -1000
4  NULL
5  5
ptype: Series

isnull
0  false
1  false
2  false
3  false
4  true
5  false
ptype: Series
```

`fillneginf` only touches -inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out << "\n";
```

### Output

```text
0  1
1  -1
2  100
3  -100
4  0
5  5
ptype: Series
```

`fillneginf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Shelf::isneginf`](isneginf.md)
- [`Shelf::dropneginf`](dropneginf.md)
- [`Shelf::fillposinf`](fillposinf.md)
- [`Shelf::fillinf`](fillinf.md)

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
