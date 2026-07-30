<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 598483535f18 -->

# Thread::cummin

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::cummin(bool skip_nulls = true) const -> Thread
```

## What It Does

Returns the running (cumulative) minimum down the thread — a thin wrapper for
`scan(ScanStatistic::Minimum, skip_nulls)`. See [`Thread::scan`](scan.md) for
the full mechanics shared by all four running-statistic methods, including
exactly how missing entries are handled under `skip_nulls`, and why the throw
message for a non-numeric thread reads `"Thread::scan"` rather than
`"Thread::cummin"`.

## Parameters

**skip_nulls** (`bool`, default `true`)
    `true`: a missing input position stays missing in the output, but later
    positions' running minimums are unaffected. `false`: a missing input
    position makes every subsequent output position missing too. See
    [`Thread::scan`](scan.md) for the full explanation.

## Returns

`Thread`
    A thread the same length as the source, holding the running minimum at
    each position.

## Examples

Example 1. Running minimum, no missing entries.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({30.0, 10.0, 50.0, 5.0}));
auto out = scores.cummin();
std::cout << out;
```

### Output

```text
0  30
1  10
2  10
3  5

ptype: Series
```

Example 2. `skip_nulls = true` (the default) — the missing position stays
missing, but the running minimum continues correctly underneath it.

```cpp
auto readings = Thread(
    pluss::make_series({30.0, 1.0, 10.0, 50.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = readings.cummin();
std::cout << out2;
```

### Output

```text
0  30
1  NULL
2  10
3  10

ptype: Series
```

The missing `1.0` — which would otherwise pull the running minimum down to `1`
— never enters the computation, so position `2`'s running minimum stays `10`,
not `1`.

Example 3. `skip_nulls = false` — the missing position poisons every position
after it.

```cpp
auto out3 = readings.cummin(false);
std::cout << out3;
```

### Output

```text
0  30
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

- [`Thread::min`](min.md): the single final minimum, rather than a running
  value at every position.
- [`Thread::scan`](scan.md): the general method this wraps.
- [`Thread::cumsum`](cumsum.md), [`Thread::cumprod`](cumprod.md),
  [`Thread::cummax`](cummax.md): the other three running statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummax.html">
    <span class="label">Previous</span>
    <span class="title">cummax</span>
  </a>
  <a class="next-link" href="cumprod.html">
    <span class="label">Next</span>
    <span class="title">cumprod</span>
  </a>
</div>
