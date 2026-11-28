<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2321a2a7ebea -->

# Slot::fillinf

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::fillinf(double value) const
```

## What It Does

Returns the same structure with every infinity cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillinf(value)` does to each kind of cell:

| A cell holding | After `fillinf(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | unchanged |
| `+inf` | `value` |
| `-inf` | `value` |
| `NULL` (missing) | `NULL` (unchanged) |

A missing cell is never filled by this method, because a missing cell holds no infinity to replace; use `fillnull` for that.

`Slot` holds the rule for scribble. A slot cell that is a one-element number is treated exactly like a `Stream` record. A cell that holds a list of numbers is flagged when **any** number inside it is flagged.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Slot`
    A slot of the same size; matching numbers (also inside list-valued cells) hold `value`.

## Examples

Example 1. Replace every infinity with `999.0`.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});

auto out = x.fillinf(999.0);
std::cout << out << "\n";
```

### Output

```text
0  1
1  NaN
2  999
3  999
4  NULL
5  5
ptype: Series
```

Every matching position now holds `999.0`; the result is [1, NaN, 999, 999, NULL, 5]. The `NULL` position is not touched: it is still `NULL`.

Example 2. A fill never invents a value for a missing cell.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});

auto out = x.fillinf(999.0);
auto still_null = out.isnull();
std::cout << out << "\n";
std::cout << "isnull\n";
std::cout << still_null << "\n";
```

### Output

```text
0  1
1  NaN
2  999
3  999
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

`fillinf` only touches infinity cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});

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

`fillinf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

Example 4. A fill reaches inside a cell that holds a list.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, nan}), pluss::make_series({2.0, inf}),
    pluss::make_series({-inf, 3.0}), std::monostate{}});

auto out = x.fillinf(999.0);
for (std::int64_t i = 0; i < out.size(); ++i) {
    if (std::holds_alternative<std::monostate>(out.at(i))) { std::cout << i << ": NULL\n"; continue; }
    std::cout << i << ": " << std::get<pluss::Series>(out.at(i)) << "\n";
}
```

### Output

```text
0: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
1: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, nan]
2: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 999]
3: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[999, 3]
4: NULL
```

The fill reaches **inside** a list-valued cell: only the matching numbers are replaced by `999.0` and their neighbours are untouched. The empty cell is still empty (`NULL`).

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A cell that holds a list of numbers is one cell: it is flagged, dropped or filled according to what is **inside** it. A cell that holds text, a timestamp or a duration cannot hold `NaN` or an infinity and is never flagged.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Slot::isinf`](isinf.md)
- [`Slot::dropinf`](dropinf.md)
- [`Slot::fillposinf`](fillposinf.md)
- [`Slot::fillneginf`](fillneginf.md)
- [`Slot::fillnan`](fillnan.md)
- [`Slot::fillinvalid`](fillinvalid.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="fillinvalid.html">
    <span class="label">Next</span>
    <span class="title">fillinvalid</span>
  </a>
</div>
