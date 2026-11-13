<!-- pluss-hand-authored -->

# NexusPull::agg

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg NexusPull::agg(std::string_view table)
```

## What It Does

Opens a scoped reducer sub-builder for one related table, so several
aggregates on that table can be written without repeating its name in every
call. `.agg("events")` followed by `.count(...)`, `.sum(...)`, and so on
qualifies each feature as `"events.<feature>"` automatically — exactly the
same qualification [`NexusPullAgg::count`](agg-count.md) and its siblings
document — and [`.end()`](end.md) hands control back to the parent
`NexusPull` so the chain continues normally.

`.agg(table)` itself does not add anything to the pull request; it is purely
a scoping device. The reducer calls made through the returned
`NexusPullAgg` are what actually accumulate specs, via the same
[`add()`](add.md) every other builder method funnels through.

## Parameters

**table** (`std::string_view`)
    The related table every reducer call through the returned `NexusPullAgg`
    will be scoped to. Must be non-empty — an empty table name throws
    immediately, before any reducer is called.

## Returns

`NexusPullAgg`
    A new sub-builder bound to this `NexusPull` and scoped to `table`.

## Examples

Example 1. Several reducers on one related table, scoped once instead of
repeated per call.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .count("event_id", "event_count")
        .sum("spend", "total_spend")
        .mean("spend", "mean_spend")
    .end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  event_count  total_spend  mean_spend
----------------------------------------------------------
0    | 1            2            15           7.5
1    | 2            3            14           4.666666667
2    | 3            0            NULL         NULL
----------------------------------------------------------
[3 cs x 4 feat]
```

Example 2. Continuing from Example 1: `.end()` returns the *same* parent
builder, so the chain can keep going — take more root fields, open another
`.agg(...)` scope, or `.collect()` directly.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").sum("spend", "total_spend").end()
    .take("region_id")
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id total_spend region_id
```

Example 3. An empty table name throws immediately.

```cpp
try {
  auto bad = nx.pull("customers").agg("");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
NexusPull::agg: table scope cannot be empty
```

## Notes

- `table` does not have to be reachable via a declared relationship at the
  point `.agg(table)` is called — that is only checked when
  [`.collect()`](collect.md) executes, matching every other pull field.
- Nothing stops opening `.agg(table)` more than once for the same table in
  one chain; each call returns an independent `NexusPullAgg`, and both sets
  of reducers accumulate normally.
- If you only need *one* reducer on a related table, the direct
  [`NexusPull::sum`](sum.md)-style methods (`.sum("events.spend", "total_spend")`)
  are shorter than opening and closing an `.agg(...)` scope for a single call.

## See Also

- [`NexusPullAgg::end`](end.md): returns from the scoped sub-builder back to
  this `NexusPull`.
- [`NexusPullAgg::count`](agg-count.md), [`::sum`](agg-sum.md), and the other
  seven scoped reducers.
- [`NexusPull::sum`](sum.md) and its siblings: the unscoped equivalent, for a
  single reducer call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="agg-count.html">
    <span class="label">Next</span>
    <span class="title">agg-count</span>
  </a>
</div>
