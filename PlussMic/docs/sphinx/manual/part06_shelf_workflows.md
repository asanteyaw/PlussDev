# Part VI: Shelf Workflows

`Shelf` is PlussMic's column model. Every `DataFrame` feature is a named
`Shelf`, and many table operations delegate their real work to Shelf methods.
If `DataFrame` is the table abstraction, `Shelf` is the typed one-dimensional
array abstraction underneath it.

A Shelf has four important pieces of state:

- a payload family: `Series`, `Note`, `Timeline`, or `TimeIndex`;
- an optional validity mask for missing values;
- optional display labels;
- an optional display name.

The payload family controls which operations are legal. Numeric arithmetic is
for `Series` shelves. Text cleaning is for `Note` shelves. Calendar extraction
is for `Timeline` and `TimeIndex` shelves. Missingness is tracked separately
through the validity mask, so a row can have a stored payload value and still
display as `NA`.

## Payload families

`ShelfKind` identifies the payload held by a `Shelf`:

| Kind | Payload | Accessor |
|---|---|---|
| `Series` | Numeric values | `as_series()` |
| `Note` | Text values | `as_note()` |
| `Timeline` | Timestamps | `as_timeline()` |
| `Duration` | Time intervals | `as_duration()` |

Use the family-specific accessors only after you know the kind:

```cpp
if (shelf.kind() == pluss::table::ShelfKind::Series) {
  const auto& values = shelf.as_series();
}

if (shelf.kind() == pluss::table::ShelfKind::Note) {
  const auto& text = shelf.as_note();
}
```

Calling the wrong accessor is an error. For example, `as_series()` is only valid
for a `Series` payload.

## Construct shelves

Numeric values normally enter a Shelf through `pluss::make_series`:

```cpp
pluss::table::Shelf numeric = pluss::make_series({1.0, 2.0, 3.0});
```

Text values normally enter through `pluss::make_note`:

```cpp
pluss::table::Shelf text = pluss::make_note({"alpha", "beta", "gamma"});
```

You can attach a validity mask at construction time:

```cpp
auto values = pluss::make_series({4.0, 1.0, 0.0, 3.0, 2.0});
auto valid = pluss::series::zeros({5}, pluss::codem::ScalarType::Bool);
auto* valid_ptr = reinterpret_cast<bool*>(valid.raw_data());
valid_ptr[0] = true;
valid_ptr[1] = true;
valid_ptr[2] = false;
valid_ptr[3] = true;
valid_ptr[4] = true;

pluss::table::Shelf score(values, valid);
```

The third row stores `0.0` in the payload, but its mask value is false. Display
and null-aware operations treat that row as missing.

## Inspect a Shelf

Use these methods when writing generic code:

```cpp
shelf.defined();
shelf.unique();
shelf.size();
shelf.kind();
shelf.dtype();
shelf.isnull();
shelf.notnull();
```

`dtype()` reports the payload dtype, such as `float64` or `utf8`. It does
not tell you whether the shelf has missing values. Use `isnull()` or
`notnull()` for that.

Display and debug methods:

```cpp
shelf.info();
shelf.show();
std::cout << shelf.to_string() << '\n';
```

`to_string()` is useful in tests, examples, and documentation because it uses
the same display path a user sees.

## Verified example: numeric and text operations

The following example is compiled and run by
`PlussMic/docs/sphinx/manual/examples/plussmic_manual_examples.cpp`.

```cpp
auto sales = sales_frame();
auto units = sales.features("units");
auto city = sales.features("city");

std::cout << "units dtype=" << units.dtype() << '\n';
std::cout << "units clipped:\n" << units.clip(6.0, 10.0).to_string() << '\n';
std::cout << "city lower:\n" << city.lower().to_string() << '\n';
std::cout << "city contains 'a':\n" << Shelf(city.contains("a")).to_string() << '\n';
```

Output:

```text
units dtype=float64
units clipped:
0  10
1  7
2  10
3  6
ptype: Series
city lower:
0  accra
1  tema
2  kumasi
3  tamale
ptype: Note
city contains 'a':
0  true
1  true
2  true
3  true
ptype: Series
```

`features("units")` returns a numeric Series-backed Shelf. `clip(6.0, 10.0)`
clamps values below `6` upward and values above `10` downward. The original
unit values are `10`, `7`, `12`, and `5`, so the result is `10`, `7`, `10`,
and `6`.

`features("city")` returns a Note-backed Shelf. `lower()` returns lower-case
city names. `contains("a")` returns a boolean `Series`, which the example wraps
as a `Shelf` only so it prints in the same format.

