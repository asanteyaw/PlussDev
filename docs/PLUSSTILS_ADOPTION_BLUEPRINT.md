# PlussTils adoption blueprint

Draft, 2026-11-23. Status: planning only; nothing here is built.

Goal: add what makes people choose PlussII, using PlussTils where the work is
utility or workflow, and leaving domain work in its own space. Every item below
was proposed, then accepted, trimmed or moved by the author. Grounding comes
from the API references and headers read on this date; items marked
"to verify" were not run.

## Decisions

| Decision | Item |
|---|---|
| Accepted | `pluss` command-line tool, sample datasets, report builder, settings layering, run manifest, disk cache, interchange formats, viewer interactivity, more PlussBI views |
| Dropped | `df.explore()`; `.show()` on Series, Note and Timeline (their factories already print); progress bars and `tic`/`toc` (use `std::chrono`; revisit only a nested timing tree, because `std::chrono` has none) |
| Moved | Experiment runner (replicate and sweep) belongs in PlussMa Stats, not PlussTils: see "Repositioned" |

## To do

Order follows dependencies. Ids are stable; tick a box when the item is built,
tested on six platforms and documented to the public-docs standard.

- [x] **P0** Shared prerequisites (below): version header, build info, seed derivation. *Built 2026-11-23; version comes from the CMake project version.*
- [x] **T1** `pluss` command-line tool: `run`, `new`, `doctor`. *Built and tested on macOS arm64 only; Linux and Windows paths are untested until CI runs; `doctor` failure in the archive check is a warning for now.*
- [x] **T2** Sample datasets. *Built as `pluss::table::examples()` and `example(name)` in PlussMic (five tables).*
- [ ] **T3** Report builder (one standalone HTML file).
- [x] **T4** Settings layering: defaults, file, environment, arguments. *`pluss::tils::Settings`, JSON files.*
- [ ] **T5** Run manifest.
- [x] **T6** Disk cache for expensive steps. *`pluss::tils::Cache`; DataFrame values are not cached yet.*
- [~] **T7** Interchange formats for arrays. *Phase A done for NPY and NPZ (`pluss::io::save_npy`, `load_npy`, `save_npz`, `load_npz`); MAT v5 and HDF5/NetCDF not started.*
- [ ] **T10** Notebook: run PlussII cells like a Jupyter notebook (see "Notebook" below).
- [ ] **T8** Viewer interactivity: selection and copy first.
- [ ] **T9** More PlussBI views: histogram, box, area.
- [ ] **M1** (PlussMa, not PlussTils) `replicate` and `sweep` in Stats.

Dependencies: T5 needs P0 and T4; T6 needs P0 and T7 or existing `pluss::io`;
T1 needs P0; M1 needs the seed derivation from P0; T2, T3, T8 and T9 stand alone.

## Cross-cutting rules

- Reuse before building: each item names the existing block it extends.
- Throw `pluss::Error` only (`PLUSS_FAIL` / `PLUSS_CHECK`); no `std::` throws.
- Methods have one descriptive word, two at most. No root-level namespace aliases.
- No hardcoded literals or clamps: derive from a formula or one named block, and
  name any unavoidable limit.
- No silent drops: anything truncated says so in its result.
- Public docs: per-method pages with three or more examples, tables instead of
  struct bodies, no internals, no timings or speed claims, no other tools by name.
- Each item ships with tests that run on all six platforms, and a scripted
  check where it has a window.

## P0. Shared prerequisites

1. **Version header.** A generated `pluss/version.h` (major, minor, patch,
   channel, build type, compiler). No version macro was found under
   `PlussQit/include` or `PlussTils/include`. T1 (`doctor`), T5 and T6 need it.
   The staged SDK folder is named `0.1.0` while releases are tagged `0.0.x`;
   decide which number is the product version before generating the header.
2. **Build info.** One function returning operating system, architecture,
   compiler and the compute platforms present (CPU, Apple GPU, CUDA). Qit already
   knows platforms; expose them once and let T1 and T5 call it.
3. **Seed derivation.** `rng_seed.h` has a process-wide base and counter
   (`rng_seed`, `next_rng_seed`). Add a pure function that derives the seed for
   run `i` from a base seed, so results do not depend on thread count or order.
   It lives in PlussTils Core, where the seed stream is; M1 and T5 use it.

## T1. `pluss` command-line tool

**Goal.** Remove the setup wall: C++ that feels like a script.

**Today.** Users write a `CMakeLists.txt` with `find_package(Pluss CONFIG REQUIRED)`
and `Pluss::PlussII` (see `docs/plussii_install.md`). The distribution is staged
with `cmake --install` into a relocatable prefix that already has `bin/`, and
`tools/check_distribution_relocation.py` plus the `sdk_consumer_smoke` target
already test a consumer build against the staged prefix.

