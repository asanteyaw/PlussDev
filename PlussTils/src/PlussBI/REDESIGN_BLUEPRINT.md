# PlussBI Redesign Blueprint

Status: DRAFT 1 (design discussion with the maintainer; not yet implemented).
Supersedes the "programmatic dashboard builder" direction in `ARCHITECTURE.md`
(see "What changes in ARCHITECTURE.md" below). Nothing here is built yet; every
claim in the audit was checked against the source on the date of writing and is
marked **verified** or **to verify**.

---

## 1. What PlussBI becomes

PlussBI stops being a C++ library for hand-writing dashboards. It becomes an
**application**: a Tableau-style tool you start with

```
pluss bi [file.pbw]
```

which starts a local engine process, opens a browser tab, and lets you do the
whole analysis by point-and-click: load data, clean it, join tables, compute
columns and measures, aggregate, rank, run time-series and statistics, build
charts, assemble dashboards, publish.

Everything the UI does is a recorded, editable **step**, and every step can be
shown as the **C++ code** that does the same thing with the Pluss library
("Show code"). The UI and the generated code run the same library calls, so
they cannot drift apart. That is a hard rule (section 6).

### Analysis tasks PlussBI must cover

| # | Task | Phase |
|---|------|-------|
| 1 | Import data from CSV, Excel, databases, APIs | 1 |
| 2 | Clean missing values, duplicates, bad data | 1 |
| 3 | Filter, sort, select subsets | 1 |
| 4 | Merge/join multiple datasets | 2 |
| 5 | Group and aggregate | 2 |
| 6 | New calculated columns | 3 |
| 7 | Custom metrics and KPIs (measures) | 3 |
| 8 | Time-series analysis | 4 |
| 9 | Percentage changes and growth rates | 4 |
| 10 | Ranking and top-N | 2 |
| 11 | Cumulative sums and rolling calculations | 4 |
| 12 | Conditional logic | 3 |
| 13 | Statistical analysis | 5 |
| 14 | Interactive dashboards and visualizations | 2 to 6 |
| 15 | Predictive modelling / machine learning | much later |

---

## 2. Decisions locked in the design discussion

1. **GUI-first, no programmatic authoring surface.** A dashboard is built in
   the UI, not in C++. Code is an *output* ("Show code"), not the input.
2. **Browser only.** No native pop-up window. PlussView goes back to its own
   job (render tables and plots) and is not a PlussBI host.
3. **Launch:** `pluss bi` from the terminal.
4. **Files, modelled on Tableau's `.twb` / `.twbx` (see 4.4).**
   - **`.pbw`, the working file**: small JSON holding the pipeline steps,
     selections, measures, dashboard layout and *references* to the data. Good
     for version control.
   - **`.pbi`, the packaged workbook**: a ZIP holding the same definition plus
     the data extracts. One shareable file; reopens editable under `pluss bi`.
   - **Web export (`.html`)**: a plain self-contained HTML dashboard made from a
     `.pbi`, for viewing in any browser with no engine. Exported, not edited.
   - "Editing" always means the browser tab served by the local engine (4.1).
     There is no desktop editor, no VS Code integration, no Electron.
5. **One-way dependency and a placement rule** (section 3).
6. **A small, careful, functional expression language** for formulas and
   measures (section 7).
7. **Selection is a first-class value** with three ways to create it
   (section 8).
8. **Vendored, small front-end stack.** Prefer small libraries over large
   frameworks (W3.CSS over Bootstrap, for instance). No npm step at user
   install time.

---

## 3. The placement rule (the most important rule in this document)

PlussBI must not bloat the Pluss libraries. For every capability, ask:

| Question | Where it goes |
|----------|---------------|
| Would a Pluss user with no BI interest use it? Is it plotting? | **Flash** |
| Is it a data manipulation or table-coordination feature? | **Nexus**, or the DataFrame/Series/Note/Timeline underneath |
| Does it exist *only* to serve the BI application? | **`BI_tools`** (the abstraction layer) |

`BI_tools` lives at `PlussTils/include/BI_tools/` (exists, currently empty).
It **delegates** to Flash, Nexus, DataFrame, Series, Note and Timeline, applies
the BI-specific transformation, and serves the result to the BI application.
It must not re-implement join, group, cleaning, rolling or statistics logic.

Dependency direction (never reversed):