## Row-space operations

A Shelf is one-dimensional, so row-space operations work directly on element
positions:

```cpp
shelf.head(5);
shelf.tail(5);
shelf.view(10, 20);
shelf.slice(10, 20);
shelf.take(std::vector<std::int64_t>{0, 3, 8});
shelf.shift(1);
shelf.arrange(true, true);
shelf.dropnull();
shelf.distinct(true);
```

`head`, `tail`, and `view` return `ShelfView`, which can auto-render. `slice`,
`take`, `shift`, `arrange`, `dropnull`, and `distinct` return ordinary
`Shelf` values.

`slice(start, stop)` is half-open. `take(indices)` keeps exactly the requested
positions in exactly that order. `shift(periods)` moves values by row offset
and marks the newly exposed edge as missing. `arrange(ascending,
nulls_last)` sorts the valid values and places masked rows according to
`nulls_last`.

## Missingness

Missingness is mask-based. A Shelf without a validity mask has no mask-level
missing values. A Shelf with a validity mask uses `true` for present and
`false` for missing.

Common methods:

```cpp
auto missing = shelf.isnull();
auto present = shelf.notnull();
auto complete = shelf.dropnull();
auto forward = shelf.ffill();
auto backward = shelf.bfill();
```

Numeric checks are separate:

```cpp
auto nan = shelf.isnan();
auto finite = shelf.isfinite();
```

`isnan()` checks floating-point NaN in a Series payload. `isnull()` checks the
Shelf validity mask. A value can be finite but masked out, and it will still
display as `NA`.

## Verified example: missingness and row operations

```cpp
Shelf score(
    pluss::make_series({4.0, 1.0, 0.0, 3.0, 2.0}),
    bool_series({true, true, false, true, true}));

std::cout << "score:\n" << score.to_string() << '\n';
std::cout << "isnull:\n" << score.isnull().to_string() << '\n';
std::cout << "fillnull:\n" << score.fillnull(99.0).to_string() << '\n';
std::cout << "dropnull:\n" << score.dropnull().to_string() << '\n';
std::cout << "shift(2):\n" << score.shift(2).to_string() << '\n';
std::cout << "sort asc nulls first:\n" << score.arrange(true, false).to_string() << '\n';
std::cout << "take 4,1,0:\n" << score.take(std::vector<std::int64_t>{4, 1, 0}).to_string() << '\n';
```

Output:

```text
score:
0  4
1  1
2  NA
3  3
4  2
ptype: Series
isnull:
0  false
1  false
2  true
3  false
4  false
ptype: Series
fillnull:
0  4
1  1
2  99
3  3
4  2
ptype: Series
dropnull:
0  4
1  1
2  3
3  2
ptype: Series
shift(2):
0  NA
1  NA
2  4
3  1
4  NA
ptype: Series
sort asc nulls first:
0  NA
1  1
2  2
3  3
4  4
ptype: Series
take 4,1,0:
0  2
1  1
2  4
ptype: Series
```

The `score` payload has five numeric values, but row 2 is masked as missing,
so it displays as `NA`. `isnull()` is the logical inverse of the validity mask.

`fillnull(99.0)` keeps the valid values `4`, `1`, `3`, and `2`, and replaces only
the masked row with `99`. `dropnull()` removes the masked row. `shift(2)` moves
values down by two rows, marks rows 0 and 1 missing because there are no source
values for them, and also carries the original missing row forward so row 4 is
missing. `arrange` with `nulls_last=false` places the missing value first,
then sorts valid values ascending. `take({4, 1, 0})` keeps rows 4, 1, and 0, in
that order.

## Numeric operations

Series-backed Shelves support arithmetic, comparisons, math transforms, rolling
windows, and scalar reductions.

Arithmetic can be written through methods or operators:

```cpp
auto plus_two = shelf + 2.0;
auto doubled = shelf * 2.0;
auto ratio = numerator / denominator;
auto remainder = shelf % 2.0;
```

Comparison operators return boolean Series-backed Shelves:

```cpp
auto positive = shelf > 0.0;
auto in_limit = shelf <= 100.0;
auto same = left == right;
```

Named numeric transforms:

```cpp
auto absolute = shelf.abs();
auto clipped = shelf.clip(0.0, 100.0);
auto logged = shelf.log();
auto exponentiated = shelf.exp();
```

Reductions return scalar `Series` values, not raw C++ scalars:

