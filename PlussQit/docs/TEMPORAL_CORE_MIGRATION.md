# Temporal Core Migration

## Decision

The public Pluss value system converges on four core types:

| Type | Meaning |
| --- | --- |
| `Series` | Numeric, boolean, and differentiable values |
| `Note` | Textual, byte, token, and language-oriented values |
| `Timeline` | Absolute instants interpreted through a timezone and calendar |
| `Duration` | Signed elapsed quantities represented at nanosecond precision |

`TimeIndex` is not a fifth value kind. It is the role played by a rank-one
`Timeline` when an operation needs an ordered temporal axis. Its public
contract therefore moves to `Timeline::index()`.

`Duration` remains the fourth type rather than being renamed to `Period`.
A period is an anchored calendar concept whose length can depend on its
position and timezone; a duration is an elapsed quantity. If Pluss later
needs anchored periods, that concept requires a separate contract instead of
changing the meaning of `Duration`.

## Public Index Contract

`Timeline::index()` is the canonical validation boundary:

```cpp
const auto checked = timeline.index();       // rank one and non-decreasing
const auto unchecked = timeline.index(false); // rank one; ordering unchecked
```

The returned accessor owns a cheap Timeline handle. Construction validates
the requested contract once, and all operations invoked through that accessor
reuse the validated state:

```cpp
const auto index = timestamps.index();

const auto position = index.searchsorted(key);
const auto previous = index.asof(key);
const auto bins = index.resample_bins(options);
const auto joined = index.join_asof(other.index());
```

Rank validation is constant-time. Monotonic validation is linear in the
number of timestamps, but is paid once for a retained accessor rather than by
every index operation.

A permanent monotonicity cache must not be attached to Timeline storage yet.
`Timeline::from_buffer()` can borrow mutable external memory, and writes made
through the external owner cannot update a Timeline mutation generation.
Caching inside the accessor is safe because it describes the storage state
that the caller explicitly validated. Callers must not mutate borrowed storage
while using an accessor whose ordering contract depends on that storage.

## Internal Engine

The existing TimeIndex implementation is retained as the initial index
engine. This avoids rewriting proven search, as-of join, alignment, rolling,
and resampling kernels while the public ownership model changes.

The intended end state is:

```text
Public
  Timeline
  Timeline::TimelineIndexAccessor Timeline::index()

Internal
  pluss::codeh::detail::TimeIndexEngine
```

Migration proceeds in compatibility-preserving stages:

1. Add `Timeline::index()` and route it through the existing validation and
   TimeIndex kernels.
2. Make new code use `timeline.index().method()`.
3. Retain public `TimeIndex`, `pluss::timeindex::*`, and
   `pluss::make_timeindex(...)` temporarily as compatibility forwarders.
4. Change exported PlussMic APIs to accept `Timeline`; operations requiring an
   index call `.index()` at their boundary.
5. Merge PlussMic's Timeline and TimeIndex payload alternatives.
6. Decode legacy serialized `TimeIndex` values as Timeline while writing only
   the Timeline representation for new data.
7. Remove the public TimeIndex type, factories, exports, and documentation.
8. Move the retained implementation behind `Timeline::TimelineIndexAccessor` as
   `detail::TimeIndexEngine`.

Compatibility names are transitional, not permanent aliases. The migration
is complete only when public headers and generated documentation no longer
expose TimeIndex as a value type.

## PlussMic Duration Contract

PlussMic currently stores `Series`, `Note`, `Timeline`, and `TimeIndex` as
separate payload alternatives. The final payload set becomes:

```text
Series
Note
Timeline
Duration
```

Removing TimeIndex therefore creates the runtime-type position needed by
Duration without increasing the number of payload alternatives. Index status
belongs to DataFrame/Slate axis metadata and operation contracts, not to a
column's value dtype.

Duration integration must define behavior throughout PlussMic rather than
merely adding it to a variant:

| Area | Required contract |
| --- | --- |
| Loading and serialization | Preserve signed nanoseconds and lane-wise `NaT` |
| Display | Render elapsed quantities, never timestamp calendar text |
| Arithmetic | Preserve `Duration` for duration addition and subtraction |
| Temporal algebra | `Timeline - Timeline -> Duration`; `Timeline +/- Duration -> Timeline` |
| Ratios | `Duration / Duration -> Series` |
| Comparison | Compare duration values with shape and absence rules preserved |
| Reduction | Sum, mean, minimum, and maximum return Duration where meaningful |
| Structure | Filtering, grouping, reshaping, and concatenation preserve dtype |
| Casting | Keep elapsed quantities distinct from epoch timestamps |

