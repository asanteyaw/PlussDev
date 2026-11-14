# NexusPull API Reference

This page documents the public `NexusPull` and `NexusPullAgg` surface — the
fluent root-grain feature builder returned by
[`Nexus::pull(root)`](nexus_api_reference.md), and its scoped
per-table reducer sub-builder returned by `NexusPull::agg(table)`.

Both types live in `pluss::bundle` beside `Nexus`. They are two different
public spellings of the exact same execution path
[`Nexus::pull()`](api/nexus/pull.md) documents in full: the string-grammar
overloads (`pull(root, {"orders.amount.sum"})`) and the fluent builder below
both bottom out in the same `PullSpec` list and the same pull engine. Nothing
here changes *what* a pull can express — only how ergonomic it is to build
that request in real feature-engineering code.

## What It Is

`nx.pull("customers")` returns a `NexusPull` anchored at `"customers"`. From
there:

- `.take(...)` requests plain (non-aggregate) fields — either from the root
  table itself, or from a related table by naming it explicitly.
- `.count(...)`, `.sum(...)`, `.mean(...)`, `.median(...)`, `.min(...)`,
  `.max(...)`, `.std(...)`, `.var(...)`, `.ndistinct(...)` request an
  aggregate field directly, given an already-qualified `"table.feature"`
  source string.
- `.agg(table)` returns a `NexusPullAgg` scoped to one related table, so
  several reducers on that table can be written without repeating the table
  name in every call — `.agg("events").count(...).sum(...).end()` instead of
  `.count("events.event_id", ...).sum("events.spend", ...)`.
- `.add(spec)` / `.add({specs...})` accepts a pre-built `PullSpec` directly —
  the only way to reach `.windowed(...)`/`.root_windowed(...)`, since those
  live on `PullSpec` itself, not on the builder.
- `.root()` / `.specs()` inspect the builder's accumulated state without
  executing anything.
- `.collect()` executes the accumulated spec list and returns the pulled
  `DataFrame` — the only method that touches the workspace.

Every builder method except `.collect()`, `.root()`, and `.specs()` returns a
reference back to the same builder (`NexusPull&` or `NexusPullAgg&`), so
calls chain in place; there is no hidden copying the way `Nexus`'s own
COW value methods work.

:::{note} `pluss::msharp::RangeView` is never spelled out in these examples
Several `take`/reducer overloads accept a `pluss::msharp::RangeView::Of<T>` —
a lightweight non-owning view type, not something user code is meant to name
directly. It has implicit constructors from a brace list, a `std::vector<T>`,
a `std::array<T, N>`, and a `std::span<const T>`, so passing any of those at
a call site works without ever writing `RangeView` yourself. Wherever an
overload set also includes a same-shaped `std::initializer_list<T>` overload
(added specifically to keep plain `{...}` brace calls unambiguous), these
pages call that one "the brace-literal form" and show it with plain `{...}`;
the `RangeView::Of<T>` overload is exercised by passing an existing **named**
container variable instead.
:::

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, and
  verified outputs.
- See [`Nexus::pull`](api/nexus/pull.md) for the field-expression grammar,
  aliasing rules, and windowed-aggregate semantics shared by every spelling
  of a pull.

## Method Pages

```{toctree}
:hidden:
:maxdepth: 1

api/nexus-pull/add
api/nexus-pull/take
api/nexus-pull/count
api/nexus-pull/ndistinct
api/nexus-pull/sum
api/nexus-pull/mean
api/nexus-pull/median
api/nexus-pull/min
api/nexus-pull/max
api/nexus-pull/std
api/nexus-pull/var
api/nexus-pull/agg
api/nexus-pull/root
api/nexus-pull/specs
api/nexus-pull/collect
api/nexus-pull/agg-count
api/nexus-pull/agg-ndistinct
api/nexus-pull/agg-sum
api/nexus-pull/agg-mean
api/nexus-pull/agg-median
api/nexus-pull/agg-min
api/nexus-pull/agg-max
api/nexus-pull/agg-std
api/nexus-pull/agg-var
api/nexus-pull/end
```
