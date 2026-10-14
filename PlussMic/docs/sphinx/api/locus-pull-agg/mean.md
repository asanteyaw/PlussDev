<!-- pluss-hand-authored -->

# LocusPullAgg::mean

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg::mean(std::string_view feature, std::string_view alias = {}) -> LocusPullAgg&
LocusPullAgg::mean(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
LocusPullAgg::mean(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
```

## What It Does

The scoped spelling of [`LocusPull::mean`](../locus-pull/mean.md):
`scope.mean("spend", "average")` inside an `agg("events")` scope is exactly
`pull.mean("events.spend", "average")`. Requires a numeric feature; see
[`sum`](sum.md) for the shared null-for-empty and `NaN`-exclusion rules.

## Parameters

| Parameter | Meaning |
|---|---|
| `feature` | A numeric field on the scope's table (not root-qualified). |
| `alias` | The output column name. Empty derives one from the qualified source. |
| `fields` | Several `(feature, alias)` pairs, added in order. |

## Returns

A reference to the same scope, so calls chain in place.

## Errors

Throws at [`collect()`](../locus-pull/collect.md): an empty `feature`, an
unknown feature on the scope's table, or a non-numeric feature.

## Examples

Example 1. The scalar form.

```cpp
auto scalar_plan = data.pull("customers");
scalar_plan.agg("events").mean("spend", "average").end();
print_numbers(scalar_plan.collect(), "average");
```

### Output

```text
average=[5, 7.33333333333, 4, null]
```

Example 2. The batch form.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events").mean({{"spend", "average"}}).end();
print_numbers(batch_plan.collect(), "average");
```

### Output

```text
average=[5, 7.33333333333, 4, null]
```

## See Also

- [`LocusPull::mean`](../locus-pull/mean.md): the unscoped spelling.
- [`median`](median.md): the order-statistic center, scoped the same way.
