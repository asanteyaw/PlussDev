<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 92974f2252d8 -->

# Catalog::fillinvalid

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::fillinvalid(double value) const
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

`Catalog` applies the [`Stream`](../stream/fillinvalid.md) method to every stream. Streams may have different lengths; nothing is padded.

## Parameters

**value** (`double`)

    The number written into every matching cell.

## Returns

`Catalog`
    A catalog of the same shape; every stream is filled.

## Examples

Example 1. Replace every NULL, NaN and infinity with `0.0`.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto out = x.fillinvalid(0.0);
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 0
2            | 0
3            | 0
4            | 0
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 10
1            | 0
2            | 0
3            | 40
4            | 50
5            | 60
========================================================================================================================
```

Every matching cell now holds `0.0`: stream `a` is [1, 0, 0, 0, 0, 5] and stream `b` is [10, 0, 0, 40, 50, 60]. The `NULL` is filled too, because `fillinvalid` covers all three kinds.

Example 2. A different value for each kind: chain the single-kind fills instead.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(values, present)},
    {"b", Stream(pluss::make_series({10.0, nan, nan, 40.0, 50.0, 60.0}))},
});

auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | -1
2            | 100
3            | -100
4            | 0
5            | 5
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 10
1            | -1
2            | -1
3            | 40
4            | 50
5            | 60
========================================================================================================================
```

`fillinvalid(value)` puts the **same** value everywhere, which is rarely what an analysis wants: a missing reading, an invalid computation and an unbounded one mean different things. Chaining `fillnull`, `fillnan`, `fillposinf` and `fillneginf` lets each kind get its own value: here `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100.

Example 3. Integer data holds nothing to fill (and has no NULL here), so it is unchanged.

```cpp
using namespace pluss::kindle;

auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
    {"b", Stream(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}))},
});

auto out = x.fillinvalid(0.0);
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 1
1            | 2
2            | 3
3            | 4
4            | 5
5            | 6
========================================================================================================================

========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=6
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value
------------------------------------------------------------------------------------------------------------------------
0            | 10
1            | 20
2            | 30
3            | 40
4            | 50
5            | 60
========================================================================================================================
```

An integer cell cannot be `NaN` or infinite, and this data has no missing entries, so the call changes nothing.

## Notes

- This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- A catalog's streams may differ in length. For `drop...` the decision is made per position: a position is removed from **every** stream when **any** stream holds the targeted kind there, which keeps the streams aligned with one another.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Catalog::invalid`](invalid.md)
- [`Catalog::dropinvalid`](dropinvalid.md)
- [`Catalog::fillnull`](fillnull.md)
- [`Catalog::fillnan`](fillnan.md)
- [`Catalog::fillinf`](fillinf.md)
- [`Stream::fillinvalid`](../stream/fillinvalid.md): the one-dimensional rule this type delegates to.

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
