# `procedure::persist` Introduction

## What This Category Owns

`persist` owns the directory-bundle format
[`Procedure::save`](../core/Procedure/save.md) writes and
[`procedure::load`](load.md) reads: a text manifest, one component
artifact per persistable step, and a human-readable audit directory --
plus `load`'s own reconstruction and integrity-checking behavior.

## The Bundle Layout

A saved bundle is a directory containing:

- `manifest.plussproc` -- a plain-text manifest: a `PLUSS_PROCEDURE_V1`
  header, procedure-level fields (schema version, `workers`/`schedule`/
  `cache`/`fail_fast`, step/link counts), then one `step`/`endstep`
  block per step (name, component, capabilities, every
  [`StepOptions`](../core/StepOptions.md)/[`StepPersistSpec`](../core/StepPersistSpec.md)
  field, and its artifact path plus checksum) and one `link`/`endlink`
  block per link.
- `steps/<index>_<sanitized-name>/` -- one component artifact per step
  whose [`can_save_component`](../core/Step/can_save_component.md) is
  `true`; a step with [`persistable`](../core/Step/persistable.md) `true`
  but no `Saver` (a [`SinkSpec`](../core/SinkSpec.md)-backed sink, or a
  plain registered callable) has no entry here at all.
- `audit/checksums.txt`, `audit/lineage.txt`, `audit/contracts.txt`,
  `audit/artifacts.txt`, `audit/summary.txt` -- tab-separated,
  human-readable summaries of the same information the manifest carries
  structurally, for a reader who wants to inspect a bundle without
  parsing the manifest itself.

## Save Requires Both `persistable` And `reloadable`

[`Procedure::save`](../core/Procedure/save.md) throws if any step is not
[`persistable`](../core/Step/persistable.md) -- documented on `save`'s
own page -- and *also* if any step is not
[`reloadable`](../core/Step/reloadable.md), a slightly stricter
requirement this page makes explicit: a step can declare itself
persistable without anything in the registry able to reconstruct it
later, and `save` refuses to write a bundle it already knows `load`
could never fully read back.

## Load Re-Verifies Everything

[`procedure::load`](load.md) does not trust the manifest blindly: every
step with a saved artifact gets its checksum recomputed and compared,
every step's reconstructed contract (component, persistable, key,
schema versions, reloadable/frozen/custom) is checked against what the
manifest itself recorded, and the whole reconstructed graph is run
through [`check`](../core/Procedure/check.md) exactly like a freshly
built one would be.

## Related Pages

- [`Procedure::save`](../core/Procedure/save.md) -- the other half of
  this story.
- [`registry`](../registry.md) -- where a `load`-time lookup for a
  custom component's key is resolved.
