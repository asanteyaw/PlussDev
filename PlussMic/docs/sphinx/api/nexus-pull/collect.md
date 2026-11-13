<!-- pluss-hand-authored -->

# NexusPull::collect

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
DataFrame NexusPull::collect() const
```

## What It Does

Executes the builder's accumulated [`PullSpec`](../nexus/pull.md) list
against the workspace and returns the materialized result — the only method
on `NexusPull` that actually touches the workspace. Every other builder
method (`.take(...)`, `.sum(...)`, `.agg(...)`, `.add(...)`) only appends to
an in-memory list; nothing is validated against the workspace, and no query
runs, until `.collect()` is called.

`collect()` is exactly equivalent to calling
[`Nexus::pull(root, specs)`](../nexus/pull.md) directly with the accumulated
[`.specs()`](specs.md) list — the fluent builder is a more ergonomic way to
assemble that list, not a different execution path.

## Parameters

None.

## Returns

`DataFrame`
    One row per row of [`.root()`](root.md), one column per accumulated
    spec, titled `<root> + "_pull"`. An empty spec list still throws — a
    pull needs at least one requested field.

## Examples

Example 1. Build a request across two calls, then collect once.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

// nx: the customers/regions/events workspace from Nexus::pull's own page.
auto pulled = nx.pull("customers")
    .take("customer_id")
    .sum("events.spend", "total_spend")
    .collect();

std::cout << pulled.ncases() << " rows, " << pulled.features().size() << " cols";
```

### Output

```text
3 rows, 2 cols
```

Example 2. `.collect()` can be called more than once on the same builder —
it re-executes the same spec list against the workspace's *current* state
each time, rather than caching the first result.

```cpp
auto builder = nx.pull("customers").take("customer_id");
auto first = builder.collect();
auto second = builder.collect();
std::cout << (first.ncases() == second.ncases() ? "same row count" : "different");
```

### Output

```text
same row count
```

## Notes

- `collect()` is `const` — calling it does not consume or clear the
  builder's spec list, so the same `NexusPull` value can be collected
  repeatedly, or extended with more `.take(...)`/`.sum(...)` calls between
  collects.
- Validation that a plain builder method skips (unknown features,
  non-root-preserving related-table paths, missing declared metadata for
  aggregates) all surfaces here, as a thrown `pluss::Error` — see
  [`Nexus::pull`](../nexus/pull.md) for the full list of what gets checked
  and when.

## See Also

- [`NexusPull::specs`](specs.md): inspect exactly what `.collect()` is about
  to execute, without executing it.
- [`Nexus::pull`](../nexus/pull.md): the lower-level entry point `.collect()`
  delegates to, and the full field-expression/validation reference.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-var.html">
    <span class="label">Previous</span>
    <span class="title">agg-var</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
