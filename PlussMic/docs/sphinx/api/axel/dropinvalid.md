<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c43774499269 -->

# Axel::dropinvalid

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::dropinvalid() const
```

## What It Does

Removes every cell that is `NULL`, `NaN` or an infinity and returns the rest, in their original order. Nothing is repaired or replaced: the offending cells are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropinvalid()` does to each kind of cell:

| A cell holding | After `dropinvalid()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | removed |
| `+inf` | removed |
| `-inf` | removed |
| `NULL` (missing) | removed |

`Axel` applies the [`Registry`](../registry/dropinvalid.md) method to every registry in the stack and returns a new `Axel` with the same depth.

## Parameters

`None`

## Returns

`Axel`
    A stack of the same depth; each registry is filtered on its own.

## Examples

Example 1. Remove the cells that hold a NULL, a NaN or an infinity.

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

auto out = x.dropinvalid();
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
```

### Output

```text
0  1
1  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

Each field is processed on its own: field `a` keeps positions 0, 5 and field `b` keeps positions 0, 3, 4, 5, so the two fields end up with different lengths. A registry's fields are independent lists, not columns of one table. The second registry has no value other than ordinary numbers, so nothing is removed.

Example 2. One call instead of three: the result is the same as chaining the three single-kind drops.

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

auto chained = x.dropnull().dropnan().dropinf();
auto at_once = x.dropinvalid();
std::cout << "dropnull().dropnan().dropinf()\n";
std::cout << chained.registry(0).fields("a") << "\n" << chained.registry(1).fields("a") << "\n";
std::cout << "dropinvalid\n";
std::cout << at_once.registry(0).fields("a") << "\n" << at_once.registry(1).fields("a") << "\n";
```

### Output

```text
dropnull().dropnan().dropinf()
0  1
1  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series

dropinvalid
0  1
1  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

`dropinvalid()` is the catch-all. Chaining the three single-kind drops removes the same cells, but it makes three passes over the data and has to be written out each time; the printed results above are identical.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::scribble;

auto first_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({3.0}),
    pluss::make_series({4.0}), pluss::make_series({5.0}), pluss::make_series({6.0})});
auto first_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({20.0}), pluss::make_series({30.0}),
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

auto out = x.dropinvalid();
std::cout << out.registry(0).fields("a") << "\n" << out.registry(1).fields("a") << "\n";
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

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

Every value is an ordinary number, so there is nothing to drop and every row is still there.

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the first registry is built from the six-row data and the second registry from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Axel::invalid`](invalid.md)
- [`Axel::fillinvalid`](fillinvalid.md)
- [`Axel::dropnull`](dropnull.md)
- [`Axel::dropnan`](dropnan.md)
- [`Axel::dropinf`](dropinf.md)
- [`Registry::dropinvalid`](../registry/dropinvalid.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropinf.html">
    <span class="label">Previous</span>
    <span class="title">dropinf</span>
  </a>
  <a class="next-link" href="dropnan.html">
    <span class="label">Next</span>
    <span class="title">dropnan</span>
  </a>
</div>
