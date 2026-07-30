# Nexus API Reference

This page documents the public `Nexus` surface with usage-first examples generated
from the current table API.

`Nexus` is a named, multi-table workspace built on top of [`DataFrame`](dataframe_api_reference.md)
— it holds a keyed collection of frames, layers on declared relationships between
them, and provides key/relationship auditing, join-graph pathfinding, a fluent query
builder ([`NexusQuery`](nexus_query_api_reference.md), returned by `from()`), and
save/load persistence.

```{note} No dedicated constructor page
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
directly to a method name. Several names are exact aliases of a canonical method
(`names`↔`keys`, `links`↔`relationships`/`add_relationship`, `meta`↔`table_meta`/
`set_table_meta`, `duplicates`↔`detect_duplicates`, `check`↔`validate_relationship`,
`orphans`↔`detect_orphans`, `audit`↔`relationship_audit`) — each alias has its own short
page pointing back to the canonical one.

## The Query Builder

`Nexus::from(table)` returns a [`NexusQuery`](nexus_query_api_reference.md), a
separate fluent builder for walking the join graph from one anchor table
(`.join()`/`.via()`/`.pick()`/`.drop()`/`.collect()`). See its own reference page for
the full method-by-method breakdown.

```{toctree}
:hidden:
:maxdepth: 1

api/nexus/add_relationship
api/nexus/apply
api/nexus/apply_all
api/nexus/audit
api/nexus/check
api/nexus/deduce
api/nexus/defined
api/nexus/detect_duplicates
api/nexus/detect_orphans
api/nexus/dloc
api/nexus/drop
api/nexus/duplicates
api/nexus/empty
api/nexus/fingerprint
api/nexus/flow
api/nexus/frame
api/nexus/frames
api/nexus/from
api/nexus/from_atlas
api/nexus/get
api/nexus/graph
api/nexus/grain_audit
api/nexus/has
api/nexus/has_relationship
api/nexus/has_store
api/nexus/has_table_meta
api/nexus/head
api/nexus/inject
api/nexus/join
api/nexus/keys
api/nexus/links
api/nexus/load
api/nexus/map_frames
api/nexus/materialize
api/nexus/meta
api/nexus/names
api/nexus/neighbors
api/nexus/nrelationships
api/nexus/ntables
api/nexus/orphans
api/nexus/path
api/nexus/plan
api/nexus/profile
api/nexus/pull
api/nexus/relationship
api/nexus/relationship_audit
api/nexus/relationships
api/nexus/rename
api/nexus/run
api/nexus/save
api/nexus/scan
api/nexus/schema
api/nexus/select
api/nexus/set_table_meta
api/nexus/show
api/nexus/size
api/nexus/store
api/nexus/summary
api/nexus/table_meta
api/nexus/tail
api/nexus/to_atlas
api/nexus/to_string
api/nexus/unique
api/nexus/validate_keys
api/nexus/validate_relationship
```
