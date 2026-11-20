<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 767567ac4e9b -->

# Registry::fillinvalid

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::fillinvalid(double value) const
```

## What It Does

Returns the same structure with every `NULL`, `NaN` and infinity cell replaced by `value`. The shape does not change: nothing is removed, only replaced.

Here is what `fillinvalid(value)` does to each kind of cell:

| A cell holding | After `fillinvalid(value)` |
|---|---|
| An ordinary number (`1`, `5`) | unchanged |
| `NaN` | `value` |
| `+inf` | `value` |
| `-inf` | `value` |
| `NULL` (missing) | `value` |

`fillinvalid` is the **all-in-one** filler: it puts the same value everywhere. If a missing reading, an invalid computation and an unbounded one should become different values, chain the single-kind fills instead: `x.fillnull(a).fillnan(b).fillposinf(c).fillneginf(d)`.

`Registry` applies the [`Slot`](../slot/fillinvalid.md) method to every field. The fields of a registry are independent lists, so each is processed on its own and no alignment between fields is assumed.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Registry`
    A registry of the same fields; every slot is filled.

## Examples

Example 1. Replace every NULL, NaN and infinity with `0.0`.

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

auto out = x.fillinvalid(0.0);
std::cout << column(out.fields("a")) << "\n" << column(out.fields("b")) << "\n";
```

### Output

```text
0  1
1  0
2  0
3  0
4  0
5  5
ptype: Series

0  10
1  0
2  0
3  40
4  50
5  60
ptype: Series
```

Every matching cell now holds `0.0`: field `a` is [1, 0, 0, 0, 0, 5] and field `b` is [10, 0, 0, 40, 50, 60]. The `NULL` is filled too, because `fillinvalid` covers all three kinds.

Example 2. A different value for each kind: chain the single-kind fills instead.

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

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << column(out.fields("a")) << "\n" << column(out.fields("b")) << "\n";
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

`fillinvalid(value)` puts the **same** value everywhere, which is rarely what an analysis wants: a missing reading, an invalid computation and an unbounded one mean different things. Chaining `fillnull`, `fillnan`, `fillposinf` and `fillneginf` lets each kind get its own value: here `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100.

Example 3. Integer data holds nothing to fill (and has no NULL here), so it is unchanged.

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

auto out = x.fillinvalid(0.0);
std::cout << column(out.fields("a")) << "\n" << column(out.fields("b")) << "\n";
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

An integer cell cannot be `NaN` or infinite, and this data has no missing entries, so the call changes nothing.

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A registry's fields are independent lists (a `Registry` is not a table), so nothing is aligned between them: each field is processed on its own and may end up a different length.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Registry::invalid`](invalid.md)
- [`Registry::dropinvalid`](dropinvalid.md)
- [`Registry::fillnull`](fillnull.md)
- [`Registry::fillnan`](fillnan.md)
- [`Registry::fillinf`](fillinf.md)
- [`Slot::fillinvalid`](../slot/fillinvalid.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillinf.html">
    <span class="label">Previous</span>
    <span class="title">fillinf</span>
  </a>
  <a class="next-link" href="fillnan.html">
    <span class="label">Next</span>
    <span class="title">fillnan</span>
  </a>
</div>
