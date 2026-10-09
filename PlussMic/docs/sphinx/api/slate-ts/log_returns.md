<!-- pluss-hand-authored -->

# TimeSeriesAccessor::log_returns

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::log_returns(std::int64_t periods = 1) const -> Slate
```

## What It Does

Computes the logarithmic period-over-period change of the bound slate:
`out[i] = ln(x[i] / base[i])`, where `base` is `x` shifted by `periods` (a lag when
`periods > 0`, a lead when `periods < 0`). A position is `NULL` wherever `x[i]` or
`base[i]` is missing or non-finite, **or either value is not strictly positive** —
unlike [`returns()`](returns.md), a zero or negative value anywhere in the
comparison always yields `NULL`, since the logarithm would otherwise be undefined.
Restricted to a numeric `Series` slate; anything else throws.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts compare
    each position against an earlier one (a lag); negative shifts compare against
    a later one (a lead). Must not be `0`, or the call throws.

## Returns

`Slate`
    A new slate, same length as the bound source, holding the period-over-period
    log return.

## Examples

Example 1. Compute the one-period log return of a numeric slate.

```cpp
using namespace pluss::table;

auto load = Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = load.ts().log_returns(1);
std::cout << out;
```

### Output

```text
0  NULL
1  0.6931471806
2  -0.2876820725
3  0.6931471806

ptype: Series
```

Position `0` has no prior position, so it's `NULL`. Position `1` is
`ln(20 / 10) = ln(2) ≈ 0.6931`; position `2` is `ln(15 / 20) = ln(0.75) ≈ -0.2877`;
position `3` is `ln(30 / 15) = ln(2) ≈ 0.6931` — the same log return as position
`1`, since both moves double the value, whereas [`returns()`](returns.md) would
report the same `1.0` fractional return for both but a different one for the
halving case.

Example 2. A negative value in the comparison — [`returns()`](returns.md) still
returns a number, but `log_returns()` is `NULL`.

```cpp
auto signed_readings = Slate(pluss::make_series({10.0, -5.0, 20.0}));
auto pct = signed_readings.ts().returns(1);
std::cout << pct;

auto log_pct = signed_readings.ts().log_returns(1);
std::cout << log_pct;
```

### Output

```text
0  NULL
1  -1.5
2  -5

ptype: Series
0  NULL
1  NULL
2  NULL

ptype: Series
```

`returns()` only requires the base to be nonzero — position `1` (`-5` vs. base
`10`) is `-5/10 - 1 = -1.5`, and position `2` (`20` vs. base `-5`) is
`20/-5 - 1 = -5`, both perfectly well-defined ratios. `log_returns()` instead
requires **both** sides to be strictly positive; since every comparison here
involves the negative `-5`, all three positions come back `NULL`.

Example 3. A pre-existing missing entry in the source slate propagates into
every log return that touches it.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = with_gap.ts().log_returns(1);
std::cout << out2;
```

### Output

```text
0  NULL
1  NULL
2  NULL
3  0.2876820725

ptype: Series
```

Position `1` is `NULL` because `x[1]` (`20.0`) itself is masked-missing. Position
`2` is `NULL` because its `base` (`x[1]`) is missing, even though `x[2]` (`30.0`)
is a real, valid value. Only position `3` (`ln(40 / 30) ≈ 0.2877`) has both sides
valid.

Example 4. Pass `periods = 0`, to see the call throw.

```cpp
try {
  auto out3 = load.ts().log_returns(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
subsp::stats::ts::log_returns: periods cannot be zero
```

Comparing each position against itself isn't a meaningful log return, so
`periods = 0` is rejected outright rather than silently returning all-zero
output.

## Notes

- Unlike [`returns()`](returns.md), `log_returns()` requires **both** `x[i]` and
  `base[i]` to be strictly greater than `0`; a zero or negative value in either
  produces `NULL` rather than an error.
- Log returns are additive across periods (the sum of consecutive one-period log
  returns equals the multi-period log return), which plain fractional
  [`returns()`](returns.md) are not — a common reason to prefer this method for
  compounding analysis.
- A pre-existing missing entry in the source slate is honored — it produces
  `NULL` in every log return that would otherwise reference it. See Example 3.
- `periods` must not be `0`, or it throws
  `"subsp::stats::ts::log_returns: periods cannot be zero"`.
- Restricted to numeric `Series` payloads; a `Note`/`Timeline`/`TimeIndex` slate
  throws `"TimeSeriesAccessor::log_returns: source must be Series"`; a
  non-numeric `Series` throws
  `"TimeSeriesAccessor::log_returns: source Series must be numeric"`.

## See Also

- [`TimeSeriesAccessor::returns`](returns.md): the fractional (non-logarithmic)
  counterpart, which tolerates zero or negative values more permissively.
- [`TimeSeriesAccessor::diff`](diff.md): the absolute period-over-period change.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lead.html">
    <span class="label">Previous</span>
    <span class="title">lead</span>
  </a>
  <a class="next-link" href="lt.html">
    <span class="label">Next</span>
    <span class="title">lt</span>
  </a>
</div>
