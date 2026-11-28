<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 023f895d7c1b -->

# Slot::fillinvalid

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::fillinvalid(double value) const
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

`Slot` holds the rule for scribble. A slot cell that is a one-element number is treated exactly like a `Stream` record. A cell that holds a list of numbers is flagged when **any** number inside it is flagged.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Slot`
    A slot of the same size; matching numbers (also inside list-valued cells) hold `value`.

## Examples

Example 1. Replace every NULL, NaN and infinity with `0.0`.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});

auto out = x.fillinvalid(0.0);
std::cout << out << "\n";
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
```

Every matching position now holds `0.0`; the result is [1, 0, 0, 0, 0, 5]. The `NULL` is filled too, because `fillinvalid` covers all three kinds.

Example 2. A different value for each kind: chain the single-kind fills instead.

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

`fillinvalid(value)` puts the **same** value everywhere, which is rarely what an analysis wants: a missing reading, an invalid computation and an unbounded one mean different things. Chaining `fillnull`, `fillnan`, `fillposinf` and `fillneginf` lets each kind get its own value: here `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100.

Example 3. Integer data holds nothing to fill (and has no NULL here), so it is unchanged.

```cpp
using namespace pluss::scribble;

auto x = Slot({
    pluss::make_series(std::vector<std::int64_t>{1}), pluss::make_series(std::vector<std::int64_t>{2}), pluss::make_series(std::vector<std::int64_t>{3}),
    pluss::make_series(std::vector<std::int64_t>{4}), pluss::make_series(std::vector<std::int64_t>{5}), pluss::make_series(std::vector<std::int64_t>{6})});

auto out = x.fillinvalid(0.0);
std::cout << out << "\n";
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
```

An integer cell cannot be `NaN` or infinite, and this data has no missing entries, so the call changes nothing.

Example 4. A fill reaches inside a cell that holds a list.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, nan}), pluss::make_series({2.0, inf}),
    pluss::make_series({-inf, 3.0}), std::monostate{}});

auto out = x.fillinvalid(0.0);
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
data=[1, 0]
2: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 0]
3: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 3]
4: pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

The fill reaches **inside** a list-valued cell: only the matching numbers are replaced by `0.0` and their neighbours are untouched. The empty cell is filled too (it becomes a one-number cell).

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A cell that holds a list of numbers is one cell: it is flagged, dropped or filled according to what is **inside** it. A cell that holds text, a timestamp or a duration cannot hold `NaN` or an infinity and is never flagged.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Slot::invalid`](invalid.md)
- [`Slot::dropinvalid`](dropinvalid.md)
- [`Slot::fillnull`](fillnull.md)
- [`Slot::fillnan`](fillnan.md)
- [`Slot::fillinf`](fillinf.md)

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
