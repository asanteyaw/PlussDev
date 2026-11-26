<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9360b042c536 -->

# Catalog::isinf

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::isinf() const
```

## What It Does

Returns a Bool result of the same shape as the input that is `true` wherever a cell holds an infinity (`+inf` or `-inf`). The answer is about the **value** in the cell: a missing cell has no value, so it is never a `NaN` and never infinite.

Three different things can sit in a numeric cell, and PlussMic keeps them apart on purpose:

| A cell can be | It means | Ask about it with |
|---|---|---|
| `NULL` | no value was recorded (the validity mask says so) | `isnull`, `notnull`, `dropnull`, `fillnull` |
| `NaN` | a value that is not a number: `0/0`, `x % 0`, `sqrt(-1)` | `isnan`, `notnan`, `dropnan`, `fillnan` |
| `+inf` / `-inf` | a value that is unbounded: `5/0`, `log(0)`, overflow | `isinf`, `notinf`, `isposinf`, `isneginf`, `dropinf`, `dropposinf`, `dropneginf`, `fillinf`, `fillposinf`, `fillneginf` |
| any of the three | not an ordinary number | `invalid`, `dropinvalid`, `fillinvalid` |

`isinf()` answers one precise question. Here is what it says for each kind of cell:

| A cell holding | `isinf()` |
|---|---|
| An ordinary number (`1`, `5`) | `false` |
| `NaN` | `false` |
| `+inf` | `true` |
| `-inf` | `true` |
| `NULL` (missing) | `false` |

`Catalog` applies the [`Stream`](../stream/isinf.md) method to every stream. Streams may have different lengths; nothing is padded.

## Parameters

`None`

## Returns

`Catalog`
    A catalog with the same keys, each stream replaced by its Bool stream.

## Examples

Example 1. Mark every record that holds an infinity (`+inf` or `-inf`), in data that holds all four kinds of cell.

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

auto out = x.isinf();
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
```

### Output

```text
0  false
1  false
2  true
3  true
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

Position 0 and 5 are ordinary numbers, position 1 is a `NaN`, positions 2 and 3 are `+inf` and `-inf`, and position 4 is `NULL`. For stream `a` the answers are [false, false, true, true, false, false]; stream `b` has `NaN` at positions 1 and 2 only, so its answers are [false, false, false, false, false, false].

Example 2. Ask the three questions on the same data, side by side.

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

auto missing = x.isnull();
auto asked = x.isinf();
auto either = x.invalid();
std::cout << "isnull\n";
std::cout << missing.keys("a") << "\n" << missing.keys("b") << "\n";
std::cout << "isinf\n";
std::cout << asked.keys("a") << "\n" << asked.keys("b") << "\n";
std::cout << "invalid\n";
std::cout << either.keys("a") << "\n" << either.keys("b") << "\n";
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

isinf
0  false
1  false
2  true
3  true
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
1  true
2  true
3  false
4  false
5  false
ptype: Series
```

`isnull` is `true` only for the `NULL` position. `isinf` answers the question about an infinity and is never `true` for `NULL`. `invalid` is `true` for all three kinds of not-an-ordinary-number at once: `NULL`, `NaN` and the infinities. The three are different questions; none of them is a synonym for another.

Example 3. Data that cannot hold NaN or infinity: integers give the same answer everywhere.

```cpp
using namespace pluss::kindle;

auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
    {"b", Stream(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}))},
});

auto out = x.isinf();
std::cout << out.keys("a") << "\n" << out.keys("b") << "\n";
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

An integer record is never a `NaN` and never infinite, so `isinf` is `false` for all of them. There is no error and no conversion to a floating type: the question simply has the answer "no".

## Notes

- `NULL` is **not** `NaN`. A missing cell has no value, so `isnan`/`isinf`/`isposinf`/`isneginf` are `false` for it and the `not...` forms are `true`. Ask `isnull` for missing cells and `invalid` for "missing or not an ordinary number".
- Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.
- Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.
- A catalog's streams may differ in length. For `drop...` the decision is made per position: a position is removed from **every** stream when **any** stream holds the targeted kind there, which keeps the streams aligned with one another.
- A `Timeline`'s "not a time" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`Catalog::notinf`](notinf.md)
- [`Catalog::isposinf`](isposinf.md)
- [`Catalog::isneginf`](isneginf.md)
- [`Catalog::isnan`](isnan.md)
- [`Catalog::isnull`](isnull.md)
- [`Catalog::dropinf`](dropinf.md)
- [`Catalog::fillinf`](fillinf.md)
- [`Stream::isinf`](../stream/isinf.md): the one-dimensional rule this type delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="isnan.html">
    <span class="label">Next</span>
    <span class="title">isnan</span>
  </a>
</div>
