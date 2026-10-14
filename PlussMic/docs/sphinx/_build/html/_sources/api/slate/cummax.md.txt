<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 03b18ed7b048 -->

# Slate::cummax

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::cummax(bool skip_nulls = true) const -> Slate
```

## What It Does

Returns the running (cumulative) maximum down the slate — a thin wrapper for
`scan(ScanStatistic::Maximum, skip_nulls)`. See [`Slate::scan`](scan.md) for
the full mechanics shared by all four running-statistic methods, including
exactly how missing entries are handled under `skip_nulls`, and why the throw
message for a non-numeric slate reads `"Slate::scan"` rather than
`"Slate::cummax"`.

## Parameters

**skip_nulls** (`bool`, default `true`)
    `true`: a missing input position stays missing in the output, but later
    positions' running maximums are unaffected. `false`: a missing input
    position makes every subsequent output position missing too. See
    [`Slate::scan`](scan.md) for the full explanation.

## Returns

`Slate`
    A slate the same length as the source, holding the running maximum at
    each position.

## Examples

Example 1. Running maximum, no missing entries.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 30.0, 20.0, 50.0}));
auto out = scores.cummax();
std::cout << out;
```

### Output

```text
0  10
1  30
2  30
3  50

ptype: Series
```

Example 2. `skip_nulls = true` (the default) — the missing position stays
missing, but the running maximum continues correctly underneath it.

```cpp
auto readings = Slate(
    pluss::make_series({10.0, 100.0, 30.0, 20.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = readings.cummax();
std::cout << out2;
```

### Output

```text
0  10
1  NULL
2  30
3  30

ptype: Series
```

The missing `100.0` — which would otherwise pull the running maximum up to
`100` — never enters the computation, so position `2`'s running maximum stays
`30`, not `100`.

Example 3. `skip_nulls = false` — the missing position poisons every position
after it.

```cpp
auto out3 = readings.cummax(false);
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

- See [`Slate::scan`](scan.md) for the full shared mechanics, including the
  identity-substitution behavior underneath `skip_nulls` and the
  `"Slate::scan"`-branded throw message.
- Restricted to numeric `Series` payloads.

## See Also

- [`Slate::max`](max.md): the single final maximum, rather than a running
  value at every position.
- [`Slate::scan`](scan.md): the general method this wraps.
- [`Slate::cumsum`](cumsum.md), [`Slate::cumprod`](cumprod.md),
  [`Slate::cummin`](cummin.md): the other three running statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="csmin.html">
    <span class="label">Previous</span>
    <span class="title">csmin</span>
  </a>
  <a class="next-link" href="cummin.html">
    <span class="label">Next</span>
    <span class="title">cummin</span>
  </a>
</div>
