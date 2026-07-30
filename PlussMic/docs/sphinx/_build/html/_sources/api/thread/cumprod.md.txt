<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6982a2dcca7d -->

# Thread::cumprod

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::cumprod(bool skip_nulls = true) const -> Thread
```

## What It Does

Returns the running (cumulative) product down the thread — a thin wrapper for
`scan(ScanStatistic::Product, skip_nulls)`. See [`Thread::scan`](scan.md) for
the full mechanics shared by all four running-statistic methods, including
exactly how missing entries are handled under `skip_nulls`, and why the throw
message for a non-numeric thread reads `"Thread::scan"` rather than
`"Thread::cumprod"`.

## Parameters

**skip_nulls** (`bool`, default `true`)
    `true`: a missing input position stays missing in the output, but later
    positions' running products are unaffected. `false`: a missing input
    position makes every subsequent output position missing too. See
    [`Thread::scan`](scan.md) for the full explanation.

## Returns

`Thread`
    A thread the same length as the source, holding the running product at
    each position.

## Examples

Example 1. Running product, no missing entries.

```cpp
using namespace pluss::table;

auto factors = Thread(pluss::make_series({2.0, 3.0, 4.0}));
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
auto readings = Thread(
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

- See [`Thread::scan`](scan.md) for the full shared mechanics, including the
  identity-substitution behavior underneath `skip_nulls` and the
  `"Thread::scan"`-branded throw message.
- Restricted to numeric `Series` payloads.

## See Also

- [`Thread::prod`](prod.md): the single final product, rather than a running
  value at every position.
- [`Thread::scan`](scan.md): the general method this wraps.
- [`Thread::cumsum`](cumsum.md), [`Thread::cummin`](cummin.md),
  [`Thread::cummax`](cummax.md): the other three running statistics.

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