Legacy TimeIndex serialization tags remain readable during the transition but
are promoted to Timeline immediately after decoding. Enum values and persisted
tags must not be silently reused for Duration.

## Completion Criteria

The migration is complete when:

- public Pluss exposes exactly `Series`, `Note`, `Timeline`, and `Duration` as
  core value types;
- every index operation is reached through `Timeline::index()`;
- PlussMic has no TimeIndex payload, kind, cast target, or public overload;
- PlussMic supports Duration across storage, I/O, display, arithmetic,
  comparison, reduction, grouping, and reshaping;
- old serialized TimeIndex data loads as Timeline;
- `Queue` and PlussTils no longer register TimeIndex and continue to register
  Duration; and
- compatibility factories and TimeIndex documentation have been removed.

## Current Checkpoint

The compatibility phase is active. `Timeline::index()` is public and its
`Timeline::TimelineIndexAccessor` name currently aliases the proven TimeIndex engine, so
new source and documentation use the accessor contract without duplicating
kernels or invalidating existing TimeIndex consumers. The complete handwritten
TimeIndex method family has moved to the `timeline-index` documentation family;
the old landing page now exists only to explain the transition.

Duration is now a first-class payload through the initial PlussMic structural
boundary:

- `Slate`, `Stream`, `Slot`, `Registry`, and `Catalog` preserve Duration kind,
  signed nanoseconds, lane order, and validity;
- explicit casts support Duration to Int64 nanoseconds or Note text and Int64
  or exact decimal-nanosecond Note values back to Duration, while
  Duration-to-Timeline is rejected;
- shared scalar formatting, comparison, append, sorting, extrema, alignment,
  group hashing, and distinct counting have Duration policies;
- DataFrame null allocation, cast routing, exact join keys, category decode,
  reshape concatenation, axis matching, `matrix()`, pivot materialization,
  `fillnull`, and `replace` preserve Duration;
- BI scalar and inclusive-range filters carry typed Duration bounds, compare
  through Duration semantics, and serialize bounds as exact signed
  nanoseconds;
- CSV, Excel, SQLite, JSON emission, and Arrow/Parquet adapters have explicit
  Duration branches; Arrow uses its native nanosecond Duration type, while
  text-schema ingestion requires exact decimal nanoseconds rather than unit
  guessing; and
- display, diagnostics, catalog alignment, Nexus labels, and modal reduction
  retain Duration identity and treat `NaT` as absence.

Public TimeIndex remains available at this checkpoint. The remaining work is
to finish Duration arithmetic and the remaining reduction policies across
PlussMic, verify the Arrow branch in an Arrow-enabled build, remove the
transitional TimeIndex names from exported variants and enums, read legacy
TimeIndex serialization as Timeline, and only then remove the compatibility
type, factories, and old landing page.

The value-kind collapse has begun at the PlussMic storage boundary. `Slate`
and `Stream` now store temporal payloads only as `Timeline`; their temporary
TimeIndex constructors normalize immediately, and compatibility extraction
projects through `Timeline::index(false)`. `Slot` applies the same
normalization to constructor and mutation input, so a Scribble round trip
cannot reintroduce TimeIndex storage. `StreamKind::TimeIndex` and
`SlotKind::TimeIndex` have been removed, and new Stream, Slot, and Catalog JSON
emits only the Timeline representation. Their transitional TimeIndex overloads
remain temporarily as input adapters and produce Timeline-backed values.
`SlateKind::TimeIndex` and its downstream switch branches remain for the next
compiler-guided cleanup phase, although the current Slate constructors can no
longer produce that runtime kind.

The PlussMic accessor cutover is also complete. Public `.timeindex()` cast and
axis-selection methods have been removed from Slate, DataFrame, Stream,
Catalog, Archive, Slot, Registry, and Axel. Their Duration replacements are
available consistently: container cast accessors expose `.duration()`, and
DataFrame feature-axis, Catalog/Archive key-axis, and Registry/Axel field-axis
accessors select Duration values directly. Duration-aware fill operations now
span Stream, Catalog, Archive, Slot, Registry, DataFrame, and Slate. Remaining
TimeIndex references are compatibility constructors and scalar overloads,
`SlateKind`/`SlateCastTarget` branches, legacy matrix payload alternatives,
and CodeH index-engine option types; none is a `.timeindex()` user call form.
