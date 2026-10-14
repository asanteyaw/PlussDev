<!-- pluss-hand-authored -->

# LocusPull::agg

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::agg(std::string_view table) -> LocusPullAgg
```

## What It Does

Opens a [`LocusPullAgg`](../../locus_pull_agg_api_reference.md) scope bound to
one related table: every reducer called on the returned object (`.count(...)`,
`.sum(...)`, and the rest) is implicitly qualified under `table`, so
`scope.sum("spend", "total")` is exactly `pull.sum("events.spend", "total")`
without repeating `"events."` on every call. `end()` returns the same
`LocusPull` the scope was opened from, so a scoped sequence reads as one
fluent chain: `.agg("events").sum("spend", "total").count("event_id",
"n").end()`.

The scope itself is a cheap, borrowing view -- opening it and calling
reducers on it does not read a stream or allocate anything; every call still
just appends a `PullSpec` to the owning `LocusPull`'s plan, the same as
calling `.sum("events.spend", "total")` directly on the pull would. `agg()`
is purely a typing convenience for several reducers on the same table, not a
different execution path.

## Parameters

| Parameter | Meaning |
|---|---|
| `table` | The related table every reducer in the scope is qualified against. Must not be empty. |

## Returns

A new `LocusPullAgg`, borrowing this `LocusPull` by reference.

## Errors

Throws immediately if `table` is empty. An unknown table name is not checked
here -- it surfaces at [`collect()`](collect.md), the same as every other
field reference.

## Examples

Example 1. A scoped batch of reducers, mixed with a plain `take()` on the
owning pull, and `end()` returning the exact same pull.

```cpp
auto scoped_plan = data.pull("customers").take("id");
auto event_scope = scoped_plan.agg("events");
auto* same_scope = &event_scope.count("event_id", "events");
event_scope.sum({{"spend", "spent"}, {"reading", "reading_total"}});
auto* same_owner = &event_scope.end();
std::cout << "same_scope=" << std::boolalpha << (same_scope == &event_scope)
          << " same_owner=" << (same_owner == &scoped_plan)
          << " specs=" << scoped_plan.specs().size() << "\n";
const auto scoped_result = scoped_plan.collect();
print_keys(scoped_result);
```

### Output

```text
same_scope=true same_owner=true specs=4
keys=[id, events, spent, reading_total] rows=4
```

`event_scope.count(...)` returns the same `LocusPullAgg`; `event_scope.end()`
returns the exact `scoped_plan` it was opened from, not a copy -- the four
specs (`id`, `events`, `spent`, `reading_total`) all live on one plan.

Example 2. `agg("events").sum("spend", "value")` and the plain
`.sum("events.spend", "value")` spelling are the same operation -- confirmed
by comparing a scoped and an unscoped reducer sequence producing identical
results (see [the reducer pages](sum.md) for the full equivalence, run
side by side).

```cpp
print_error("empty scope", [&] { (void)data.pull("customers").agg(""); });
```

### Output

```text
empty scope: LocusPull::agg: table scope cannot be empty
```

## See Also

- [`LocusPullAgg` API Reference](../../locus_pull_agg_api_reference.md): every
  reducer available inside a scope, and `end()`.
- [`sum`](sum.md) and the other reducers: the unscoped spelling `agg()`'s
  scoped calls are equivalent to.
