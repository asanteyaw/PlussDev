<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6909fbc3127f -->

# Shelf::cumsum

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::cumsum(bool skip_nulls = true) const
```

## What It Does

Returns the running (cumulative) sum down the shelf — a thin wrapper for
`scan(ScanStatistic::Sum, skip_nulls)`. See [`Shelf::scan`](scan.md) for the
full mechanics shared by all four running-statistic methods, including exactly
how missing entries are handled under `skip_nulls`, and why the throw message
for a non-numeric shelf reads `"Shelf::scan"` rather than `"Shelf::cumsum"`.

## Parameters

**skip_nulls** (`bool`, default `true`)

`true`: a missing input position stays missing in the output, but later
positions' running sums are unaffected. `false`: a missing input position
makes every subsequent output position missing too. See
[`Shelf::scan`](scan.md) for the full explanation.

## Returns

`Shelf`
    A shelf the same length as the source, holding the running sum at each
    position.

## Examples

Example 1. Running sum, no missing entries.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = values.cumsum();
std::cout << out;
```

### Output

```text
0  10
1  30
2  60
3  100

ptype: Series
```

Example 2. `skip_nulls = true` (the default) — the missing position stays
missing, but the sum continues correctly underneath it.

```cpp
auto readings = Shelf(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = readings.cumsum();
std::cout << out2;
```

### Output

```text
0  10
1  NULL
2  40
3  80

ptype: Series
```

Example 3. `skip_nulls = false` — the missing position poisons every position
after it.

```cpp
auto out3 = readings.cumsum(false);
std::cout << out3;
```

### Output

```text
0  10
1  NULL
2  NULL
3  NULL

ptype: Series
```

## Notes

- See [`Shelf::scan`](scan.md) for the full shared mechanics, including the
  identity-substitution behavior underneath `skip_nulls` and the
  `"Shelf::scan"`-branded throw message.
- Restricted to numeric `Series` payloads.

## See Also

- [`Shelf::sum`](sum.md): the single final total, rather than a running value
  at every position.
- [`Shelf::scan`](scan.md): the general method this wraps.
- [`Shelf::cumprod`](cumprod.md), [`Shelf::cummin`](cummin.md),
  [`Shelf::cummax`](cummax.md): the other three running statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumprod.html">
    <span class="label">Previous</span>
    <span class="title">cumprod</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