**Shape.**

| Command | Does |
|---|---|
| `pluss run file.cpp [args]` | compile with the SDK and run, with a cached build |
| `pluss new name` | scaffold a project: `CMakeLists.txt`, `main.cpp`, a `.gitignore` |
| `pluss doctor` | check the toolchain and print what to fix |

- Installed as `bin/pluss` from the same `install(TARGETS ...)` that stages the SDK.
- Finds the SDK relative to its own path, so the relocation check covers it.
- `run`: a per-user cache folder (platform cache directory) keyed by a hash of
  the source, SDK version (P0) and compiler id; one build folder per key so a
  rebuild is incremental; generates a `CMakeLists.txt` that calls
  `find_package(Pluss CONFIG REQUIRED)`; default build type `RelWithDebInfo`,
  as the install guide recommends for readable traces; `--release` overrides.
- `doctor`: CMake at least 3.28 and a C++20 compiler (the install guide's
  minimums); a one-line program built and run against this SDK; on Linux,
  `zenity` or `kdialog` for Save As; on Windows, the ClangCL toolset and the
  static SQLite triplet the install guide requires; which compute platforms
  are usable.

**Open questions.** Windows needs a Visual Studio environment to compile: does
`run` locate it, or does it print the command to run in a developer prompt?
Multi-file programs: `run dir/` as a project, or only single files at first?

**Verify.** Extend `sdk_consumer_smoke` to `pluss doctor` and `pluss run hello.cpp`
on the six archives.

## T2. Sample datasets

**Goal.** First-run and examples that need no data entry.

**Today.** Loaders (`pluss::loader`) are for batching Series datasets, and
PlussBI already uses DataFrames, so PlussTils can depend on PlussMic.
To verify that dependency direction before choosing the home.

