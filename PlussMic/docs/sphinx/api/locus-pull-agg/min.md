<!-- pluss-hand-authored -->

# LocusPullAgg::min

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg& LocusPullAgg::min(std::string_view feature, std::string_view alias = {})
LocusPullAgg& LocusPullAgg::min(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
LocusPullAgg& LocusPullAgg::min(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

The scoped spelling of [`LocusPull::min`](../locus-pull/min.md):
`scope.min("spend", "low")` inside an `agg("events")` scope is exactly
`pull.min("events.spend", "low")`. Requires a numeric feature; see
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
scalar_plan.agg("events").min("spend", "low").end();
print_numbers(scalar_plan.collect(), "low");
```

### Output

```text
low=[2, 5, 4, null]
```

Example 2. The batch form.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events").min({{"spend", "low"}}).end();
print_numbers(batch_plan.collect(), "low");
```

### Output

```text
low=[2, 5, 4, null]
```

## See Also

- [`LocusPull::min`](../locus-pull/min.md): the unscoped spelling.
- [`max`](max.md): the matching upper extreme, scoped the same way.
