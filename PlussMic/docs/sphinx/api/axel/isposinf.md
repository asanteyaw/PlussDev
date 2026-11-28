<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b17f909dd0e -->

# Axel::isposinf

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::isposinf() const
```

## What It Does

Returns a Bool result of the same shape as the input that is `true` wherever a cell holds `+inf`. The answer is about the **value** in the cell: a missing cell has no value, so it is never a `NaN` and never infinite.

Three different things can sit in a numeric cell, and PlussMic keeps them apart on purpose:

| A cell can be | It means | Ask about it with |
|---|---|---|
| `NULL` | no value was recorded (the validity mask says so) | `isnull`, `notnull`, `dropnull`, `fillnull` |
| `NaN` | a value that is not a number: `0/0`, `x % 0`, `sqrt(-1)` | `isnan`, `notnan`, `dropnan`, `fillnan` |
| `+inf` / `-inf` | a value that is unbounded: `5/0`, `log(0)`, overflow | `isinf`, `notinf`, `isposinf`, `isneginf`, `dropinf`, `dropposinf`, `dropneginf`, `fillinf`, `fillposinf`, `fillneginf` |
| any of the three | not an ordinary number | `invalid`, `dropinvalid`, `fillinvalid` |

`isposinf()` answers one precise question. Here is what it says for each kind of cell:

| A cell holding | `isposinf()` |
|---|---|
| An ordinary number (`1`, `5`) | `false` |
| `NaN` | `false` |
| `+inf` | `true` |
| `-inf` | `false` |
| `NULL` (missing) | `false` |

`Axel` applies the [`Registry`](../registry/isposinf.md) method to every registry in the stack and returns a new `Axel` with the same depth.

## Parameters

`None`

## Returns

`Axel`
    A stack of the same depth; every registry replaced by its Bool registry.

## Examples

Example 1. Mark every cell that holds `+inf`, in data that holds all four kinds of cell.

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

auto out = x.isposinf();
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
```

### Output

```text
0  false
1  false
2  true
3  false
4  false
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

Position 0 and 5 are ordinary numbers, position 1 is a `NaN`, positions 2 and 3 are `+inf` and `-inf`, and position 4 is `NULL`. For field `a` the answers are [false, false, true, false, false, false]; field `b` has `NaN` at positions 1 and 2 only, so its answers are [false, false, false, false, false, false]. The second registry has no value other than ordinary numbers, so every answer is `false`.

Example 2. Ask the three questions on the same data, side by side.

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

auto missing = x.isnull();
auto asked = x.isposinf();
auto either = x.invalid();
std::cout << "isnull\n";
std::cout << missing.registry(0).fields("a") << "\n" << missing.registry(1).fields("a") << "\n";
std::cout << "isposinf\n";
std::cout << asked.registry(0).fields("a") << "\n" << asked.registry(1).fields("a") << "\n";
std::cout << "invalid\n";
std::cout << either.registry(0).fields("a") << "\n" << either.registry(1).fields("a") << "\n";
```

### Output

```text
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

isposinf
0  false
1  false
2  true
3  false
4  false
5  false
ptype: Series

0  false
1  false
2  false
3  false
4  false
5  false
ptype: Series

invalid
0  false
1  true
2  true
3  true
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

`isnull` is `true` only for the `NULL` position. `isposinf` answers the question about `+inf` and is never `true` for `NULL`. `invalid` is `true` for all three kinds of not-an-ordinary-number at once: `NULL`, `NaN` and the infinities. The three are different questions; none of them is a synonym for another.

Example 3. Data that cannot hold NaN or infinity: integers give the same answer everywhere.

```cpp
using namespace pluss::scribble;

auto first_a = Slot({
    pluss::make_series(std::vector<std::int64_t>{1}), pluss::make_series(std::vector<std::int64_t>{2}), pluss::make_series(std::vector<std::int64_t>{3}),
    pluss::make_series(std::vector<std::int64_t>{4}), pluss::make_series(std::vector<std::int64_t>{5}), pluss::make_series(std::vector<std::int64_t>{6})});
auto first_b = Slot({
    pluss::make_series(std::vector<std::int64_t>{10}), pluss::make_series(std::vector<std::int64_t>{20}), pluss::make_series(std::vector<std::int64_t>{30}),
    pluss::make_series(std::vector<std::int64_t>{40}), pluss::make_series(std::vector<std::int64_t>{50}), pluss::make_series(std::vector<std::int64_t>{60})});
auto first = Registry({{"a", first_a}, {"b", first_b}});
auto second_a = Slot({
    pluss::make_series(std::vector<std::int64_t>{1}), pluss::make_series(std::vector<std::int64_t>{2}), pluss::make_series(std::vector<std::int64_t>{3}),
    pluss::make_series(std::vector<std::int64_t>{4}), pluss::make_series(std::vector<std::int64_t>{5}), pluss::make_series(std::vector<std::int64_t>{6})});
auto second_b = Slot({
    pluss::make_series(std::vector<std::int64_t>{10}), pluss::make_series(std::vector<std::int64_t>{20}), pluss::make_series(std::vector<std::int64_t>{30}),
    pluss::make_series(std::vector<std::int64_t>{40}), pluss::make_series(std::vector<std::int64_t>{50}), pluss::make_series(std::vector<std::int64_t>{60})});
auto second = Registry({{"a", second_a}, {"b", second_b}});
auto x = Axel(std::vector<Registry>{first, second});

auto out = x.isposinf();
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
```

### Output

```text
0  false
1  false
2  false
3  false
4  false
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

An integer cell is never a `NaN` and never infinite, so `isposinf` is `false` for all of them. There is no error and no conversion to a floating type: the question simply has the answer "no".

## Notes

- `NULL` is **not** `NaN`. A missing cell has no value, so `isnan`/`isinf`/`isposinf`/`isneginf` are `false` for it and the `not...` forms are `true`. Ask `isnull` for missing cells and `invalid` for "missing or not an ordinary number".
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the first registry is built from the six-row data and the second registry from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Axel::isneginf`](isneginf.md)
- [`Axel::isinf`](isinf.md)
- [`Axel::dropposinf`](dropposinf.md)
- [`Axel::fillposinf`](fillposinf.md)
- [`Registry::isposinf`](../registry/isposinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