```
BI application (browser UI + local server)
        |
     BI_tools   (BI-only glue: language, steps, session, selection, formats)
        |
   Flash   Nexus   DataFrame / Series / Note / Timeline   PlussMa
```

Nothing in Flash, Nexus or the DataFrame may mention PlussBI.

### Worked example: hit maps

Brushing, hover and cross-filtering need the browser to know which row each
mark is. That information exists only inside Flash during layout, so it cannot
be rebuilt outside without duplicating Flash's layout logic.

- Flash gains a **standalone, generic function** that takes a plot and returns
  the geometry of its marks with their source-row ids (working name
  `flash::marks(plot)`; the final name is decided at implementation, see the
  naming rules). Any Pluss user can call it. It is not a BI feature.
- `BI_tools` wraps it: it turns "marks plus row ids" into the selections and
  filters the BI session understands, and into the JSON the browser needs.

---

## 4. Architecture

### 4.1 Process model

```
 terminal: pluss bi data.pbw
        |
        v
 +---------------------------+        HTTP + WebSocket       +------------------+
 | Local engine process      | <---------------------------> | Browser tab (UI) |
 |  - Nexus workbench        |   operations in, small        |  - data grid     |
 |  - BI_tools               |   results out                 |  - step list     |
 |  - cpp-httplib server     |                               |  - chart/dash    |
 +---------------------------+                               +------------------+
```

- The browser is only the interface. All data work runs in C++.
- The UI sends *operations* (add step, edit step, select, filter). The engine
  returns *small results* (a page of rows, an aggregate, mark geometry), never
  whole large tables.
- Platform scope: macOS first. Windows and Linux are documented as in
  development.
- Security baseline for the local server: listen on 127.0.0.1 only, a random
  per-session token required on every request, `Origin`/`Host` checks. A user
  editing their own data in their own browser is not a threat; other websites
  or local processes reaching the engine are.
- cpp-httplib is already vendored (`third_party/httplib`) and used by the
  existing live server (`pluss/bi/serve.h`). WebSocket support must be added or
  replaced (open question Q1).

### 4.2 The pipeline (step list)

The core data structure. A project is an ordered list of steps over a Nexus:

```
import  ->  clean  ->  join  ->  add column  ->  filter  ->  group  ->  rank ...
```

- Each step has: a kind, parameters, and the table(s) it reads and writes.
- Steps are immutable values. Editing a step re-runs the pipeline from that
  step (cached up to it). Undo is "drop the last step". Refresh re-runs from
  `import`.
- Each step kind has a `to_code()` that emits the C++ for it. "Show code" is the
  concatenation of the steps' code plus the chart code. Rule 6 below.

### 4.3 Session state

Interaction is session/state driven only (locked in `ARCHITECTURE.md` and kept):
slicers, selections, brushes, cross-filters and parameters all write to one
session state; tiles re-query from it. No visual-to-visual wiring.

### 4.4 Files

Three artifacts, one definition format.

**`.pbw` working file.** JSON (text, diff-friendly). Holds steps, measures,
named selections, dashboard layout, and data references (path/URL/connection
plus a schema fingerprint so a changed source is detected). Does not hold the
data. Equivalent of Tableau's `.twb`.

**`.pbi` packaged workbook.** A ZIP archive (the same container idea as
`.twbx`, `.xlsx`, `.docx`):

```
sales.pbi                    (ZIP)
  manifest.json              format version, created-by, table fingerprints
  project.json               the same definition a .pbw holds
  data/orders.parquet        one extract per table the project needs
  data/customers.parquet
```

- Opening it with `pluss bi sales.pbi` unpacks the extracts and opens the
  editor. Saving writes the archive back. It is a normal editable project that
  happens to carry its data, so it can be mailed or archived as one file.
- A `.pbw` can be packaged into a `.pbi` ("Package") and a `.pbi` can be
  unpacked to a `.pbw` plus data files ("Unpack").
- Publish option "strip editing": removes steps and source references so a
  shared `.pbi` does not reveal the pipeline or file paths. Keeps what the
  dashboard needs.

**Web export `.html`.** "Export for web" writes one self-contained HTML file: the
dashboard, the extracts for the data its visuals use, and a small viewer. Any
browser opens it by double-click; no engine. Static: interactivity is limited
to what the embedded extract supports (slicers and cross-filtering over the
embedded data; no new queries). A live mode that talks to a running engine can
come later.

Why not make the `.pbi` itself HTML: a browser cannot open a ZIP, and the
HTML-prepended-to-ZIP trick is fragile. The split keeps each file honest.

