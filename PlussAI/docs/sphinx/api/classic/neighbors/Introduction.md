# `classic::neighbors` Introduction

## What This Family Owns

`neighbors` owns exactly one type: [`SpatialIndex`](SpatialIndex.md), an
exact k-nearest-neighbor index over a fixed point set. It is the smallest
family in this reference by symbol count — one class, three methods —
but a real, general-purpose tool: build it over any `[n, dim]` point set
and query it as many times as you like, with either an external batch
([`query_k`](SpatialIndex/query_k.md)) or a self-vs-self search
([`query_self_k`](SpatialIndex/query_self_k.md)).

## Where It's Used Internally

[`SpatialIndex`](SpatialIndex.md) was built specifically to replace the
CPU host-loop brute-force k-NN search that several other places in this
reference needed independently — most directly,
[`plan_balance`](../balance/plan_balance.md)'s own `BlendNeighbors`
synthesis and its cleaning's own neighbor searches, both via
`query_self_k`. It deliberately does **not** replace the separate,
`Series`/matmul-based pairwise-distance path those same call sites keep
as their own GPU fallback — `SpatialIndex` itself is CPU-only by
construction (a real host-memory recursive structure, not a vectorized
computation), so it complements that path rather than subsuming it.

## Exact, Never Approximate

Worth repeating from [`SpatialIndex`'s own Introduction](SpatialIndex/Introduction.md#exact-not-approximate):
every query this class answers is provably identical to what a
brute-force pairwise-distance scan over the same points would find. The
KD-tree's pruning is purely a speed optimization; it never trades away
correctness, at any dimensionality, though its speed advantage narrows
in higher dimensions.

## Reading Order

1. [`SpatialIndex`](SpatialIndex.md) — the hub page (Signature/
   Construction/State/Methods table).
2. [`SpatialIndex::build`](SpatialIndex/build.md) — how you obtain one.
3. [`SpatialIndex::query_k`](SpatialIndex/query_k.md) — external-batch
   queries, and [`KNearestResult`](SpatialIndex/query_k.md#returns),
   the shared result type.
4. [`SpatialIndex::query_self_k`](SpatialIndex/query_self_k.md) —
   self-vs-self queries.

## Related Pages

- [`balance`](../balance.md) — [`plan_balance`](../balance/plan_balance.md),
  this family's own internal consumer.
