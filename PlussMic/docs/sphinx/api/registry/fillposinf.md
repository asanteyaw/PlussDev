<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3b5f45564082 -->

# Registry::fillposinf

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::fillposinf(double value) const
```

## What It Does

Returns the same structure with every +inf cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillposinf(value)` does to each kind of cell:

| A cell holding | After `fillposinf(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | unchanged |
| `+inf` | `value` |
| `-inf` | unchanged |
| `NULL` (missing) | `NULL` (unchanged) |

A missing cell is never filled by this method, because a missing cell holds no +inf to replace; use `fillnull` for that.

`Registry` applies the [`Slot`](../slot/fillposinf.md) method to every field. The fields of a registry are independent lists, so each is processed on its own and no alignment between fields is assumed.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Registry`
    A registry of the same fields; every slot is filled.

## Examples

Example 1. Replace every +inf with `1000.0`.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto x_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto out = x.fillposinf(1000.0);
std::cout << out.fields("a") << "\n" << out.fields("b") << "\n";
```

### Output

```text
0  1
1  NaN
2  1000
3  -inf
4  NULL
5  5
ptype: Series

0  10
1  NaN
2  NaN
3  40
4  50
5  60
ptype: Series
```

Every matching cell now holds `1000.0`: field `a` is [1, NaN, 1000, -inf, NULL, 5] and field `b` is [10, NaN, NaN, 40, 50, 60]. The `NULL` position is not touched: it is still `NULL`.

Example 2. A fill never invents a value for a missing cell.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto x_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto out = x.fillposinf(1000.0);
auto still_null = out.isnull();
std::cout << out.fields("a") << "\n" << out.fields("b") << "\n";
std::cout << "isnull\n";
std::cout << still_null.fields("a") << "\n" << still_null.fields("b") << "\n";
```

### Output

```text
0  1
1  NaN
2  1000
3  -inf
4  NULL
5  5
ptype: Series

0  10
1  NaN
2  NaN
3  40
4  50
5  60
ptype: Series

isnull
0  false
1  false
2  false
3  false
4  true
5  false
ptype: Series

0  false
1  false
2  false
3  false
4  false
5  false
ptype: Series
```

`fillposinf` only touches +inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto x_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out.fields("a") << "\n" << out.fields("b") << "\n";
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

0  10
1  -1
2  -1
3  40
4  50
5  60
ptype: Series
```

`fillposinf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A registry's fields are independent lists (a `Registry` is not a table), so nothing is aligned between them: each field is processed on its own and may end up a different length.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Registry::isposinf`](isposinf.md)
- [`Registry::dropposinf`](dropposinf.md)
- [`Registry::fillneginf`](fillneginf.md)
- [`Registry::fillinf`](fillinf.md)
- [`Slot::fillposinf`](../slot/fillposinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