```cpp
double mean = shelf.mean().item<double>();
double total = shelf.sum().item<double>();
double sd = shelf.std(false).item<double>();
```

Available reductions include:

```cpp
shelf.sum();
shelf.count();
shelf.any();
shelf.all();
shelf.mean();
shelf.median();
shelf.min();
shelf.max();
shelf.var();
shelf.std();
shelf.corr(other);
```

`corr` accepts a method:

```cpp
double pearson = x.corr(y);
double spearman = x.corr(y, pluss::table::ShelfCorrMethod::Spearman);
double kendall = x.corr(y, pluss::table::ShelfCorrMethod::KendallTau);
```

## Comparisons and bands

Use `compare` when the comparison operator is data-driven:

```cpp
auto gt_zero = shelf.compare(0.0, pluss::table::ShelfBinaryCompareOp::Gt);
auto le_other = shelf.compare(other, pluss::table::ShelfBinaryCompareOp::Le);
```

Use `between` for interval checks:

```cpp
auto inclusive = shelf.between(10.0, 20.0, true);
auto half_open = shelf.between(10.0, 20.0, true, false);
auto text_band = text.between("a", "m", true);
```

For DataFrame workflows, `df.query("age between 10 and 20")` is often more
readable. Direct Shelf comparisons are useful when you are constructing masks
in C++.

## Rolling windows

Use `window` for simple rolling aggregations over numeric Series payloads:

```cpp
auto rolling_sum = shelf.window(3, pluss::table::ShelfWindowOp::RollingSum, 1);
auto rolling_mean = shelf.window(3, pluss::table::ShelfWindowOp::RollingMean, 2);
auto rolling_min = shelf.window(5, pluss::table::ShelfWindowOp::RollingMin, 3);
auto rolling_max = shelf.window(5, pluss::table::ShelfWindowOp::RollingMax, 3);
```

The arguments are:

- `window`: number of rows in the trailing window;
- `op`: rolling operation;
- `min_periods`: minimum number of valid observations required for an output
  value.

If a row does not have enough valid observations, the output row is masked and
prints as `NA`.

## Shelf time-series accessor

`shelf.ts()` is for one numeric value stream. It does not need a separate
date feature for operations such as returns, lag, lead, rolling mean, or
expanding statistics.

```cpp
auto returns = price.ts().returns(1);
auto log_returns = price.ts().log_returns(1);
auto diff = price.ts().diff(1);
auto lagged = price.ts().lag(1);
auto leading = price.ts().lead(1);
auto rolling = price.ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 10});
auto z = price.ts().zscore_rolling(20, 10);
```

Use DataFrame time-series APIs when you need to resample or align one or more
value features against an explicit time feature.

## Casting

Use `cast` to change payload family:

```cpp
auto as_note = shelf.cast(pluss::table::ShelfCastTarget::Note);
auto as_series = text.cast(pluss::table::ShelfCastTarget::Series,
                           pluss::codem::ScalarType::Float64);
auto as_time = date_text.cast(pluss::table::ShelfCastTarget::Timeline);
auto as_index = date_text.cast(pluss::table::ShelfCastTarget::TimeIndex);
```

Use `astype` as a shorthand for Series dtype conversion:

```cpp
auto as_i64 = numeric.astype(pluss::codem::ScalarType::Int64);
auto as_f64 = numeric.astype(pluss::codem::ScalarType::Float64);
```

Casting rules are family-specific:

- Series to Series changes numeric dtype.
- Series to Note formats each scalar as text.
- Series to Timeline treats numeric values as either epoch nanoseconds or
  `YYYYMMDD` values when all valid rows look like dates.
- Note to Series parses bools or numbers depending on the requested dtype.
- Note to Timeline parses date/time text.
- Timeline and TimeIndex can be converted to Note or numeric epoch-nanosecond
  Series.

Parsing failures become invalid rows where supported by the implementation.

## Text operations

Note-backed Shelves support text cleaning:

```cpp
auto lowered = text.lower();
auto replaced = text.replace("old", "new");
auto mask = text.contains("needle");
```

`contains` returns a `Series`, not a `Shelf`. Wrap it in `Shelf(...)` when
you want Shelf-style display.

## Verified example: windows, casts, text, and returns

