<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2043fd940416 -->

# Axel::dropnan

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::dropnan() const
```

## What It Does

Removes every cell that holds a `NaN` and returns the rest, in their original order. Nothing is repaired or replaced: the offending cells are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropnan()` does to each kind of cell:

| A cell holding | After `dropnan()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | removed |
| `+inf` | kept |
| `-inf` | kept |
| `NULL` (missing) | kept |

It is the exact counterpart of the matching `is...` method: `x.dropnan()` keeps the positions where `x.isnan()` is `false`.

`Axel` applies the [`Registry`](../registry/dropnan.md) method to every registry in the stack and returns a new `Axel` with the same depth.

## Parameters

`None`

## Returns

`Axel`
    A stack of the same depth; each registry is filtered on its own.

## Examples

Example 1. Remove the cells that hold NaN.

```cpp
using namespace pluss::scribble;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

// A slot cell holds a Series; print a slot of one-number cells the way a column prints (an empty cell is NULL, a true/false cell is 1/0).
auto column = [](const Slot& slot) {
    std::vector<double> values;
    std::vector<bool> present;
    for (std::int64_t i = 0; i < slot.size(); ++i) {
        const auto& cell = slot.at(i);
        present.push_back(!std::holds_alternative<std::monostate>(cell));
        values.push_back(present.back() ? std::get<pluss::Series>(cell).to(pluss::codem::ScalarType::Float64).host<double>()[0] : 0.0);
    }
    return pluss::table::Shelf(pluss::make_series(values), pluss::make_series(present));
};

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

auto out = x.dropnan();
std::cout << column(out.registry(0).fields("a")) << "\n" << column(out.registry(1).fields("a")) << "\n";
```

### Output

```text
0  1
1  inf
2  -inf
3  NULL
4  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

Each field is processed on its own: field `a` keeps positions 0, 2, 3, 4, 5 and field `b` keeps positions 0, 3, 4, 5, so the two fields end up with different lengths. A registry's fields are independent lists, not columns of one table. The second registry has no value other than ordinary numbers, so nothing is removed.

Example 2. `dropnull` and `dropnan` remove different things.

```cpp
using namespace pluss::scribble;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

// A slot cell holds a Series; print a slot of one-number cells the way a column prints (an empty cell is NULL, a true/false cell is 1/0).
auto column = [](const Slot& slot) {
    std::vector<double> values;
    std::vector<bool> present;
    for (std::int64_t i = 0; i < slot.size(); ++i) {
        const auto& cell = slot.at(i);
        present.push_back(!std::holds_alternative<std::monostate>(cell));
        values.push_back(present.back() ? std::get<pluss::Series>(cell).to(pluss::codem::ScalarType::Float64).host<double>()[0] : 0.0);
    }
    return pluss::table::Shelf(pluss::make_series(values), pluss::make_series(present));
};

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

auto without_missing = x.dropnull();
auto without_this = x.dropnan();
std::cout << "dropnull\n";
std::cout << column(without_missing.registry(0).fields("a")) << "\n" << column(without_missing.registry(1).fields("a")) << "\n";
std::cout << "dropnan\n";
std::cout << column(without_this.registry(0).fields("a")) << "\n" << column(without_this.registry(1).fields("a")) << "\n";
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

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series

dropnan
0  1
1  inf
2  -inf
3  NULL
4  5
ptype: Series

0  1
1  2
2  3
3  4
4  5
5  6
ptype: Series
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropnan()` removes only the NaN positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::scribble;

// A slot cell holds a Series; print a slot of one-number cells the way a column prints (an empty cell is NULL, a true/false cell is 1/0).
auto column = [](const Slot& slot) {
    std::vector<double> values;
    std::vector<bool> present;
    for (std::int64_t i = 0; i < slot.size(); ++i) {
        const auto& cell = slot.at(i);
        present.push_back(!std::holds_alternative<std::monostate>(cell));
        values.push_back(present.back() ? std::get<pluss::Series>(cell).to(pluss::codem::ScalarType::Float64).host<double>()[0] : 0.0);
    }
    return pluss::table::Shelf(pluss::make_series(values), pluss::make_series(present));
};

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

auto out = x.dropnan();
std::cout << column(out.registry(0).fields("a")) << "\n" << column(out.registry(1).fields("a")) << "\n";
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

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the first registry is built from the six-row data and the second registry from clean data, which is why the second printed result differs.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Axel::isnan`](isnan.md)
- [`Axel::fillnan`](fillnan.md)
- [`Axel::dropnull`](dropnull.md)
- [`Axel::dropinf`](dropinf.md)
- [`Axel::dropinvalid`](dropinvalid.md)
- [`Registry::dropnan`](../registry/dropnan.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropinvalid.html">
    <span class="label">Previous</span>
    <span class="title">dropinvalid</span>
  </a>
  <a class="next-link" href="dropneginf.html">
    <span class="label">Next</span>
    <span class="title">dropneginf</span>
  </a>
</div>
