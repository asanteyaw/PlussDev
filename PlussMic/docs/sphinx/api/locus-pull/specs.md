<!-- pluss-hand-authored -->

# LocusPull::specs

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
const std::vector<PullSpec>& LocusPull::specs() const noexcept
```

## What It Does

Exposes the plan's stored `PullSpec` list, in the order they were added, for
inspection -- it does not validate or execute anything. The reference stays
live as the plan grows, so a reference taken early reflects every spec added
afterward too, rather than a snapshot of what existed when `specs()` was
called. Every builder spelling -- `take()`, `sum()`, `add()`, scoped
[`agg()`](agg.md) calls -- ends up appending to this same list, so `specs()`
is a uniform way to inspect a plan regardless of which methods built it.

## Parameters

None.

## Returns

A reference to the stored spec vector.

## Examples

Example 1. `specs()` is empty on a freshly built plan, and reflects every
field added, including scoped `agg()` and raw `add()` calls.

```cpp
auto spec_plan = data.pull("customers");
std::cout << "empty specs=" << spec_plan.specs().size() << "\n";
spec_plan.take("id").sum("events.spend", "spent");
print_specs(spec_plan.specs());
```

### Output

```text
empty specs=0
specs=2
source=id op=take alias=<default>
source=events.spend op=sum alias=spent
```

Example 2. The reference stays live: a reference taken before more fields
are added reflects them afterward too.

```cpp
auto inspected = data.pull("customers");
inspected.take("id");
const auto& spec_view = inspected.specs();
std::cout << "before=" << spec_view.size() << "\n";
inspected.take("name");
std::cout << "after=" << spec_view.size() << "\n";
```

### Output

```text
before=1
after=2
```

Example 3. Every builder spelling -- `take()`, `add()`, and a scoped `agg()`
call -- appends to the same list.

```cpp
auto mixed_plan = data.pull("customers");
mixed_plan.take("id");
mixed_plan.add(nx::take("name", "customer"));
mixed_plan.agg("events").sum("spend", "spent").count("event_id", "events").end();
print_specs(mixed_plan.specs());
```

### Output

```text
specs=4
source=id op=take alias=<default>
source=name op=take alias=customer
source=events.spend op=sum alias=spent
source=events.event_id op=count alias=events
```

## See Also

- [`add`](add.md): where every spec in this list is actually appended.
- [`root`](root.md): the other read-only planning accessor.
