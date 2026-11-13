<!-- pluss-hand-authored -->

# NexusPullAgg::end

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull& NexusPullAgg::end() const
```

## What It Does

Closes the scoped reducer sub-builder opened by [`NexusPull::agg(table)`](agg.md)
and returns the *same* parent `NexusPull` the sub-builder was created from —
not a copy, and not a new builder. Every reducer call made through the
`NexusPullAgg` (`.count(...)`, `.sum(...)`, `.mean(...)`, and the rest) has
already accumulated its spec on that parent via [`add()`](add.md) by the
time `.end()` runs; `.end()` itself adds nothing. Its only job is handing
control back so the chain can keep using plain `NexusPull` methods —
`.take(...)`, another `.agg(...)`, or [`.collect()`](collect.md) — after an
`.agg(...)` scope.

Calling `.end()` is required to get back to those methods: `NexusPullAgg`
does not itself expose `.take(...)` or `.collect()`, only the nine scoped
reducers plus `.end()`. A chain that opens `.agg("events")` must close it
with `.end()` before it can call `.collect()` or start a second `.agg(...)`
scope.

## Parameters

None.

## Returns

`NexusPull&`
    A reference to the exact same `NexusPull` builder that
    [`.agg(table)`](agg.md) was called on — the same object, with every spec
    added through the closed `NexusPullAgg` now part of its accumulated
    list.

## Examples

Example 1. The common case: one `.agg(...)` scope, closed immediately before
`.collect()`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").sum("spend", "total_spend").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  total_spend
--------------------------------
0    | 1            15
1    | 2            14
2    | 3            NULL
--------------------------------
[3 cs x 2 feat]
```

Example 2. Two separate `.agg("events")...end()` scopes chained in the same
pull, matching the note on [`NexusPull::agg`](agg.md) that nothing stops
opening `.agg(table)` more than once for the same table — each call returns
an independent `NexusPullAgg`, and `.end()` returns to the same parent both
times.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").sum("spend", "total_spend").end()
    .agg("events").count("event_id", "event_count").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  total_spend  event_count
---------------------------------------------
0    | 1            15           2
1    | 2            14           3
2    | 3            NULL         0
---------------------------------------------
[3 cs x 3 feat]
```

Example 3. `.end()` hands back the *same* builder, so the chain can keep
requesting plain fields afterward, not just `.collect()` directly —
continuing to `.take("region_id")` after the scope closes.

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

## Notes

- `.end()` never validates anything and never touches the workspace — like
  every other builder method except `.collect()`, an unknown table or
  feature named inside the closed `.agg(...)` scope only surfaces when
  [`.collect()`](collect.md) actually executes.
- Because `.end()` returns the same `NexusPull&` object rather than a copy,
  storing its return value in a second named variable does not create an
  independent builder — both names refer to the same accumulated spec list.
- `NexusPullAgg` has no destructor-based "auto-close" behavior: forgetting
  `.end()` simply leaves the sub-builder as a live, unused temporary — it
  does not implicitly return to `NexusPull` on its own.

## See Also

- [`NexusPull::agg`](agg.md): opens the scoped sub-builder `.end()` closes.
- [`NexusPullAgg::count`](agg-count.md), [`::sum`](agg-sum.md), and the other
  seven scoped reducers that accumulate specs before `.end()` is called.
- [`NexusPull::collect`](collect.md): typically the next call after the
  final `.end()` in a chain.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