**Container implementation.**
- Extracts are written with the existing `save_parquet`
  (`PlussMic/src/loader/Loader.h`, also `DataFrame::save_parquet`); read back
  with `load_parquet`. Verified present.
- ZIP read/write: `ExcelLoader.cpp` already reads `.xlsx` through libzip, but
  only behind the optional `PLUSSMIC_HAS_LIBZIP` flag, and no vendored zip
  library was found. A format that cannot work on some builds is not acceptable
  for `.pbi`. Decision needed (Q9): make libzip a required dependency of the BI
  build, or vendor miniz (single small file, permissive licence, consistent with
  the vendored-small-libraries rule).
- Not an Excel workbook and not tied to any office library: the container is a
  plain ZIP with a documented layout.

Both `manifest.json` and `project.json` carry a format version from day one, and
the loader refuses (with a clear `pluss::Error`) a version it does not know.

---

## 4A. Security and trust model

Format decision: `project.json`, `manifest.json` and `.pbw` are JSON (not XML):
the browser reads JSON natively, `pluss/bi/detail/json_lite.h` exists, and XML
parsers need hardening against entity-expansion/XXE attacks.

Four separate threats, each with its own defence. Every defence has a test.

### T1. Another website or process drives the local engine
- Listen on 127.0.0.1 only, on a random free port.
- A random per-session token, given once in the launch URL and then held in a
  `SameSite=Strict` cookie; every request must carry it.
- Reject requests whose `Origin`/`Host` is not the engine's own. No CORS headers.
- A body-size limit on every route (the existing `serve.h` already caps one).
- The import file browser lists only paths under the user's home directory.
- Tests: wrong token, wrong Origin, wrong Host, oversized body all return an
  error and change no state.

### T2. A shared `.pbw` / `.pbi` is untrusted input
Opening a file someone else sent must not run anything on its own.
- **Untrusted mode is the default for any file not created on this machine.**
  Steps that read an external source (path, URL, database, API) are shown as
  "not run" and use the embedded extract when one exists. Each external source
  needs an explicit per-source approval from the user in the UI, and the
  approval is not saved in the shared file.
- Credentials are never stored in a project file. A source records a reference
  to the macOS Keychain; the secret stays in the Keychain.
- Loading checks: unknown format version is refused; `project.json` size limit;
  JSON nesting-depth limit; unknown step kinds are refused, not skipped.

### T3. The `.pbi` ZIP container
- Reject entries whose normalised path leaves the extraction root ("zip-slip"),
  absolute paths, and symlink entries.
- Limits: entry count, per-entry and total unpacked size, and compression ratio,
  so a small archive cannot expand into gigabytes. Check the sizes declared in
  the archive and also count actual bytes read.
- Unpack into a private temporary directory owned by the session; remove it on
  exit. Never unpack beside user files.
- Parquet extracts are parsed by the existing loader; apply a row/column limit
  before materialising.

### T4. The expression language
- A closed set of pure functions over tables and values. No function reads
  files, runs commands, opens connections, or reads the environment.
- Limits: formula length, nesting depth, iteration count in `FOR`, and result
  size, so a formula cannot hang or exhaust memory.
- Errors are `pluss::Error` with the formula position; they never crash the
  engine.
- Generated C++ ("Show code") is text for the user to review and run
  themselves; the engine never compiles or runs it.

### T5. The web export `.html`
- It contains whatever data its visuals need, readable through the browser's
  developer tools. "View-only" is not protection. The export dialog says so and
  shows exactly which tables and columns are embedded; "strip editing" removes
  steps and source references. Never embed a column the visuals do not use.

### Order of work
T1 is built with the server (Phase 1). T3 and T2 are built with `.pbi`
packaging, and the untrusted-mode behaviour exists from the first time a file is
opened (Phase 1, even before `.pbi`: a `.pbw` can already name external
sources). T4 is built with the language (Phase 3). T5 ships with the web export
(Phase 6).

---

## 5. Audit: what exists, and where gaps must be built

Legend: **HAVE** exists and is verified; **PARTIAL** exists but incomplete for
BI; **MISSING** not found. "Layer" is where the work belongs under section 3.

### 5.1 Data in and out

