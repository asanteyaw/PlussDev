<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1caa2a4276e7 -->

# Axel::fillneginf

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::fillneginf(double value) const
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

`Axel` applies the [`Registry`](../registry/fillneginf.md) method to every registry in the stack and returns a new `Axel` with the same depth.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Axel`
    A stack of the same depth; every registry is filled.

## Examples

Example 1. Replace every -inf with `-1000.0`.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto first_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto first_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto first = Registry({{"a", first_a}, {"b", first_b}});
auto second_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({3.0}),
    pluss::make_series({4.0}), pluss::make_series({5.0}), pluss::make_series({6.0})});
auto second_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({20.0}), pluss::make_series({30.0}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto second = Registry({{"a", second_a}, {"b", second_b}});
auto x = Axel(std::vector<Registry>{first, second});

auto out = x.fillneginf(-1000.0);
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
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

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

Every matching cell now holds `-1000.0`: field `a` is [1, NaN, inf, -1000, NULL, 5] and field `b` is [10, NaN, NaN, 40, 50, 60]. The `NULL` position is not touched: it is still `NULL`. The second registry has no value other than ordinary numbers, so nothing changes.

Example 2. A fill never invents a value for a missing cell.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto first_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto first_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto first = Registry({{"a", first_a}, {"b", first_b}});
auto second_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({3.0}),
    pluss::make_series({4.0}), pluss::make_series({5.0}), pluss::make_series({6.0})});
auto second_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({20.0}), pluss::make_series({30.0}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto second = Registry({{"a", second_a}, {"b", second_b}});
auto x = Axel(std::vector<Registry>{first, second});

auto out = x.fillneginf(-1000.0);
auto still_null = out.isnull();
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
std::cout << "isnull\n";
std::cout << still_null.registry(0).fields("a") << "\n" << still_null.registry(1).fields("a") << "\n";
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

0  1
1  2
2  3
3  4
4  5
5  6
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

`fillneginf` only touches -inf cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); filling `NULL` is the job of `fillnull`.

Example 3. Give each kind its own value by chaining the fills.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto first_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto first_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto first = Registry({{"a", first_a}, {"b", first_b}});
auto second_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({3.0}),
    pluss::make_series({4.0}), pluss::make_series({5.0}), pluss::make_series({6.0})});
auto second_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({20.0}), pluss::make_series({30.0}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto second = Registry({{"a", second_a}, {"b", second_b}});
auto x = Axel(std::vector<Registry>{first, second});

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
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

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

`fillneginf` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the first registry is built from the six-row data and the second registry from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Axel::isneginf`](isneginf.md)
- [`Axel::dropneginf`](dropneginf.md)
- [`Axel::fillposinf`](fillposinf.md)
- [`Axel::fillinf`](fillinf.md)
- [`Registry::fillneginf`](../registry/fillneginf.md): the one-dimensional rule this type delegates to.

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
