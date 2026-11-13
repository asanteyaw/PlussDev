<!-- pluss-hand-authored -->

# LocusPullAgg::sum

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg& LocusPullAgg::sum(std::string_view feature, std::string_view alias = {})
LocusPullAgg& LocusPullAgg::sum(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
LocusPullAgg& LocusPullAgg::sum(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

The scoped spelling of [`LocusPull::sum`](../locus-pull/sum.md):
`scope.sum("spend", "total")` inside an `agg("events")` scope is exactly
`pull.sum("events.spend", "total")`. Requires a numeric feature; see that
page for the shared null-for-empty and `NaN`-exclusion rules every numeric
reducer follows.

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

Example 1. The scalar form, alongside a scoped `count`.

```cpp
auto scalar_plan = data.pull("customers");
scalar_plan.agg("events").count("spend", "n").sum("spend", "total").end();
const auto scalar_result = scalar_plan.collect();
print_numbers(scalar_result, "n");
print_numbers(scalar_result, "total");
```

### Output

```text
n=[4, 3, 1, 0]
total=[20, 22, 4, null]
```

Example 2. The batch form, and an empty batch as a no-op -- `sum` with a
bare `std::initializer_list<std::pair<...>>{}` adds nothing to the plan.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events").sum({{"spend", "total"}}).end();
print_numbers(batch_plan.collect(), "total");

auto empty_batch = data.pull("customers").take("id");
auto empty_scope = empty_batch.agg("events");
empty_scope.sum(std::initializer_list<std::pair<std::string_view, std::string_view>>{});
std::cout << "specs=" << empty_scope.end().specs().size() << "\n";
```

### Output

```text
total=[20, 22, 4, null]
specs=1
```

`specs=1` counts only the earlier `take("id")` -- the empty `sum(...)` call
appended nothing.

## See Also

- [`LocusPull::sum`](../locus-pull/sum.md): the unscoped spelling, and the
  shared null-for-empty and `NaN`-exclusion rules.
- [`mean`](mean.md), [`median`](median.md), [`min`](min.md), [`max`](max.md),
  [`std`](std.md), [`var`](var.md): the other scoped numeric reducers.