| Need | State | Evidence / gap | Layer for gap |
|------|-------|----------------|---------------|
| CSV, TXT, Excel (incl. multi-sheet workbook), Parquet, JSON | HAVE | `PlussMic/src/loader/Loader.h` (`load_csv/txt/excel/parquet/json`, `load_excel_workbook`) | |
| Load from URL | PARTIAL | `SourceResolver.cpp` uses libcurl behind `PLUSSMIC_HAS_CURL`; optional at build time | build option |
| Database connections (SQL servers, SQLite) | MISSING | no driver code in `loader/` | **loader** (new connector layer; start with SQLite) |
| REST/API sources (auth, paging, JSON to table) | MISSING | URL fetch exists, no auth/paging | **loader** |
| Save/export (CSV, Excel, TXT) | HAVE | `save_csv/excel/txt/save_as` | |
| Schema/type inference | HAVE | `Nexus::deduce`, `scan`, `profile` | |

### 5.2 Cleaning

| Need | State | Evidence |
|------|-------|----------|
| Missing values: detect, drop, fill | HAVE | `isnull/notnull/dropnull/fillnull`, `ffill/bfill/interp`, `dropna`-family on DataFrame and `Nexus` |
| NaN/inf as values distinct from NULL | HAVE | `isnan/dropnan/fillnan`, `isinf/...` (NULL vs NaN separation done) |
| Duplicates | HAVE | `drop_duplicates`, `Nexus::detect_duplicates/duplicates` |
| Bad data / type problems | PARTIAL | `astype/cast`, `Nexus::audit/grain_audit/relationship_audit` exist; no single "data quality report with suggested fixes" for the UI | **Nexus** (a `profile`-driven suggestion list) |
| Replace values, string clean (trim, case) | HAVE | `replace`, `strip/lstrip/rstrip`, `upper/lower/title` |

### 5.3 Subsets, sort, joins

| Need | State | Evidence |
|------|-------|----------|
| Filter / where | HAVE | `filter`, `where`, `query` |
| Sort | HAVE | `sort`, `order`, `arrange` |
| Select / drop / rename | HAVE | `select`, `drop`, `rename` (also on `Nexus`) |
| Multi-table relationships (keys, cardinality), join plans, join, traverse | HAVE | `Nexus::add_relationship`, `join`, `from(...).join(...).via(...)`, `materialize`, `store/run`, `path` |
| Join audits (grain, key validity) | HAVE | `validate_keys`, `grain_audit`, `relationship_audit` |
| Visual join builder helpers (suggest a key between two tables) | MISSING | `neighbors`/`path` exist, no key *suggestion* | **Nexus** |

### 5.4 Aggregation, calculated columns, conditionals

| Need | State | Evidence / gap | Layer for gap |
|------|-------|----------------|---------------|
| Group and aggregate | HAVE | `group`, `agg`, `grouped_window_aggregate`, `Nexus::pull(...).sum/mean/...` | |
| Pivot | HAVE | `pivot`, `pivot_table` (`DataFrameView`), `stack/unstack` | |
| New column from expression | PARTIAL | `assign`, `map`, `transform`, `where`; **no string expression parser** exists | **Nexus/DataFrame** or `BI_tools`, see 5.8 |
| Conditional logic | PARTIAL | `where`, `map`; the SQL engine appears to lack `CASE WHEN`, window functions, `HAVING` (grep found only the token "case"; **to verify** against the parser) | **DataFrame SQL** if SQL remains a lowering target |
| Measures with filter context | MISSING | the existing `QuerySpec` is a query, not a reusable context-aware measure | `BI_tools` (BI-only concept) |

### 5.5 Time series, growth, ranking, windows

| Need | State | Evidence |
|------|-------|----------|
| Rolling, expanding, EWM windows | HAVE | `rolling`, `expanding`, `ewm`, `rolling_block`, `zscore_rolling` |
| Cumulative sum/min/max/product | HAVE | `cumsum/cummin/cummax/cumprod` |
| Lag/lead/diff/shift, returns, log returns | HAVE | `lag/lead/diff/shift`, `returns`, `log_returns` |
| Percentage change, growth rates | PARTIAL | `returns` and `diff` cover it; a named `pct_change`/YoY/period-over-period is not present (**to verify**) | **DataFrame/Series** |
| Resample/frequency, calendar fields | HAVE | `resample`, `asfreq`, `year/month/quarter/week/...`, `month_start/end` |
| Time intelligence (YTD, QTD, same-period-last-year) | MISSING | no named time-intelligence ops | **Timeline/DataFrame** (generic; useful outside BI) |
| Rank, top-N, bottom-N | HAVE | `rank`, `top`, `bottom` |

