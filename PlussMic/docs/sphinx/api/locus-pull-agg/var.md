<!-- pluss-hand-authored -->

# LocusPullAgg::var

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg::var(std::string_view feature, std::string_view alias = {}) -> LocusPullAgg&
LocusPullAgg::var(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
LocusPullAgg::var(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
```

## What It Does

The scoped spelling of [`LocusPull::var`](../locus-pull/var.md):
`scope.var("spend", "sample_var")` inside an `agg("events")` scope is
exactly `pull.var("events.spend", "sample_var")` -- [`std`](std.md) squared.
Requires a numeric feature, and is null for zero *or exactly one* related
value, the same as `std`; see [`sum`](sum.md) for the shared `NaN`-exclusion
rule.

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
scalar_plan.agg("events").var("spend", "sample_var").end();
print_numbers(scalar_plan.collect(), "sample_var");
```

### Output

```text
sample_var=[6.66666666667, 4.33333333333, null, null]
```

Example 2. The full scoped reducer family in one chain, batch form, with
`collect()`'s own column order.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events")
    .count({{"spend", "n"}})
    .ndistinct({{"spend", "distinct_spend"}})
    .sum({{"spend", "total"}})
    .mean({{"spend", "average"}})
    .median({{"spend", "middle"}})
    .min({{"spend", "low"}})
    .max({{"spend", "high"}})
    .std({{"spend", "sample_sd"}})
    .var({{"spend", "sample_var"}})
    .end();
print_keys(batch_plan.collect());
```

### Output

```text
keys=[n, distinct_spend, total, average, middle, low, high, sample_sd, sample_var] rows=4
```

Columns appear in the order the scoped reducers were chained, the same
addition-order rule [`collect()`](../locus-pull/collect.md) follows for any
plan.

## See Also

- [`LocusPull::var`](../locus-pull/var.md): the unscoped spelling.
- [`std`](std.md): the matching sample standard deviation.
