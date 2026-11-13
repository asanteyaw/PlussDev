<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab9de14fc413 -->

# Slate::cumprod

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::cumprod(bool skip_nulls = true) const
```

## What It Does

Returns the running (cumulative) product down the slate — a thin wrapper for
`scan(ScanStatistic::Product, skip_nulls)`. See [`Slate::scan`](scan.md) for
the full mechanics shared by all four running-statistic methods, including
exactly how missing entries are handled under `skip_nulls`, and why the throw
message for a non-numeric slate reads `"Slate::scan"` rather than
`"Slate::cumprod"`.

## Parameters

**skip_nulls** (`bool`, default `true`)
    `true`: a missing input position stays missing in the output, but later
    positions' running products are unaffected. `false`: a missing input
    position makes every subsequent output position missing too. See
    [`Slate::scan`](scan.md) for the full explanation.

## Returns

`Slate`
    A slate the same length as the source, holding the running product at
    each position.

## Examples

Example 1. Running product, no missing entries.

```cpp
using namespace pluss::table;

auto factors = Slate(pluss::make_series({2.0, 3.0, 4.0}));
auto out = factors.cumprod();
std::cout << out;
```

### Output

```text
0  2
1  6
2  24

ptype: Series
```

Example 2. `skip_nulls = true` (the default) — the missing position stays
missing, but the product continues correctly underneath it.

```cpp
auto readings = Slate(
    pluss::make_series({2.0, 100.0, 3.0, 4.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = readings.cumprod();
std::cout << out2;
```

### Output

```text
0  2
1  NULL
2  6
3  24

ptype: Series
```

The missing `100.0` never enters the running product at all — position `2`'s
value is `2 * 3 = 6`, not `2 * 100 * 3`.

Example 3. `skip_nulls = false` — the missing position poisons every position
after it.

```cpp
auto out3 = readings.cumprod(false);
std::cout << out3;
```

### Output

```text
0  2
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

- [`Slate::prod`](prod.md): the single final product, rather than a running
  value at every position.
- [`Slate::scan`](scan.md): the general method this wraps.
- [`Slate::cumsum`](cumsum.md), [`Slate::cummin`](cummin.md),
  [`Slate::cummax`](cummax.md): the other three running statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummin.html">
    <span class="label">Previous</span>
    <span class="title">cummin</span>
  </a>
  <a class="next-link" href="cumsum.html">
    <span class="label">Next</span>
    <span class="title">cumsum</span>
  </a>
</div>
