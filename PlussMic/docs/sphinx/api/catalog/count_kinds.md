<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b1e8fed01194 -->

# Catalog::count_kinds

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
pluss::table::DataFrame Catalog::count_kinds() const
```

## What It Does

Returns a `DataFrame` with one row per stream and the columns `null`, `nan`, `posinf`, `neginf`, `finite` and `total`: how many of its cells are missing, `NaN`, `+inf`, `-inf`, ordinary, and how many there are in all.

The four non-ordinary kinds never overlap, so `null + nan + posinf + neginf + finite == total` on every row. It is the quick way to audit data: one call tells you not only whether a column has bad cells but what kind they are, which decides the remedy:

| Count is non-zero | Usually means | Remedy |
|---|---|---|
| `null` | values were never recorded | `fillnull`, `dropnull` |
| `nan` | an invalid computation (`0/0`, `x % 0`, `sqrt(-1)`) | find the cause, then `fillnan` or `dropnan` |
| `posinf` / `neginf` | an unbounded one (`5/0`, `log(0)`) | `fillposinf` / `fillneginf`, or `dropinf` |

Streams of different lengths are counted on their own, so the report is a normal rectangular table even when the catalog is ragged.

## Parameters

`None`

## Returns

`DataFrame`
    One row per stream (labelled with its name); the integer columns `null`, `nan`, `posinf`, `neginf`, `finite` and `total`.

## Examples

Example 1. Count the cells of each kind in every stream.

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

auto report = x.count_kinds();
std::cout << report;
```

### Output

```text
================================================
     | null  nan   posinf  neginf  finite  total
------------------------------------------------
a    | 1     1     1       1       2       6
b    | 0     2     0       0       4       6
------------------------------------------------
[2 cs x 6 feat]
```

One row per stream. `a` has one cell of each kind (`NULL`, `NaN`, `+inf`, `-inf`) and two ordinary numbers; `b` has two `NaN` cells and four ordinary numbers. The kinds never overlap, so `null + nan + posinf + neginf + finite` is always `total`.

Example 2. Read one kind across all of them: which of my columns contain NaN?

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

auto report = x.count_kinds();
std::cout << report.features("nan");
std::cout << report.features("null");
```

### Output

```text
a  1
b  2
Name: nan, ptype: Series
a  1
b  0
Name: null, ptype: Series
```

Each column of the report is itself a column you can print, filter or compare. This is the quick audit: a non-zero `nan` or `null` count tells you which streams need attention and what kind of attention they need.

Example 3. A stream that cannot hold NaN or infinity reports zero of each.

```cpp
using namespace pluss::kindle;

auto x = Catalog(std::vector<std::pair<std::string, Stream>>{
    {"a", Stream(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
    {"b", Stream(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}))},
});

auto report = x.count_kinds();
std::cout << report;
```

### Output

```text
================================================
     | null  nan   posinf  neginf  finite  total
------------------------------------------------
a    | 0     0     0       0       6       6
b    | 0     0     0       0       6       6
------------------------------------------------
[2 cs x 6 feat]
```

Integer streams cannot hold `NaN` or an infinity and this data has no missing entries, so everything is `finite` and the other counts are `0`.

## Notes

- The report is a `DataFrame`, so everything you can do with one (`print`, `features("nan")`, filtering, sorting) works on it directly.
- A stream that cannot hold `NaN` or an infinity reports `0` for `nan`, `posinf` and `neginf`; its `null` count is its number of missing cells.
- Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows `NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.

## See Also

- [`isnull`](isnull.md) / [`isnan`](isnan.md) / [`isinf`](isinf.md): the per-cell questions behind the counts.
- [`invalid`](invalid.md): `true` for all three kinds at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="combine.html">
    <span class="label">Previous</span>
    <span class="title">combine</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
