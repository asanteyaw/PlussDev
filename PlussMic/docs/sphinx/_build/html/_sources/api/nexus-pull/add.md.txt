<!-- pluss-hand-authored -->

# NexusPull::add

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::add(const nexus::PullSpec& spec) -> NexusPull&
NexusPull::add(std::initializer_list<nexus::PullSpec> specs) -> NexusPull&
```

## What It Does

Appends one or more already-built `PullSpec` values to the builder's
accumulated request list, verbatim — no parsing, no qualification, no
implicit aliasing beyond whatever the spec already carries.

Every other builder method (`.take(...)`, `.sum(...)`, `.agg(table).count(...)`,
and so on) is sugar over this: each one builds a `PullSpec` internally (via
the free [`nexus::take`/`nexus::sum`/etc. factories](take.md)) and calls
`add()` with it. `add()` is the one method that lets you hand the builder a
spec you built yourself, which is the *only* way to reach
[`PullSpec::windowed(...)`/`::root_windowed(...)`](../nexus/pull.md) — those
live on `PullSpec`, not on `NexusPull`, so a windowed aggregate has to be
constructed as a spec first and added, rather than expressed as a plain
`.sum(...)` call.

## Parameters

**spec** (`const nexus::PullSpec&`)
    A single pre-built spec to append.

**specs** (`std::initializer_list<nexus::PullSpec>`)
    Several pre-built specs to append in one call, in list order.

Neither overload validates `spec`/`specs` against the workspace at this
point — an unknown source or a malformed table path only surfaces later,
when [`.collect()`](collect.md) actually executes the pull.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single spec, built via the free `nexus::take` factory.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;
using namespace pluss::bundle::nexus;

// nx: a workspace with a "customers" root table and an "events" table
// related OneToMany via customer_id, matching Nexus::pull's own example.
auto pulled = nx.pull("customers")
    .add(take("customer_id"))
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id
```

Example 2. A batch of specs in one call, mixing a plain field with an
aggregate.

```cpp
auto pulled = nx.pull("customers")
    .add(take("customer_id"))
    .add({take("region_id"), sum("events.spend", "total_spend")})
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  region_id  total_spend
------------------------------------------
0    | 1            10         15
1    | 2            20         14
2    | 3            10         NULL
------------------------------------------
[3 cs x 3 feat]
```

Customer 3 has no rows in `events` at all, so its `total_spend` aggregate is
`NULL` rather than `0` — an aggregate over an empty group produces a missing
value, not a zero.

Example 3. The one thing `add()` unlocks that no fluent shorthand does:
attaching a windowed aggregate spec.

```cpp
using pluss::bundle::nexus::sum;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .add({sum("events.spend", "recent_spend").windowed(30, "d")})
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id recent_spend
```

## Notes

- `add()` never deduplicates or reorders — specs execute and materialize
  columns in the order they were added, across every method that ultimately
  calls it.
- Because every fluent method is built on `add()`, [`.specs()`](specs.md)
  after a chain of `.take(...)`/`.sum(...)`/`.agg(...)` calls shows exactly
  the `PullSpec` values those calls constructed — there is no separate
  internal representation.

## See Also

- [`NexusPull::take`](take.md): the common case (`add(nexus::take(...))`)
  spelled as one call instead of two.
- [`NexusPull::agg`](agg.md): scopes several reducer specs under one related
  table without repeating its name.
- [`NexusPull::specs`](specs.md): inspect everything accumulated via `add()`
  (directly or through another builder method) before collecting.
- [`Nexus::pull`](../nexus/pull.md): the full field-expression grammar and
  windowed-aggregate semantics `PullSpec` implements.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../nexus_pull_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">NexusPull API Reference</span>
  </a>
  <a class="next-link" href="agg.html">
    <span class="label">Next</span>
    <span class="title">agg</span>
  </a>
</div>
