# PlussMic parity audit: table vs kindle vs scribble

Status: FIRST PASS (2026-11-26). Method: compare the public API reference page
names (`PlussMic/docs/sphinx/api/<class>/*.md`) of each class, level by level.
This finds what is **absent by name**. It does not prove that a method with the
same name behaves the same way; that needs a semantic pass later. Items marked
"decision" need the maintainer.

## The rule

The table stack (Shelf / DataFrame / Atlas) is the specification. Kindle
(Stream / Catalog / Archive) and scribble (Slot / Registry / Axel) mimic it:
same names, signatures, semantics, output shapes. They drift only where their
nature blocks it, and every drift is recorded here with its reason.

Generality ladder (ascending): table, kindle, scribble.
- table: rectangular, one type per column. Advantage: performance.
- kindle: one type per column, ragged allowed.
- scribble: mixed types per column, ragged allowed.

Nothing table can do should be impossible in kindle or scribble unless nature
forbids it. User advice for the docs: rectangular and same-typed, use table;
same-typed columns but uneven, use kindle; mixed, use scribble.

| Level | table | kindle | scribble |
|-------|-------|--------|----------|
| column | Shelf (175 pages) | Stream (85) | Slot (55) |
| frame | DataFrame (201) | Catalog (117) | Registry (90) |
| stack | Atlas (53) | Archive (62) | Axel (50) |

## Level 1: Shelf vs Stream vs Slot

### Not gaps (covered another way)
- Named arithmetic, comparison and logic (`add sub mul div mod pow`, the `r*`
  forms, `eq ne lt le gt ge`, `land lor lxor`, `ror rxor`): Stream has the
  operators (`operator+` etc.). **To verify**: the `r*` forms and `pow`, and
  that Slot has operators at all (it has none).
- Text methods (`strip lstrip rstrip lower upper casefold titlecase length starts
  ends has matches fullmatch replace pad chars encode normalize split partition
  indicate`): Stream exposes them through `nt()`, as Shelf does (the direct
  Shelf forms are the older spelling). Slot also has `nt()`.
- Label methods (`name named unname`): Stream has the label accessor
  (`lbl`, `with_label`, ...). **Decision**: same names as Shelf, or keep `lbl`?

### Capability gaps in Stream (should exist; nothing in a ragged column blocks them)
- Reductions and summaries: `all any count prod median mode percentiles se skew
  kurtosis describe info info_string scan`.
- Counting nulls and non-finite: `nnull nnan nposinf nneginf isfinite`.
- Cumulative: `cumsum cummin cummax cumprod csmin csmax`.
- Ordering: `sort`/`arrange`, `rank`, `top`, `bottom`, `duplicated`.
- Bounds and binning: `between bounded bucketize round`.
- Relations: `corr corr_with covar compare matmul`.
- Windows and fills: `window shift interp fillnull_`.
- Shaping and sampling: `append put repeated sample shuffle rand align extract
  transform flow fold group weigh view ncases`.
- Value box: no `StreamValue`; `Stream::sum/mean/min/max` return a Stream where
  Shelf returns a `ShelfValue`, and `Stream::value()` is missing.
- Bridges: `framerize`, `kindelize` (Shelf has both; Stream has `tabelize`,
  `to_shelf`, `from_shelf`).

### Possible nature drift in Stream (decisions)
- `corr`, `covar`, `matmul`: need equal-length operands. A ragged stream
  allows unequal lengths; likely valid with an explicit alignment rule.
  Record the rule when decided.

### Slot (scribble) beyond Stream
Slot lacks everything above and also these that Stream has: `abs exp log clip std
var map select bfill ffill label dtype plot ts as_series as_note as_timeline
as_duration scribbelize`.

Nature drift for Slot (heterogeneous cells):
- Numeric methods (`abs exp log clip std var cumsum ...`) apply to Series cells
  and must define what a Note, Timeline, Duration or NULL cell does. Proposal:
  Series cells compute; other kinds raise `pluss::Error` naming the cell
  position and kind, unless the method has a natural meaning for it (a
  Duration can be summed). Decide per method family and record it.