```cpp
Shelf values = pluss::make_series({1.0, 2.0, 4.0, 8.0, 16.0});
Shelf labels = pluss::make_note({"10", "20", "bad", "40"});
Shelf words = pluss::make_note({"North-East", "South", "north-west", "Central"});

std::cout << "values + 3:\n" << (values + 3.0).to_string() << '\n';
std::cout << "between 2 and 8 inclusive:\n" << values.between(2.0, 8.0, true).to_string() << '\n';
std::cout << "rolling mean window 3 min 2:\n"
          << values.window(3, pluss::table::ShelfWindowOp::RollingMean, 2).to_string() << '\n';
std::cout << "returns:\n" << values.ts().returns(1).to_string() << '\n';
std::cout << "label cast to float64:\n"
          << labels.cast(pluss::table::ShelfCastTarget::Series,
                         pluss::codem::ScalarType::Float64).to_string()
          << '\n';
std::cout << "words replace/lower:\n" << words.replace("-", " ").lower().to_string() << '\n';
std::cout << "words contains north:\n" << Shelf(words.lower().contains("north")).to_string() << '\n';
```

Output:

```text
values + 3:
0  4
1  5
2  7
3  11
4  19
ptype: Series
between 2 and 8 inclusive:
0  false
1  true
2  true
3  true
4  false
ptype: Series
rolling mean window 3 min 2:
0  NA
1  1.5
2  2.333333333
3  4.666666667
4  9.333333333
ptype: Series
returns:
0  nan
1  1
2  1
3  1
4  1
ptype: Series
label cast to float64:
0  10
1  20
2  NA
3  40
ptype: Series
words replace/lower:
0  north east
1  south
2  north west
3  central
ptype: Note
words contains north:
0  true
1  false
2  true
3  false
ptype: Series
```

`values + 3` performs elementwise numeric addition. `between(2.0, 8.0, true)` marks
the values `2`, `4`, and `8` as true because the interval is inclusive.

The rolling mean uses a trailing window of three rows and requires at least two
observations. Row 0 has only one observation, so it prints `NA`. Row 1 averages
`1` and `2`, giving `1.5`. Row 4 averages `4`, `8`, and `16`, giving
`9.333333333`.

`returns(1)` computes one-period fractional returns. The first row has no
previous value and prints `nan`; every later value doubles from the previous
row, so each return is `1`.

Casting `labels` to a Float64 Series parses `"10"`, `"20"`, and `"40"`. The
string `"bad"` cannot be parsed as a number, so that row is invalid and prints
`NA`.

The text example first replaces hyphens with spaces and then lowercases. The
contains check is performed on the lowercased values, so it matches both
`North-East` and `north-west`.

## Time operations

Timeline and TimeIndex payloads support calendar extraction:

```cpp
auto years = dates.tl().year();
auto months = dates.tl().month();
auto days = dates.tl().day();
auto hours = dates.tl().hour();
auto weekdays = dates.tl().weekday_name();
```

These return Shelves so they can be inserted back into a DataFrame:

```cpp
df.set("year") = df.features("timestamp").tl().year();
```

If you need to parse string dates, cast a Note-backed Shelf to Timeline first:

```cpp
auto timeline = text_dates.cast().timeline();
auto months = timeline.tl().month();
```

## Display labels and names

Display metadata is useful for presentation, but it should not be treated as
schema truth.

```cpp
auto named = shelf.name("balance");
auto labeled = named.with_labels(labels);

named.show();
std::cout << named.to_string() << '\n';
```

For DataFrames, feature names are the schema. Shelf display names are just
presentation metadata.

## Functional and mutating methods

Shelf transforms are functional by default:

```cpp
auto filled = shelf.fillnull(0.0);
auto clipped = shelf.clip(0.0, 1.0);
auto shifted = shelf.shift(1);
```

The original Shelf is unchanged unless you use a mutating method with a
trailing underscore:

```cpp
shelf.fillnull_(0.0);
```

Use mutating methods when mutation is intentional and local. Use functional
methods when building pipelines or returning values from helper functions.

## Choosing Shelf or DataFrame APIs

Use Shelf directly when:

- you are transforming one feature;
- you need a reusable mask;
- you are writing a DataFrame `assign` lambda;
- you need payload-specific methods such as `lower`, `contains`, `year`, or
  `window`;
- you want scalar reductions such as `mean().item<double>()`.

Use DataFrame methods when:

- the operation uses multiple features;
- you need to preserve feature names;
- you need table-shaped output;
- you are loading, saving, joining, grouping, or pivoting.

The common pattern is to use DataFrame for structure and Shelf for feature
logic:

```cpp
auto enriched = df.assign({{
    "clean_city",
    [](const pluss::table::DataFrame& frame) {
      return frame.features("city").replace("-", " ").lower();
    },
}});
```
