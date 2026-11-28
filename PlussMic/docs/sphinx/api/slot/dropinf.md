<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f49aac7ffb3f -->

# Slot::dropinf

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::dropinf() const
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

`Slot` holds the rule for scribble. A slot cell that is a one-element number is treated exactly like a `Stream` record. A cell that holds a list of numbers is flagged when **any** number inside it is flagged.

## Parameters

`None`

## Returns

`Slot`
    A new, shorter slot holding the cells that were kept, in order.

## Examples

Example 1. Remove the cells that hold infinity.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});

auto out = x.dropinf();
std::cout << out << "\n";
```

### Output

```text
0  1
1  NaN
2  NULL
3  5
ptype: Series
```

The original positions 2, 3 are removed; the other positions remain, in their original order, and the result is numbered again from 0.

Example 2. `dropnull` and `dropinf` remove different things.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}),
    pluss::make_series({-inf}), std::monostate{}, pluss::make_series({5.0})});

auto without_missing = x.dropnull();
auto without_this = x.dropinf();
std::cout << "dropnull\n";
std::cout << without_missing << "\n";
std::cout << "dropinf\n";
std::cout << without_this << "\n";
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

dropinf
0  1
1  NaN
2  NULL
3  5
ptype: Series
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropinf()` removes only the infinity positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::scribble;

auto x = Slot({
    pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({3.0}),
    pluss::make_series({4.0}), pluss::make_series({5.0}), pluss::make_series({6.0})});

auto out = x.dropinf();
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

Every value is an ordinary number, so there is nothing to drop and every row is still there.

Example 4. A cell that holds a list is dropped as a whole when the list contains the kind.

```cpp
using namespace pluss::scribble;

const double nan = pluss::series::scalar_nan;
const double inf = pluss::series::scalar_inf;

auto x = Slot({
    pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, nan}), pluss::make_series({2.0, inf}),
    pluss::make_series({-inf, 3.0}), std::monostate{}});

auto out = x.dropinf();
std::cout << "cells kept: " << out.size() << "\n";
for (std::int64_t i = 0; i < out.size(); ++i) {
    if (std::holds_alternative<std::monostate>(out.at(i))) { std::cout << i << ": NULL\n"; continue; }
    std::cout << i << ": " << std::get<pluss::Series>(out.at(i)) << "\n";
}
```

### Output

```text
cells kept: 3
0: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
1: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, nan]
2: NULL
```

The five cells are `{1, 2}`, `{1, NaN}`, `{2, +inf}`, `{-inf, 3}` and an empty cell. A cell is dropped as a whole when what is inside it matches, so the cells that remain are the original cells 0, 1, 4. Nothing inside a surviving cell is changed.

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A cell that holds a list of numbers is one cell: it is flagged, dropped or filled according to what is **inside** it. A cell that holds text, a timestamp or a duration cannot hold `NaN` or an infinity and is never flagged.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Slot::isinf`](isinf.md)
- [`Slot::fillinf`](fillinf.md)
- [`Slot::dropposinf`](dropposinf.md)
- [`Slot::dropneginf`](dropneginf.md)
- [`Slot::dropnan`](dropnan.md)
- [`Slot::dropinvalid`](dropinvalid.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="dropinvalid.html">
    <span class="label">Next</span>
    <span class="title">dropinvalid</span>
  </a>
</div>
