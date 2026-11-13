<!-- pluss-hand-authored -->

# LocusPullAgg::ndistinct

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg& LocusPullAgg::ndistinct(std::string_view feature, std::string_view alias = {})
LocusPullAgg& LocusPullAgg::ndistinct(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
LocusPullAgg& LocusPullAgg::ndistinct(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

The scoped spelling of [`LocusPull::ndistinct`](../locus-pull/ndistinct.md):
`scope.ndistinct("spend", "distinct_spend")` inside an `agg("events")` scope
is exactly `pull.ndistinct("events.spend", "distinct_spend")`. Works on any
stream kind, not just numeric ones.

## Parameters

| Parameter | Meaning |
|---|---|
| `feature` | A field on the scope's table (not root-qualified). |
| `alias` | The output column name. Empty derives one from the qualified source. |
| `fields` | Several `(feature, alias)` pairs, added in order. |

## Returns

A reference to the same scope, so calls chain in place.

## Errors

Throws at [`collect()`](../locus-pull/collect.md): an empty `feature`, or an
unknown feature on the scope's table.

## Examples

Example 1. The scalar form.

```cpp
auto scalar_plan = data.pull("customers");
scalar_plan.agg("events").ndistinct("spend", "distinct_spend").end();
print_numbers(scalar_plan.collect(), "distinct_spend");
```

### Output

```text
distinct_spend=[4, 3, 1, 0]
```

Example 2. The batch form, one pair at a time.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events").ndistinct({{"spend", "distinct_spend"}}).end();
print_numbers(batch_plan.collect(), "distinct_spend");
```

### Output

```text
distinct_spend=[4, 3, 1, 0]
```

## See Also

- [`LocusPull::ndistinct`](../locus-pull/ndistinct.md): the unscoped
  spelling.
- [`count`](count.md): the other stream-kind-agnostic scoped reducer.
