# Nexus API Reference

This page documents the public `Nexus` surface with usage-first examples generated
from the current table API.

`Nexus` is a named, multi-table workspace built on top of [`DataFrame`](dataframe_api_reference.md)
— it holds a keyed collection of frames, layers on declared relationships between
them, and provides key/relationship auditing, join-graph pathfinding, a fluent query
builder ([`NexusQuery`](nexus_query_api_reference.md), returned by `from()`), and
save/load persistence.

```{important} This reference is being built out in batches
Not every `Nexus` method has its own page yet. The sidebar and the list below only
include methods that are already documented — more are being added incrementally.
Construct a `Nexus` with `Nexus({{"name", frame}, ...})` (a vector or brace list of
name/`DataFrame` pairs); there is no dedicated constructor page, the same as
`DataFrame`'s own constructors.
```

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant
  scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump
directly to a method name.

## The Query Builder

`Nexus::from(table)` returns a [`NexusQuery`](nexus_query_api_reference.md), a
separate fluent builder for walking the join graph from one anchor table
(`.join()`/`.via()`/`.pick()`/`.drop()`/`.collect()`). See its own reference page for
the full method-by-method breakdown.

```{toctree}
:hidden:
:maxdepth: 1

api/nexus/apply
api/nexus/apply_all
api/nexus/deduce
api/nexus/defined
api/nexus/dloc
api/nexus/drop
api/nexus/empty
api/nexus/flow
api/nexus/frame
api/nexus/frames
api/nexus/get
api/nexus/has
api/nexus/head
api/nexus/inject
api/nexus/keys
api/nexus/map_frames
api/nexus/names
api/nexus/ntables
api/nexus/profile
api/nexus/rename
api/nexus/scan
api/nexus/select
api/nexus/size
api/nexus/summary
api/nexus/tail
api/nexus/unique
```
