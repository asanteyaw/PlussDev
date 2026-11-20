<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d3c5c72a0b7f -->

# Registry::isnan

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::isnan() const
```

## What It Does

Returns a Bool result of the same shape as the input that is `true` wherever a cell holds a `NaN`. The answer is about the **value** in the cell: a missing cell has no value, so it is never a `NaN` and never infinite.

Three different things can sit in a numeric cell, and PlussMic keeps them apart on purpose:

| A cell can be | It means | Ask about it with |
|---|---|---|
| `NULL` | no value was recorded (the validity mask says so) | `isnull`, `notnull`, `dropnull`, `fillnull` |
| `NaN` | a value that is not a number: `0/0`, `x % 0`, `sqrt(-1)` | `isnan`, `notnan`, `dropnan`, `fillnan` |
| `+inf` / `-inf` | a value that is unbounded: `5/0`, `log(0)`, overflow | `isinf`, `notinf`, `isposinf`, `isneginf`, `dropinf`, `dropposinf`, `dropneginf`, `fillinf`, `fillposinf`, `fillneginf` |
| any of the three | not an ordinary number | `invalid`, `dropinvalid`, `fillinvalid` |

`isnan()` answers one precise question. Here is what it says for each kind of cell:

| A cell holding | `isnan()` |
|---|---|
| An ordinary number (`1`, `5`) | `false` |
| `NaN` | `true` |
| `+inf` | `false` |
| `-inf` | `false` |
| `NULL` (missing) | `false` |

`Registry` applies the [`Slot`](../slot/isnan.md) method to every field. The fields of a registry are independent lists, so each is processed on its own and no alignment between fields is assumed.

## Parameters

`None`

## Returns

`Registry`
    A registry with the same field names, each slot replaced by its Bool slot.

## Examples

Example 1. Mark every cell that holds a `NaN`, in data that holds all four kinds of cell.

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

auto x_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto x_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto out = x.isnan();
std::cout << column(out.fields("a")) << "\n" << column(out.fields("b")) << "\n";
```

### Output

```text
0  0
1  1
2  0
3  0
4  0
5  0
ptype: Series

0  0
1  1
2  1
3  0
4  0
5  0
ptype: Series
```

Position 0 and 5 are ordinary numbers, position 1 is a `NaN`, positions 2 and 3 are `+inf` and `-inf`, and position 4 is `NULL`. For field `a` the answers are [false, true, false, false, false, false]; field `b` has `NaN` at positions 1 and 2 only, so its answers are [false, true, true, false, false, false].

Example 2. Ask the three questions on the same data, side by side.

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

auto x_a = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});
auto x_b = Slot({
    pluss::make_series({10.0}), pluss::make_series({nan}), pluss::make_series({nan}),
    pluss::make_series({40.0}), pluss::make_series({50.0}), pluss::make_series({60.0})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto missing = x.isnull();
auto asked = x.isnan();
auto either = x.invalid();
std::cout << "isnull\n";
std::cout << column(missing.fields("a")) << "\n" << column(missing.fields("b")) << "\n";
std::cout << "isnan\n";
std::cout << column(asked.fields("a")) << "\n" << column(asked.fields("b")) << "\n";
std::cout << "invalid\n";
std::cout << column(either.fields("a")) << "\n" << column(either.fields("b")) << "\n";
```

### Output

```text
isnull
0  0
1  0
2  0
3  0
4  1
5  0
ptype: Series

0  0
1  0
2  0
3  0
4  0
5  0
ptype: Series

isnan
0  0
1  1
2  0
3  0
4  0
5  0
ptype: Series

0  0
1  1
2  1
3  0
4  0
5  0
ptype: Series

invalid
0  0
1  1
2  1
3  1
4  1
5  0
ptype: Series

0  0
1  1
2  1
3  0
4  0
5  0
ptype: Series
```

`isnull` is `true` only for the `NULL` position. `isnan` answers the question about a `NaN` and is never `true` for `NULL`. `invalid` is `true` for all three kinds of not-an-ordinary-number at once: `NULL`, `NaN` and the infinities. The three are different questions; none of them is a synonym for another.

Example 3. Data that cannot hold NaN or infinity: integers give the same answer everywhere.

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

auto x_a = Slot({
    pluss::make_series(std::vector<std::int64_t>{1}), pluss::make_series(std::vector<std::int64_t>{2}), pluss::make_series(std::vector<std::int64_t>{3}),
    pluss::make_series(std::vector<std::int64_t>{4}), pluss::make_series(std::vector<std::int64_t>{5}), pluss::make_series(std::vector<std::int64_t>{6})});
auto x_b = Slot({
    pluss::make_series(std::vector<std::int64_t>{10}), pluss::make_series(std::vector<std::int64_t>{20}), pluss::make_series(std::vector<std::int64_t>{30}),
    pluss::make_series(std::vector<std::int64_t>{40}), pluss::make_series(std::vector<std::int64_t>{50}), pluss::make_series(std::vector<std::int64_t>{60})});
auto x = Registry({{"a", x_a}, {"b", x_b}});

auto out = x.isnan();
std::cout << column(out.fields("a")) << "\n" << column(out.fields("b")) << "\n";
```

### Output

```text
0  0
1  0
2  0
3  0
4  0
5  0
ptype: Series

0  0
1  0
2  0
3  0
4  0
5  0
ptype: Series
```

An integer cell is never a `NaN` and never infinite, so `isnan` is `false` for all of them. There is no error and no conversion to a floating type: the question simply has the answer "no".

## Notes

- `NULL` is **not** `NaN`. A missing cell has no value, so `isnan`/`isinf`/`isposinf`/`isneginf` are `false` for it and the `not...` forms are `true`. Ask `isnull` for missing cells and `invalid` for "missing or not an ordinary number".
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A registry's fields are independent lists (a `Registry` is not a table), so nothing is aligned between them: each field is processed on its own and may end up a different length.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Registry::notnan`](notnan.md)
- [`Registry::isinf`](isinf.md)
- [`Registry::isnull`](isnull.md)
- [`Registry::invalid`](invalid.md)
- [`Registry::dropnan`](dropnan.md)
- [`Registry::fillnan`](fillnan.md)
- [`Slot::isnan`](../slot/isnan.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isinf.html">
    <span class="label">Previous</span>
    <span class="title">isinf</span>
  </a>
  <a class="next-link" href="isneginf.html">
    <span class="label">Next</span>
    <span class="title">isneginf</span>
  </a>
</div>
