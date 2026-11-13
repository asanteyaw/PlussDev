<!-- pluss-hand-authored -->

# Nexus::from

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
NexusTraverse Nexus::from(std::string_view table) const
```

## What It Does

Enters the fluent [`NexusTraverse`](../../nexus_traverse_api_reference) builder, anchored at
`table`. `from()` itself does no joining — it just binds a starting table and hands back a
builder whose methods (`.join()`, `.via()`, `.pick()`, `.drop()`, `.name()`) accumulate a
[`JoinPlan`](plan.md) one call at a time, materialized only when you call
`.collect()`.

This is the higher-level counterpart to building a [`JoinPlan`](plan.md) by hand via
[`path()`](path.md)/[`plan()`](plan.md) and executing it with
[`materialize()`](materialize.md) — `NexusTraverse` covers the common case (a chain of joins
from one anchor table) with a shorter, chainable spelling, and can auto-detect the
relationship to use at each step when it's unambiguous.

## Parameters

**table** (`std::string_view`)
    The table to anchor the query at. Must already exist in the workspace — an unknown
    name throws `"Nexus::from: unknown table '<table>'"`.

## Returns

`NexusTraverse`
    A query builder anchored at `table`, with an empty plan (zero steps) so far.

## Examples

Example 1. Enter the builder and immediately collect — with no `.join()` calls, this is
equivalent to [`frame(table)`](frame.md).

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

auto result = biz.from("customers").collect();
std::cout << result.ncases() << " " << result.nfeatures();
```

### Output

```text
3 2
```

Example 2. An unknown table throws immediately, before any joining is attempted.

```cpp
try {
  auto bad = biz.from("missing");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::from: unknown table 'missing'
```

## Notes

- Never touches table data itself — the anchor is validated (it must exist) but nothing is
  read or joined until [`.collect()`](../../nexus_traverse_api_reference) runs.
- See the [`NexusTraverse API Reference`](../../nexus_traverse_api_reference) for the full
  builder method list (`.join()`, `.via()`, `.pick()`, `.drop()`, `.name()`, `.collect()`).

## See Also

- [`NexusTraverse API Reference`](../../nexus_traverse_api_reference): every method on the
  builder this returns.
- [`Nexus::path`](path.md) / [`plan`](plan.md) / [`materialize`](materialize.md): the
  lower-level primitives `NexusTraverse` is built on top of.
- [`Nexus::join`](join.md): the single-hop primitive `NexusTraverse::join` wraps.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frames.html">
    <span class="label">Previous</span>
    <span class="title">frames</span>
  </a>
  <a class="next-link" href="from_atlas.html">
    <span class="label">Next</span>
    <span class="title">from_atlas</span>
  </a>
</div>
