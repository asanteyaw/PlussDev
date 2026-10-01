<!-- pluss-hand-authored -->

# NexusPull::specs

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::specs() const noexcept -> const std::vector<nexus::PullSpec>&
```

## What It Does

Exposes the builder's accumulated request list — every
[`PullSpec`](../nexus/pull.md) that `.take(...)`, `.sum(...)`,
`.agg(...)`, and [`.add(...)`](add.md) have appended so far, in the exact
order they were added. This is a read-only window into the same list
[`.collect()`](collect.md) will execute; nothing here is copied or
recomputed.

## Parameters

None.

## Returns

`const std::vector<nexus::PullSpec>&`
    The accumulated specs, in call order. Empty for a freshly constructed
    builder that hasn't had any field requested yet.

## Examples

Example 1. Inspect what a chain of calls actually built, before collecting.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

// nx: the customers/regions/events workspace from Nexus::pull's own page.
auto builder = nx.pull("customers")
    .take("customer_id")
    .sum("events.spend", "total_spend");

std::cout << builder.specs().size() << " specs\n";
for (const auto& spec : builder.specs()) {
  std::cout << "  source=" << spec.source
            << " op=" << static_cast<int>(spec.op)
            << " alias=" << (spec.alias.has_value() ? *spec.alias : std::string("<none>"))
            << "\n";
}
```

### Output

```text
2 specs
  source=customer_id op=0 alias=<none>
  source=events.spend op=3 alias=total_spend
```

`op=0` is `PullOp::Take`; `op=3` is `PullOp::Sum` — see
[`nexus::PullOp`](../nexus/pull.md) for the full enumerator list.

## Notes

- Every fluent method funnels through [`add()`](add.md), so `.specs()`
  after any combination of `.take(...)`/`.sum(...)`/`.agg(table)...end()`
  calls shows exactly what those calls constructed — there is no separate
  internal representation to fall out of sync with.
- `.specs()` reflects `.add(...)` calls too, including hand-built specs with
  `.windowed(...)`/`.root_windowed(...)` attached.

## See Also

- [`NexusPull::add`](add.md): the method every other builder call
  ultimately appends a spec through.
- [`NexusPull::root`](root.md): the complementary piece of builder state —
  which table these specs will be pulled against.
- [`NexusPull::collect`](collect.md): execute exactly this list.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="root.html">
    <span class="label">Previous</span>
    <span class="title">root</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
