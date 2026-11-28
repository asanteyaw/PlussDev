<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dd4470ac55ff -->

# Registry::dropinf

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::dropinf() const
```

## What It Does

Removes every cell that holds an infinity and returns the rest, in their original order. Nothing is repaired or replaced: the offending cells are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropinf()` does to each kind of cell:

| A cell holding | After `dropinf()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | kept |
| `+inf` | removed |
| `-inf` | removed |
| `NULL` (missing) | kept |

It is the exact counterpart of the matching `is...` method: `x.dropinf()` keeps the positions where `x.isinf()` is `false`.

`Registry` applies the [`Slot`](../slot/dropinf.md) method to every field. The fields of a registry are independent lists, so each is processed on its own and no alignment between fields is assumed.

## Parameters

`None`

## Returns

`Registry`
    A new registry; each field is a shorter slot. Fields may end up with different lengths.

## Examples

Example 1. Remove the cells that hold infinity.

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

auto out = x.dropinf();
std::cout << out.fields("a") << "\n" << out.fields("b") << "\n";
```

### Output

```text
0  1
1  NaN
2  NULL
3  5
ptype: Series

0  10
1  NaN
2  NaN
3  40
4  50
5  60
ptype: Series
```

Each field is processed on its own: field `a` keeps positions 0, 1, 4, 5 and field `b` keeps positions 0, 1, 2, 3, 4, 5, so the two fields end up with different lengths. A registry's fields are independent lists, not columns of one table.

Example 2. `dropnull` and `dropinf` remove different things.

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

auto without_missing = x.dropnull();
auto without_this = x.dropinf();
std::cout << "dropnull\n";
std::cout << without_missing.fields("a") << "\n" << without_missing.fields("b") << "\n";
std::cout << "dropinf\n";
std::cout << without_this.fields("a") << "\n" << without_this.fields("b") << "\n";
```

### Output

```text
dropnull
0  1
1  NaN
2  inf
3  -inf
4  5
ptype: Series

0  10
1  NaN
2  NaN
3  40
4  50
5  60
ptype: Series

dropinf
0  1
1  NaN
2  NULL
3  5
ptype: Series

0  10
1  NaN
2  NaN
3  40
4  50
5  60
ptype: Series
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropinf()` removes only the infinity positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::scribble;

auto x_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({3.0}),
    pluss::make_series({4.0}), pluss::make_series({5.0}), pluss::make_series({6.0})});
auto x_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({20.0}), pluss::make_series({30.0}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto out = x.dropinf();
std::cout << out.fields("a") << "\n" << out.fields("b") << "\n";
```

### Output

```text
0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series

0  10
1  20
2  30
3  40
4  50
5  60
ptype: Series
```

Every value is an ordinary number, so there is nothing to drop and every row is still there.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A registry's fields are independent lists (a `Registry` is not a table), so nothing is aligned between them: each field is processed on its own and may end up a different length.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Registry::isinf`](isinf.md)
- [`Registry::fillinf`](fillinf.md)
- [`Registry::dropposinf`](dropposinf.md)
- [`Registry::dropneginf`](dropneginf.md)
- [`Registry::dropnan`](dropnan.md)
- [`Registry::dropinvalid`](dropinvalid.md)
- [`Slot::dropinf`](../slot/dropinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="dropinvalid.html">
    <span class="label">Next</span>
    <span class="title">dropinvalid</span>
  </a>
</div>
