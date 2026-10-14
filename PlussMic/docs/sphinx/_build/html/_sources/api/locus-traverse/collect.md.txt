<!-- pluss-hand-authored -->

# LocusTraverse::collect

[Back to LocusTraverse API Reference](../../locus_traverse_api_reference)

## Signature

```cpp
pluss::kindle::Catalog LocusTraverse::collect() const
```

## Materializing The Plan

`collect()` is the execution boundary of a `LocusTraverse` chain. The methods
before it describe work: `join()` appends one hop, `via()` appends a resolved
path, `options()` replaces relational policy, and `pick()` or `drop()` records a
final projection. `collect()` reads the captured Locus, executes the hops in
order, applies the projection, and returns a new `Catalog`.

A zero-hop plan is valid. Calling `collect()` immediately after
`locus.from("orders")` returns the anchor Catalog's streams and record layout.
This makes one builder useful for code that may or may not add relationships:
the terminal operation does not require a special case.

Each hop receives the Catalog produced by the previous hop. This matters in a
long plan because a right-side name collision can suffix a stream, and a later
hop must find its left key in that accumulated schema. Join matching preserves
the selected `JoinHow`; ragged stream layout follows the current
[`options()`](options.md). After the last hop, `pick()` runs first and `drop()`
runs second.

## Returns

`pluss::kindle::Catalog`
: A newly materialized, defined Catalog. Its stream order is the accumulated
  join order unless `pick()` supplies an explicit order. The result owns its
  Catalog structure; the source Locus and builder remain unchanged and may be
  reused.

The result can itself be ragged. With the default relational policy, trailing
absence is preserved rather than padded merely to make every stream the same
length. Use `options()` with `RaggedMaterialize::Pad` when a rectangular record
domain is required.

## Examples

All outputs are emitted by
`PlussMic/examples/bundle/locus_traverse_reference_demo.cpp`.

Example 1. A zero-hop collection returns the anchor Catalog.

```cpp
const auto anchor = data.from("orders").collect();
print_keys(anchor);
print_numbers(anchor, "order_id");
```

### Output

```text
keys=[order_id, customer_id, amount, state] rows=5
order_id=[100, 101, 102, 103, 104]
```

Example 2. `collect()` executes an accumulated adjacent join. The default join
is inner, so the order whose customer key has no partner is omitted.

```cpp
const auto joined = data.from("orders").join("customers").collect();
print_keys(joined);
print_words(joined, "name");
```

### Output

```text
keys=[order_id, customer_id, amount, state, name] rows=4
name=[Ada, Bola, Bola, Chen]
```

Example 3. Builders are persistent values. Deriving a projection or a joined
branch does not mutate the common base plan.

```cpp
const auto base = data.from("orders");
print_keys(base.pick({"order_id"}).collect());
print_keys(base.join("customers").pick({"order_id", "name"}).collect());
print_keys(base.collect());
```

### Output

```text
keys=[order_id] rows=5
keys=[order_id, name] rows=4
keys=[order_id, customer_id, amount, state] rows=5
```

Example 4. Projection is terminal: `pick()` establishes the candidate schema,
then `drop()` removes from that schema.

```cpp
const auto result = data.from("orders")
    .pick({"order_id", "state"})
    .drop({"state"})
    .collect();
print_keys(result);
```

### Output

```text
keys=[order_id] rows=5
```

## Validation Boundary

Some errors occur while the plan is formed: an unknown `join()` target, an
ambiguous direct relationship, or an unreachable `via()` target. Errors that
require materialized schemas occur here: unknown projection names, absent join
keys, incompatible key kinds, contradicted cardinality, output-name collision,
invalid ragged policy, and loss forbidden by `RaggedLoss::Error`.

Because collection is eager, no partial Catalog is returned when a later hop or
projection fails.

## Cost And Reuse

`collect()` allocates every join result and the final projection. Key encoding
and matching are delegated to Catalog's relational kernels; eligible numeric
built sides may reuse their internal match cache. `pick()` and `drop()` do not
push stream pruning into earlier joins, so place them for schema clarity rather
than expecting them to reduce join work. Reuse an uncollected base builder when
several related outputs share the same prefix, but expect each call to
`collect()` to execute its own materialization.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../locus_traverse_api_reference.html"><span class="label">Previous</span><span class="title">LocusTraverse API Reference</span></a>
  <a class="next-link" href="drop.html"><span class="label">Next</span><span class="title">drop</span></a>
</div>