### 5.6 Statistics

| Need | State | Evidence |
|------|-------|----------|
| Descriptive summary, percentiles, correlation with | HAVE | `summarize`, `percentiles`, `corr_with`, `Nexus::summary/profile` |
| Regression, distributions, time-series models | HAVE (in PlussMa) | `pluss::ma::stats`/regression guide; exposure via DataFrame is **to verify** |
| Statistical output as a *table the UI can show* (coefficients, fit stats) | PARTIAL | results objects exist; a uniform "result to DataFrame" adapter is **to verify** | **PlussMa/DataFrame** if generic; else `BI_tools` |

### 5.7 Charts and interactivity (Flash)

| Need | State | Evidence / gap | Layer for gap |
|------|-------|----------------|---------------|
| The standard chart families | HAVE | ~65 Flash functions (relational, distribution, categorical, regression, matrix, 3D, time-series) | |
| SVG export | HAVE | `Studio::to_svg`, `save_svg` | |
| Mark geometry with source-row ids (hit map) | MISSING | no hit-test/geometry API in `flash.h`/`studio.h` | **Flash**: standalone generic function (section 3) |
| Brush/lasso region to row set | MISSING | needs the hit map | **Flash** (generic) + `BI_tools` (to selection) |
| Hover/tooltip data per mark | MISSING | needs the hit map | **Flash** (generic) |
| Browser-side re-render for zoom/pan | design choice | Q3 | |

### 5.8 Language, selection, session (BI layer)

| Need | State | Layer |
|------|-------|-------|
| Expression language (parse, type-check, evaluate, emit C++) | MISSING | `BI_tools` |
| Lowering target for the language | decision | the language lowers to Series/DataFrame ops (preferred) and, where a table-wide plan is clearer, to the shared planner; it does **not** invent a second execution path |
| Pipeline/step list, undo/replay, show-code | MISSING | `BI_tools` |
| Selection object (click/rule/positional), durable by row key | MISSING | `BI_tools` (uses `Nexus::TableMeta::primary_key`) |
| Session state, cross-filter | PARTIAL | `pluss/bi/session.h` (112 lines) and `Control`/`Filter` exist from the old design; reuse the idea, redo the code |
| Local server | PARTIAL | `serve.h` (384 lines, `POST /api/filter` only); needs a real operation API and push channel |
| Front end | PARTIAL | `frontend/` has a 71-line shell; needs a real app |

### 5.9 What already exists in `pluss::bi` (old design)

About 5.8k lines: `Studio`, `Board`, `Tile`, `View`, `Control`, `QuerySpec`
(group/pivot/resample plans, sort, traverse steps), a Flash-based renderer that
produces self-contained HTML/SVG, persistence (`PersistedStudio`), and the live
`serve.h`. Reuse policy: keep what is a thin view over Nexus (the query plan
shapes, HTML export for `.pbi`), rewrite what was the programmatic authoring
surface. Decide per file during phase 0.

---

## 6. Rules for contributors

1. **UI and code are the same calls.** Each step's `to_code()` emits exactly
   the library calls the engine runs. Add a test per step kind that runs the
   engine path and the generated code and compares results.
2. **Placement rule (section 3) applies to every change.** If you need
   something Flash/Nexus/DataFrame lacks, build it there first as a generic,
   documented feature with its own tests; only then call it from `BI_tools`.
3. **No second execution path.** SQL text, the language, and the UI all lower
   to the same planner/operations.
4. **No silent drops.** Joins, filters and type conversions report what they
   dropped or coerced. The UI must be able to show it.
5. **Throw `pluss::Error` only.**
6. **Public names:** no `std::` throws, method names at most two words, no
   root namespace aliases. The existing project conventions apply.
7. **Performance first.** Nothing in the engine loops over rows in user space
   when a vectorised Series/DataFrame operation exists.
8. **Docs:** every user-visible feature gets a guide page that is a real,
   compiled, output-verified program (project documentation standard).

---

## 7. The expression language (design draft)

Goal: Excel-simple to read, programmatic in power, compiled to Pluss calls.

Principles (agreed):
- Operations are **functions**: `SUM(data[sales])`, `AVG(data[price])`.
- **Conditionals** are functions, nestable: `IF(cond, then, else)`; else-if is
  an `IF` inside an `IF`.
