<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f3b5ffd0411 -->

# DataFrameLabelAccessor::distinct

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
Slate DataFrameLabelAccessor::distinct(bool dropnull = true) const
```

## What It Does

Returns the unique case-label values, in first-occurrence order. With
`dropnull = false`, a single `NULL` is kept (at the position of its first
occurrence) if any case label is null; with the default `dropnull = true`,
`NULL` is excluded entirely.

## Parameters

**dropnull** (`bool`, default `true`)

Whether to exclude a null case label from the result. When `false`, at most one
`NULL` entry is kept.

## Returns

`Slate`
    The distinct case-label values, in first-occurrence order.

## Examples

Example 1. Case labels with one repeated value.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

auto out = labeled.labels().distinct();
std::cout << out;
```

### Output

```text
0  east
1  west
2  north
ptype: Note
```

`"east"` appears twice (rows `0` and `2`) but only once in the result, at the
position of its first occurrence; `"west"` and `"north"` each appear once.

Example 2. Pass `dropnull = false` to keep one `NULL` entry instead of
excluding it.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({100.0, 150.0, 200.0, 90.0, 50.0}))}},
    Slate(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(),
                               20.0, std::numeric_limits<double>::quiet_NaN(), 10.0})));
std::cout << readings;

auto without_null = readings.labels().distinct();
std::cout << without_null;

auto with_null = readings.labels().distinct(false);
std::cout << with_null;
```

### Output (readings)

```text
============
     | value
------------
10   | 100  
NULL | 150  
20   | 200  
NULL | 90   
10   | 50   
------------
[5 cs x 1 feat]
```

### Output (distinct(), default dropnull = true)

```text
0  10
1  20
ptype: Series
```

### Output (distinct(false))

```text
0  10
1  NULL
2  20
ptype: Series
```

With the default `dropnull = true`, the two `NULL` case labels are excluded
entirely, leaving only `10` and `20`. With `dropnull = false`, a single
`NULL` is kept — at the position of its *first* occurrence (row `1`, before
`20` first appears at row `2`) — even though it occurs twice in the source.

## Notes

- This operates on the flat case-label slate as a whole, even on a structured
  (multi-level) frame — it does not distinguish per level. Use
  [`levels()`](levels.md) for per-level distinct values.
- The result carries no display labels of its own; it prints with a plain
  default integer index.

## See Also

- [`DataFrameLabelAccessor::freq`](freq.md): the value counts behind this same
  deduplication.
- [`DataFrameLabelAccessor::levels`](levels.md): per-level distinct values for a
  structured frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
