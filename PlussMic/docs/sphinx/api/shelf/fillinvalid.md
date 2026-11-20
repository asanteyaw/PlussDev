<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 882fabd66512 -->

# Shelf::fillinvalid

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::fillinvalid(double value) const
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

`Shelf` holds the rule: the question is asked of the shelf's numeric payload directly, and the answer is combined with the validity mask so that a missing entry never counts as a value.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Shelf`
    A shelf of the same length; the matching entries hold `value`. A validity mask on the input is kept, so `NULL` entries stay `NULL`.

## Examples

Example 1. Replace every NULL, NaN and infinity with `0.0`.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

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
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

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
using namespace pluss::table;

auto x = Shelf(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}));

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

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Shelf::invalid`](invalid.md)
- [`Shelf::dropinvalid`](dropinvalid.md)
- [`Shelf::fillnull`](fillnull.md)
- [`Shelf::fillnan`](fillnan.md)
- [`Shelf::fillinf`](fillinf.md)

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