- **Repetition**: `FOR(...)` (see 7.2).
- Variables, user functions, conditionals, repetition are all supported; ops are
  mostly functional.

### 7.1 Semantics to settle before writing a parser

1. **Elementwise vs aggregate.** An expression over columns is elementwise
   (`IF(data[a] > 3, 1, 0)` yields a column); `SUM/AVG/COUNT/...` reduce to a
   value. This matches Series behaviour and compiles to Series ops directly.
2. **`FOR` does not loop over rows.** Row loops are the slow, spreadsheet way
   and defeat vectorisation. Proposed: `FOR(r IN regions, expr)` iterates over a
   list of values, groups or periods and returns a list/table. Row-wise work is
   implicit vectorisation. *Needs maintainer confirmation (Q4).*
3. **Filter context.** Inside a tile, `SUM(data[sales])` respects that tile's
   slicers/selection/cross-filter by default, so a measure is defined once and
   reused everywhere. Explicit override: `SUM(data[sales], ALL(region))`.
   *Needs confirmation (Q5).*
4. **Types and NULL.** NULL, NaN and infinity are distinct (project rule);
   every function documents what it does with each.
5. **Errors** are `pluss::Error` with the formula position; the UI underlines
   the offending part.

### 7.2 Measures

A **measure** is a named expression evaluated under the current filter context
(DAX's idea, kept; syntax ours). A **calculated column** is evaluated once per
row and stored. The UI makes the difference visible (a column adds a field; a
measure adds a metric).

### 7.3 Time intelligence

Menu-driven built-ins that compile to generic Timeline/DataFrame operations:
YTD, QTD, MTD, same period last year, period-over-period change, rolling window.
Built in `Timeline`/`DataFrame` first (section 5.5), not in `BI_tools`.

---

## 8. Selecting cells (design draft)

Excel's selection is positional and ad hoc; DAX's is logical. Support both,
unified: a **Selection** is a first-class value (a set of rows, optionally
restricted to columns) with three ways to make it.

1. **Click in the grid**: click, shift-click, ctrl-click, as in Excel.
2. **A rule**: `data[region == "West" AND sales > 1000]`.
3. **Typed positions**: `data[sales, 2:100]`, `PICK(data[sales], 2, 7, 15)`.

Any selection can be used as a value: `SUM(sel)`, named, saved in the `.pbw`,
and shown in "Show code".

**Durability.** A click-selection is stored as the rows' primary keys when the
table has a key (`TableMeta::primary_key`), so it survives sort, filter and
reload. With no key it is stored as positions in a frozen snapshot, with a
visible warning that it will not follow reloaded data.

**Selection to rule.** After a click-selection, the UI offers to describe it as
a rule when one exists ("these rows all have Category in {A, C, F}"). Nice
bridge; not required for the first version.

**Status bar.** Selecting cells shows Sum/Average/Count immediately, with no
formula (Excel's best trick; cheap and expected).

**Columns are the default unit.** A formula applies to a whole column,
vectorised. A per-row-address mode exists only as an explicit exception, to
avoid a cell-dependency graph.

---

## 9. Build plan

Each phase ends with something a user can run. Gap items from section 5 are
built in their own layer *before* the BI feature that needs them.

**Phase 0: foundations**
- Decide Q1 to Q7 below. Settle file extensions.
- Decide per file what to keep from `pluss::bi` (5.9).
- Create `BI_tools/` skeleton and `pluss bi` launcher (find where the `pluss`
  command is implemented; `pluss kernel install` exists, so a CLI is present).
- Gap work: none yet.

**Phase 1: load, look, clean**
- Local server + browser shell listing the Nexus tables.
- Virtualised data grid with sort/filter; Excel-style selection and status bar.
- Import wizard (CSV/Excel/Parquet/JSON/URL); step list; "Show code".
- Cleaning steps (missing, duplicates, types) with a data-quality panel.
- Gap work: database connector (SQLite first); API source; data-quality
  suggestions in Nexus.

**Phase 2: join, group, charts**
- Relationship/join builder over `Nexus` with key suggestions and join audits.
- Group/aggregate, pivot, rank/top-N steps.
- Charts from Flash; shelf-based chart builder.
- Gap work: **Flash mark geometry function**; join-key suggestion.

**Phase 3: language, calculated columns, measures**
- Parser, evaluator, C++ emitter for the language.
- Calculated columns, conditional logic, measures with filter context.
- Gap work: SQL `CASE`/window functions if SQL stays a lowering target.

**Phase 4: time series**
- Percent change/growth, YTD/same-period-last-year, rolling/cumulative steps.
- Gap work: named time-intelligence ops in Timeline/DataFrame.

**Phase 5: statistics**
- Descriptive stats, regression/forecast steps shown as tables and charts.
- Gap work: uniform result-to-table adapter in PlussMa/DataFrame.

**Phase 6: dashboards and publishing**
- Dashboard layout, slicers, cross-filtering via session state, brushing.
- `.pbi` packaging (ZIP: definition plus Parquet extracts), "strip editing"
  publish option, and the static `.html` web export with its viewer.

**Later:** live `.pbi`, predictive modelling / machine learning.

---

## 10. Open questions

**Proposed defaults (maintainer has not yet explicitly approved; proceed with
these unless told otherwise).**
- Q1: no WebSocket. HTTP POST for operations, Server-Sent Events for pushes.
- Q2: Preact + htm (no build step), W3.CSS, own virtualised data grid,
  SortableJS or native drag events, plain textarea formula box first. All
  vendored under `share/pluss/bi/`.
- Q3: charts are Flash SVG plus a hit-map overlay; zoom/pan re-render through
  the engine (verify latency on localhost).
- Q4: `FOR` iterates over lists, groups, periods or ranges, never rows; no
  `WHILE` initially.
- Q5: filter context automatic in tiles, shown in each tile, with per-tile
  slicer toggles and `ALL`/`ALLEXCEPT` in formulas.
- Q7: named selections reusable across tiles, saved in `.pbw`; rule selections
  re-evaluate, click selections are frozen to row keys.
- Q8: RESOLVED. The CLI is `PlussTils/tools/pluss_cli/main.cpp` (subcommands
  `run`, `new`, `kernel`, `doctor`, `version`). `pluss bi` launches a prebuilt
  `bin/pluss-bi` engine with assets in `share/pluss/bi/`; "Show code" output is
  runnable with `pluss run`.

- **Q1.** WebSocket: add to cpp-httplib (not supported upstream in the vendored
  form) or use a small separate library, or use server-sent events plus POST?
- **Q2.** Front end: which small framework/grid/drag-and-drop libraries? (Prefer
  small, vendored, no build step at install time.)
- **Q3.** Charts: draw in the browser from engine data (hover/zoom) or show Flash
  SVG with the hit-map overlay? Current leaning: Flash SVG plus hit-map overlay,
  because it keeps Flash the single source of plot logic.
- **Q4.** `FOR` iterates over lists/groups/periods, not rows. Confirm.
- **Q5.** Filter context automatic in tiles, DAX-style. Confirm.
- **Q6.** RESOLVED: `.pbw` (JSON working file), `.pbi` (ZIP packaged workbook,
  editable), `.html` web export (view-only).
- **Q7.** Can a saved Selection be named and reused across tiles? (Proposed: yes.)
- **Q8.** Where does the `pluss` CLI live, and how does `pluss bi` get installed
  alongside `pluss kernel install`?
- **Q9.** RESOLVED: use libzip for `.pbi`. It is a *required* dependency of the
  BI build (today it is optional in the loader behind `PLUSSMIC_HAS_LIBZIP`).
- **Q10.** RESOLVED: macOS first. The `.pbw`/`.pbi` file association (double-click
  opens `pluss bi <file>`) is a macOS installer step. Windows and Linux are
  stated in the docs as still under development.
- **Q11.** RESOLVED: no desktop shell. The browser editor is the editor. A native
  WebView or Electron wrapper can be added later without changing it.
- **Q12.** Security baseline: see section 4A (threats T1 to T5). Confirm the
  untrusted-by-default policy for files not created on this machine.

---

## 11. What changes in ARCHITECTURE.md

When this blueprint is accepted, update `ARCHITECTURE.md`:

- Decisions 3, 10, 11, 13 (hybrid programmatic authoring; `Studio/Board/Tile/View`
  naming; fixed v1 scope; staged C++ builder) describe the old surface. Retire or
  rewrite them.
- Decision 4 becomes "browser only; PlussView hosting is removed".
- Decisions 1, 2, 5, 6, 7, 8, 12 (narrow first mission over `Nexus`, `Nexus` as
  workbench, one central table contract, one central planner, session-driven
  interaction, `NexusQuery` is relational planning, one planner for SQL) stand.
