<!-- pluss-hand-authored -->

# LocusPullAgg::std

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg::std(std::string_view feature, std::string_view alias = {}) -> LocusPullAgg&
LocusPullAgg::std(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
LocusPullAgg::std(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
```

## What It Does

The scoped spelling of [`LocusPull::std`](../locus-pull/std.md):
`scope.std("spend", "sample_sd")` inside an `agg("events")` scope is exactly
`pull.std("events.spend", "sample_sd")`. Requires a numeric feature; like
its unscoped counterpart, it's null for a root record with zero *or exactly
one* related value, not just zero -- see [`sum`](sum.md) for the shared
`NaN`-exclusion rule.

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

Example 1. The scalar form -- null for both the one-event and zero-event
customer.

```cpp
auto scalar_plan = data.pull("customers");
scalar_plan.agg("events").std("spend", "sample_sd").end();
print_numbers(scalar_plan.collect(), "sample_sd");
```

### Output

```text
sample_sd=[2.58198889747, 2.08166599947, null, null]
```

Example 2. The batch form.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events").std({{"spend", "sample_sd"}}).end();
print_numbers(batch_plan.collect(), "sample_sd");
```

### Output

```text
sample_sd=[2.58198889747, 2.08166599947, null, null]
```

## See Also

- [`LocusPull::std`](../locus-pull/std.md): the unscoped spelling.
- [`var`](var.md): the matching sample variance, scoped the same way.