- `dtype`: per cell, not per column. Proposal: `dtypes()` returning a Slot or
  Note of per-cell dtypes. Decision.
- `as_series()` etc. on a whole Slot: only when every non-null cell has that
  kind; otherwise throw. The per-cell form is `SlotValue` (see below).

### Value boxes
`ShelfValue` exists. Needed: `StreamValue` and `SlotValue`, same surface as
`ShelfValue` (`kind() as_series() as_note() as_timeline() as_duration() host()
to_dbl() to_int() to_str()`, implicit payload conversions, `defined()`),
with scribble-only drift: a cell may be NULL (`is_null()`, a null constant).
`Slot::Value` becomes an alias of `SlotValue`; `Slot::at(i)` and `values()`
return it, so public code never sees `std::variant`.

## Level 2: DataFrame vs Catalog vs Registry

DataFrame has 201 pages. Catalog lacks 122 of them by name, Registry lacks 144.
The kindle and scribble classes also have pages DataFrame does not (these are
nature additions, not gaps): Catalog `keys entries eloc nkeys nentries merge
reduce match_keys keywords similarity tfidf stats ...`; Registry `fields
fieldaxis sloc nstate morph keep prune profiles ...`.

Gaps to carry over (grouped; nothing in raggedness blocks them unless noted):
- Arithmetic and comparison: `add sub mul div ... eq ne lt ... land lor lxor`
  and the `r*` forms, `abs exp log pow round clip`.
- Reductions: `all any count prod median mode percentiles se skew kurtosis
  describe info_string scan tally`, null counters, `corr corr_with covar matmul`.
- Cumulative and ordering: `cum* cs* arrange rank top bottom duplicated
  drop_duplicates shift interp`.
- Shaping: `assign insert extend lengthen place put put_ update reorder
  reshuffle shuffle rand repeated stack unstack pivot pivot_table transpose
  unsqueeze zoom mask conform unify copy equals`.
- Metadata: `dtypes ptypes labels features feataxis nfeatures ncases height width
  title title_` (partly nature: Catalog is keyed by entry, not feature).
- Relational and SQL: `sql prep_sql query`, `contingency observe factor
  case_study`.
- Higher-order: `flow fold`, `aggregate`.
- Bridges: `kindelize`.

Registry additionally lacks (relative to DataFrame, and Catalog has them):
`astype bfill ffill count_kinds filter group info join max mean min plot plot3d
query scribbelize select sort std sum transform ts var where`.

Decisions: SQL over ragged and heterogeneous frames (probably through
`tabelize()`/`kindelize()` bridges with a documented drop/pad rule);
`transpose`/`pivot` over ragged data (output is ragged, valid).

## Level 3: Atlas vs Archive vs Axel

Atlas has 53 pages. Missing in Archive: `align_depth axis0_labels column_names
features flow frame frames kindelize map map_frames ncases nfeatures
relabel_depth slice_frames sort_depth squeeze`. Missing in Axel: those plus
`aggregate_depth dloc label loc plot scribbelize ts`.

Archive and Axel additions (nature, keep): Archive `catalog catalogs keys
nkeys nrecords minrecords maxrecords minkeys maxkeys slice_depth map_catalogs`;
Axel `registry registries fields fieldaxis labels concat_depth`.

## Next steps (proposed order)

1. Maintainer reviews this first pass and settles the **decisions** above.
2. Build `StreamValue` and `SlotValue` (header, implementation, tests, doc pages
   mirroring `ShelfValue`).
3. Close level-1 gaps in Stream, then Slot; then level 2; then level 3. Each
   method lands with a test and a doc page to the Shelf standard.
4. Semantic pass: for methods present under the same name, run the same inputs
   through Shelf and Stream/Slot and compare results.
5. Rewrite the Slot/Registry/Axel docs (385 pages) against the Shelf benchmark.