**Shape.** `loader::sample("name")` returns a `DataFrame`; a second call lists the
names. Candidate sets, all with provenance recorded:
- generated and seeded: sales by city and quarter, a sensor stream (kindle), the
  accounts table with NULL, NaN and infinity (the PlussMic guide's example);
- small public-domain sets only. Do not ship anything under a licence that
  restricts redistribution; record each licence beside the data.

Storage: tiny sets compiled in; larger ones as files under the SDK's share folder,
found the same way as T1 finds the SDK.

**Open questions.** Which names? Compiled in or files for the middle size?

## T3. Report builder

**Goal.** Hand someone one file with tables, plots and notes.

**Today.** `Studio::export_html` writes a board; `render_html` in PlussMic's
display layer writes a themed table page; plots save SVG.

**Shape.** `Report r("title"); r.add(frame); r.add(chart); r.note("text"); r.save("report.html");`
one self-contained file (inline style and script, inline SVG), using the table
themes already in the display layer. A table over a size limit is shown as its
first and last rows with a visible "showing N of M" line, never a silent cut.

**Open questions.** Home: PlussBI next to `export_html`, or its own family?
Pagination or truncation for tables of thousands of rows?

## T4. Settings layering

**Goal.** One place for run parameters: defaults, then a file, then the
environment, then arguments.

**Today.** `pluss::tils::env` has `get`, `get_or`, `get_bool`, `get_int`,
`get_double` and clamped variants.

**Shape.** A `Settings` object: `set` defaults, `file`, `env` (prefix), `args`
(from `argc`/`argv`), then typed reads. `origin(key)` says which layer a value
came from, which T5 stores. Strict by default: an unknown key from a layer is an
error. The file format starts as JSON, because PlussMic already loads it.

**Open questions.** Add TOML later? How do list-valued settings read from the
environment?

## T5. Run manifest

**Goal.** One saved record that makes a result reproducible and comparable.

**Shape.** `Manifest m = manifest(settings)` records the version and build info
(P0), `rng_seed()`, every setting with its origin (T4), start time (`Timeline`),
elapsed time (`Duration`) and any user keys. `m.save(path)` writes JSON;
`load(path)` reads it; `diff(a, b)` returns a `DataFrame` of the fields that differ.
Docs describe the record, not its speed.

## T6. Disk cache

**Goal.** Skip an expensive step when its inputs have not changed.

**Today.** `Archive::stamp()` and `Stream::stamp()` return a cheap change marker;
`pluss::io::save` and `load` cover Series, Note, Timeline, TimeIndex and STL
containers (`docs/IO_SAVE_LOAD_GUIDE.md`). DataFrame goes through its own file
savers.

**Shape.** `Cache c(dir); auto v = c.get("key", parts, make);` where `parts` are
explicit key parts (settings, stamps of inputs, the version from P0) and `make`
computes on a miss. No guessing: a changed library version always misses. `clear`
and `stats` for housekeeping. Size limit is an option that is off by default.

**Open questions.** Locking when two processes share a cache. DataFrame values:
which saver round-trips every dtype, including the validity mask?

## T7. Interchange formats

**Goal.** Move arrays to and from colleagues' tools.

**Where.** Series I/O belongs next to `pluss::io` in PlussQit, not in Loaders
(Loaders is batching). 

| Phase | Format | Dependency |
|---|---|---|
| A | NPY (one array) and NPZ (a zip of them) | none new; libzip is already bundled |
| A | MAT v5 numeric arrays | none |
| B | HDF5 or NetCDF | an optional component; decide after A |

Cover every Series dtype, including complex and bool, and both byte orders.

**Open questions.** Public docs avoid naming other tools; NPY and MAT are file
format names that come from them. Name the formats, or call them "array file"
and "workspace file"?

## T8. Viewer interactivity

**Audit, 2026-11-23** (`InHouseHtmlViewer.cpp` and the shells):

| Present | Absent (not found) |
|---|---|
| Search box and Reset; sort triangles; column drag and auto-fit; Theme, View, Format and Save As menus; plot zoom out, Fit and in; wheel scrolling | Cell or row selection; copy to the clipboard; hide or reorder columns; hover values on plots; pan |

**Order.** (1) Selection and copy as tab-separated text: the most useful, and a
paste into a spreadsheet works. (2) Hide and reorder columns from the header.
(3) Plot hover readout and pan.

**Constraints.** The three native shells (Cocoa, Win32, X11) stay in parity; the
clipboard is per platform. Sizes go in the em-based `HtmlTableGeometry` block.
Add scripted commands (like `shot:`) so each behaviour is testable without a
person.

## T9. More PlussBI views

**Today.** `ViewKind` is Kpi, Line, Bar, DetailTable, PivotTable, Scatter,
Heatmap and Corr (`pluss/bi/specs.h`).

**Shape.** Add `histogram`, `box` and `area` constructors and `ViewKind` values.
Histogram bins come from a rule with a documented default and a knob, not a
fixed count. To verify: whether PlussPl `flash` already has the plots, so the
view delegates instead of drawing. Drill-down and tooltips follow T8's pattern.

## Repositioned: M1, `replicate` and `sweep` in PlussMa Stats

**Why PlussMa.** It is simulation methodology. PlussMa already has Monte Carlo
for finance (`MonteCarloOptions`: paths, steps, antithetic, control variates,
seed), stochastic simulators in NatSci, several of which already include
`pluss/tils/kit/rng_seed.h`. The dependency from Ma to Tils exists today, so Ma
can use the seed derivation (P0) without PlussTils owning any statistics.

**Why it was first proposed for PlussTils.** The seed stream lives there, and the
idea was framed as "run any program". The primitive (seed derivation) belongs in
PlussTils Core; the runner does not.

**Shape (sketch).** `stats::replicate(fn, n, options)` runs `fn(seed)` n times
with a derived seed each; `stats::sweep(grid, fn, options)` does the same over a
parameter grid. Both return a `Series` of results plus names, with a summary
(mean, standard error, interval). Ma does not depend on PlussMic, so a
`DataFrame` is one line in user code. Parallel execution must give results independent of thread count; which
existing parallel runtime to use is to verify.

A grid of whole programs across a cluster is different work: that is T4 (settings
from the environment, such as a job index) plus T5 (a manifest per run).

## Notebook (T10)

**Goal.** Cells that keep state, with tables and plots shown inline, for C++.

**What exists.** xeus-cpp, the successor of xeus-cling built on clang-repl, has conda-forge packages for Apple Silicon (checked by search, not installed). clang-repl itself ships with LLVM; Homebrew's LLVM 19 starts here but could not compile the PlussII headers against the Xcode SDK (a standard-library mismatch in that combination), so the engine has not been proven with PlussII yet.

**Shape.** A Jupyter kernel that uses clang-repl as the engine, installed by `pluss kernel install`, so `pluss` stays the one tool. PlussII's own outputs map to Jupyter's display types: a `DataFrame` as HTML (the HTML renderer exists), a plot as SVG (plots already save SVG), a `Series` or `Note` as text. `.show()` inside a notebook shows inline instead of opening a window.

**First step.** A spike: load the PlussII headers and libraries into clang-repl on a matching toolchain, run a few cells, and measure how long the first cell takes (a precompiled header would likely be needed).

## Verification plan

Per item: unit tests; the six-platform CI archive checks; scripted checks for
anything with a window; public docs built and read in the browser before
reporting; no timing claims.

## Open decisions for the author

1. The product version number for P0 (`0.1.0` staged folder versus `0.0.x` tags).
2. T1 on Windows: locate the Visual Studio environment, or print a command.
3. T2: dataset names and the compiled-in versus file split.
4. T3: home (PlussBI or a new family) and the table size rule.
5. T7: file-format names in public docs.
6. Whether M1 is wanted now or later.
