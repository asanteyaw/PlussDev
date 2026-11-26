<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 495d3eeee86b -->

# Stream::dropposinf

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::dropposinf() const
```

## What It Does

Removes every record that holds `+inf` and returns the rest, in their original order. Nothing is repaired or replaced: the offending records are simply taken out.

The cells of the other kinds are left exactly as they were. Here is what `dropposinf()` does to each kind of cell:

| A cell holding | After `dropposinf()` |
|---|---|
| An ordinary number (`1`, `5`) | kept |
| `NaN` | kept |
| `+inf` | removed |
| `-inf` | kept |
| `NULL` (missing) | kept |

It is the exact counterpart of the matching `is...` method: `x.dropposinf()` keeps the positions where `x.isposinf()` is `false`.

`Stream` is a `Shelf` with an optional label, so it asks the question of its shelf and carries the label (and any metadata) across to the result.

## Parameters

`None`

## Returns

`Stream`
    A new, shorter stream; the label is filtered to the same records.

## Examples

Example 1. Remove the records that hold +inf.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Stream(values, present);

auto out = x.dropposinf();
std::cout << out << "\n";
```

### Output

```text
0  1
1  NaN
2  -inf
3  NULL
4  5
ptype: Series
```

The original position 2 is removed; the other positions remain, in their original order, and the result is numbered again from 0.

Example 2. `dropnull` and `dropposinf` remove different things.

```cpp
using namespace pluss::kindle;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Stream(values, present);

auto without_missing = x.dropnull();
auto without_this = x.dropposinf();
std::cout << "dropnull\n";
std::cout << without_missing << "\n";
std::cout << "dropposinf\n";
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

dropposinf
0  1
1  NaN
2  -inf
3  NULL
4  5
ptype: Series
```

`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. `dropposinf()` removes only the +inf positions and leaves the `NULL` in place. Neither one is a replacement for the other.

Example 3. Data with nothing to remove comes back unchanged.

```cpp
using namespace pluss::kindle;

auto x = Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}));

auto out = x.dropposinf();
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

## Notes

- `NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Stream::isposinf`](isposinf.md)
- [`Stream::fillposinf`](fillposinf.md)
- [`Stream::dropneginf`](dropneginf.md)
- [`Stream::dropinf`](dropinf.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
