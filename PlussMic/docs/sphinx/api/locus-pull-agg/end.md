<!-- pluss-hand-authored -->

# LocusPullAgg::end

[Back to LocusPullAgg API Reference](../../locus_pull_agg_api_reference)

## Signature

```cpp
LocusPull& LocusPullAgg::end() const
```

## What It Does

Returns the exact `LocusPull` the scope was opened from -- not a copy, the
same object [`LocusPull::agg(table)`](../locus-pull/agg.md) borrowed. `end()`
doesn't execute anything or change what the scope already appended; it's
purely how a scoped chain hands control back to the owning pull, so further
calls (more `take()`s, another `agg()` scope, or `collect()`) read naturally
after it: `.agg("events").sum("spend", "total").end().take("name")`.

## Parameters

None.

## Returns

A reference to the owning `LocusPull`.

## Examples

Example 1. `end()` returns the identical owning builder, confirmed by
address, and the scope's specs land on that same builder's plan.

```cpp
auto scoped_plan = data.pull("customers").take("id");
auto event_scope = scoped_plan.agg("events");
event_scope.count("event_id", "events");
event_scope.sum({{"spend", "spent"}, {"reading", "reading_total"}});
auto* same_owner = &event_scope.end();
std::cout << "same_owner=" << std::boolalpha << (same_owner == &scoped_plan)
          << " specs=" << scoped_plan.specs().size() << "\n";
```

### Output

```text
same_owner=true specs=4
```

`same_owner` confirms `end()` is the identical `scoped_plan`, not a copy --
all four specs (`id` plus three scoped reducers) live on the one builder.

## See Also

- [`LocusPull::agg`](../locus-pull/agg.md): where a scope is opened.
- [`LocusPull::collect`](../locus-pull/collect.md): the usual next call after
  `end()`.
