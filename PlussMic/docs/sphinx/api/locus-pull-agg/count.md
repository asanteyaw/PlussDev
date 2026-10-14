<!-- pluss-hand-authored -->

# LocusPullAgg::count

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPullAgg::count(std::string_view feature, std::string_view alias = {}) -> LocusPullAgg&
LocusPullAgg::count(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
LocusPullAgg::count(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> LocusPullAgg&
```

## What It Does

The scoped spelling of [`LocusPull::count`](../locus-pull/count.md):
`scope.count("spend", "n")` inside an `agg("events")` scope is exactly
`pull.count("events.spend", "n")` -- `feature` is relative to the scope's
table, not root. Like its unscoped counterpart, `count` works on any stream
kind, not just numeric ones. The batch overload takes several
`(feature, alias)` pairs at once, each qualified the same way.

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

Example 1. The scalar form, as one reducer in a scoped chain.

```cpp
auto scalar_plan = data.pull("customers");
scalar_plan.agg("events").count("spend", "n").end();
print_numbers(scalar_plan.collect(), "n");
```

### Output

```text
n=[4, 3, 1, 0]
```

Example 2. The batch form, one pair at a time -- and empty, as a no-op.

```cpp
auto batch_plan = data.pull("customers");
batch_plan.agg("events").count({{"spend", "n"}}).end();
print_numbers(batch_plan.collect(), "n");

auto empty_batch = data.pull("customers").take("id");
auto empty_scope = empty_batch.agg("events");
empty_scope.sum(std::initializer_list<std::pair<std::string_view, std::string_view>>{});
std::cout << "specs=" << empty_scope.end().specs().size() << "\n";
```

### Output

```text
n=[4, 3, 1, 0]
specs=1
```

An empty batch on any of these reducers adds nothing -- `specs=1` here
counts only the earlier `take("id")`, not a spec from the empty `sum(...)`
call.

## See Also

- [`LocusPull::count`](../locus-pull/count.md): the unscoped spelling, and
  the full stream-kind-agnostic behavior.
- [`ndistinct`](ndistinct.md): the other stream-kind-agnostic scoped reducer.
