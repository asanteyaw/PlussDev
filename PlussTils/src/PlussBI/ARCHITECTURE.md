# PlussBI Architecture

## Mission

PlussBI should become the programmatic dashboard layer for the Pluss stack.
It is not a loose collection of plotting helpers. It should be a coherent
system that lets users:

- register one or more tables inside `pluss::bundle::Nexus`
- declare relationships, metadata, and reusable join plans inside that
  `pluss::bundle::Nexus` workbench
- describe dashboards in C++
- bind visuals and controls to shared state
- render as a real, shareable browser dashboard by default
- fall back to a native PlussView window for offline/embedded hosts without
  rewriting the dashboard

The system should centralize common behavior aggressively so future growth does
not become maintenance debt.

## Locked Product Decisions

The following product decisions are intentionally locked for the current BI
direction and should be treated as foundation assumptions rather than open
questions.

1. PlussBI's first mission is narrow and concrete.
   PlussBI starts as a dashboard builder over `Nexus`, `DataFrame`, and Flash,
   not as a vague all-purpose analytics application framework. Broader app-like
   workflows may come later, but the initial architecture should optimize for a
   strong dashboard system first.

2. `Nexus` is not just a passive source bag; it is the BI workbench.
   In the BI context, `pluss::bundle::Nexus` is the coordinated bundle-layer
   workspace where named tables, derived views, relationship knowledge,
   reusable plans, and multi-table query flows live. This is the analytical
   workbench root. PlussBI should build over that workbench rather than invent
   a parallel source registry.

3. Authoring should be hybrid.
   The implementation should centralize plain-data specs underneath, while the
   downstream authoring surface remains ergonomic. In practice that means spec
   structs for maintainability and builder-style convenience on top for user
   code.

4. Runtime priority is browser-first.
   The first-class runtime is a real OS browser tab: `Studio::show()` writes
   the same self-contained dashboard HTML/SVG artifact `export_html()`
   produces and opens it with the system default browser
   (`pluss::bi::detail::open_html_in_browser`). This
   revises the original native-first framing: a dashboard that only opens in
   a blocking, unshare-able native window cannot rival a real BI tool, and
   PlussBI already generates browser-native HTML/SVG for every render, so
   there is no separate "browser export" artifact to build later — the
   default runtime and the export artifact are the same document. PlussView
   (native `WebKit`/`WebView2`/GTK-WebKit hosting, or the in-house
   software-rendered fallback) remains available as a second host for
   offline, embedded, or sandboxed environments where launching a system
   browser process is undesirable or unavailable — request it explicitly via
   `HtmlWindowOptions::backend_preference` (`NativeWebView`, `PlussWebView`,
   or `PlussWebViewOnly`); it is not being removed.

5. BI should normalize inputs to one central table contract.
   Low-level Flash may still admit `Slate`, `DataFrame`, `Atlas`, and other
   raw payload families directly. PlussBI itself should not branch wildly on
   payload family. It should resolve everything into one central table/query
   result contract, then bind visuals from that normalized result.

6. Querying must be centralized.
   Visuals should not own private filtering/grouping/pivot/join logic. One
   shared planner should turn query specs plus session state into resolved
   tables. If this discipline is lost early, the BI layer will become
   unmaintainable very quickly.

7. Interaction is session/state driven only.
   Cross-filtering, brushing, slicers, parameter changes, and selection state
   should all flow through shared session state. Visual-to-visual direct wiring
   should not become part of the public model.

8. `NexusQuery` is relational planning, not raw SQL.
   Today, `Nexus` exposes two distinct query faces:
   `NexusTraverse`, returned by
   `Nexus::from(...).join(...).via(...).pick(...).collect()`, and
   `NexusQuery`, returned by `Nexus::query(...).execute(...)`.
   `NexusTraverse` is the fluent workbench-native planning path; `NexusQuery`
   is the SQL-facing workspace path. Raw SQL parsing is still delegated through
   the shared lower-level `DataFrame::sql()` engine underneath, but the user
   entry point at this layer is the workbench-wide `Nexus::query()` surface.

9. Layout starts as responsive grid only.
   The first BI layout model should be rows, columns, spans, and responsive
   breakpoint behavior. Freeform canvas can come later, but it should not
   complicate the first usable product.

10. The public BI naming family is `Studio`, `Board`, `Tile`, `View`,
    `Control`, and `Session`.
    `Portfolio` is too domain-specific for the root object, and `Visual` is
    broader than needed for the public surface. `Studio` better conveys the
    orchestrating root while `View` cleanly describes a rendered analytic
    surface.

11. The first usable BI release has a fixed scope.
    The first release should cover one `Studio`, one or more `Board`s
    architecturally, responsive grid layout, a centralized query planner over
    `Nexus`, KPI cards, line views, bar views, pivot-table views, detail-table
    views, categorical filters, numeric/date range filters, session-driven
    cross-filtering, and native rendering in PlussView.

12. Future SQL over `Nexus` must target the same planner.
    If a SQL surface is later added for `Nexus`, it should compile or translate
    into the same shared `QuerySpec` / join-plan machinery used by the fluent
    API. PlussBI should not grow one execution path for `NexusQuery` and a
    second unrelated execution path for SQL text.

13. The primary public BI authoring style is staged, not one giant chain.
    The locked downstream shape is a staged C++ object-builder flow:
    create a `Studio`, bind a `Nexus`, request a `Board`, configure that board
    in place, build controls and views as named local objects, register them
    explicitly, then render. This keeps small examples clean while staying
    readable and maintainable for larger production dashboards.

14. Ergonomic sugar is deferred until the staged surface is proven stable.
    Shorter convenience forms can come later, but current implementation work
    should optimize first for correctness, scalability, testability, and clear
    ownership boundaries on the approved staged public surface.

## Open Product Decisions

The following still need deliberate discussion before implementation grows
around them:

- ~~how much browser export should be present in the first usable release~~
  — resolved: browser hosting is the default runtime (decision #4).
- ~~how much *live, in-browser* interactivity belongs in this phase~~ —
  resolved: `Studio::serve()` (Phase 2, `include/pluss/bi/serve.h`) now
  embeds a real HTTP server so filter controls re-query and redraw via
  `POST /api/filter` without a new C++-side call or tab reopen. `Studio::
  show()`/`export_html()` remain the static-snapshot path, deliberately
  unchanged — `serve()` is the explicit opt-in for a live session, not a
  new default, since it blocks the calling thread until interrupted.
- whether a SQL front-end over `Nexus` should be implemented immediately after
  the fluent planner matures, or deferred until the planner and dashboard
  runtime are hardened

## Design Principles

1. `Nexus` is the data root.
   Every dashboard query should resolve against a named source inside a
   `pluss::bundle::Nexus`, or against a derived view that is itself registered
   back into the same workbench graph.

2. One query planner, not many ad hoc helpers.
   Filtering, grouping, pivoting, sorting, windowing, and reshaping should be
   described once in a query spec and executed by one planner. Visuals should
   consume the planner, not reimplement table logic privately.

3. One shared interaction state.
   Cross-filtering, brushing, slicers, and dashboard parameters should all live
   in a single session state model so every visual sees the same selection and
   filter context.

4. Flash stays the rendering engine.
   PlussBI should compose Flash plots and Flash scenes rather than becoming a
   second plotting library. The BI layer adds orchestration, layout, binding,
   caching, and interaction.

5. Runtime separation matters.
   Data logic, interaction state, layout, rendering, and transport should be
   distinct layers so PlussView hosting and browser hosting can share the same
   dashboard definition.

6. The `Nexus` workbench is the coordination center.
   BI should treat `pluss::bundle::Nexus` as the orchestrated multi-table
   workspace and query coordinator. The dashboard layer should ask that
   workbench for resolved tables through `NexusTraverse`, `NexusQuery`,
   materialized join plans, and registered derived views; it should not clone
   relationship planning or source coordination privately.

7. Large dashboards must remain readable as code.
   The public surface should favor named intermediate objects and staged
   registration over a single endlessly chained expression. Fluent sub-builders
   on a control or view are acceptable, but the top-level dashboard assembly
   should stay readable, inspectable, and scalable.

## `Nexus` As Bundle Workbench Root

`pluss::bundle::Nexus` is not merely "where the tables happen to sit." It is
the bundle-layer root object that coordinates analytical table work for BI.

PlussBI should explicitly rely on these existing workbench responsibilities:

- named table registration
- relationship declaration and validation
- schema and graph inspection
- shortest-path and explicit join planning
- stored/materializable join plans
- fluent multi-table traversal via `NexusTraverse`
- SQL-style workspace queries via `NexusQuery`

This matters because BI should not privately recreate any of that. The BI layer
should add dashboard semantics, not a shadow workbench.

## Locked Public Authoring Contract

The approved downstream authoring contract for the current BI implementation
track is:

```cpp
using namespace pluss::bi;

Studio studio("credit");
studio.source(nexus);

auto& board = studio.board("overview");
board.title("Credit Overview");
board.grid(12, 8);

auto segment = category("segment");
segment.feature("segment").label("Segment").place(0, 0, 3, 1);
board.control(segment);

auto principal = range("principal");
principal.feature("principal").label("Principal").place(3, 0, 4, 1);
board.control(principal);

auto risk = bar("risk_mix");
risk.query(from("loans")).x("risk_band").title("Risk Mix").place(0, 1, 6, 3);
board.view(risk);

studio.show();
```

This shape is intentionally locked for the next BI phases.

Why this is the primary public style:

- It scales from small demos to enterprise dashboards without becoming a
  visually noisy mega-chain.
- Controls and views can be named, reused, configured conditionally, tested,
  and passed around before registration.
- `Studio`, `Board`, `Control`, and `View` each keep a clear ownership role.
- Shared spec structs can still power the implementation underneath without
  forcing that internal shape directly onto users.

What is intentionally not the primary public style right now:

- `Studio(...).source(...).board(...).control(...).view(...).show()` as the
  main downstream authoring pattern
- sugar added purely for terseness before the stable staged API is complete
- `add(...)`-style registration that obscures whether the board received a
  control, a view, or some other tile kind

The implementation may still use builder semantics on each object, but the
root composition model should stay staged: acquire object, configure object,
register object, render studio.

## Proposed Object Model

### `Studio`

Root dashboard object.

Responsibilities:

- owns a `pluss::bundle::Nexus` reference or handle
- owns one or more `Board`s
- owns shared `Session` state
- coordinates refresh, render, export, and serialization

Suggested shape:

```cpp
namespace pluss::bi {

class Studio {
public:
  explicit Studio(std::string id);

  Studio& source(const pluss::bundle::Nexus& nexus);
  Board& board(std::string id);
  Studio& theme(ThemeSpec spec);
  Studio& refresh();
  Studio& show();
  Studio& save(std::string_view path);
};

} // namespace pluss::bi
```

Notes:

- `Studio` is the root coordinator, not a passive list of dashboards.
- `board(...)` should return a stable board reference so downstream code can
  configure a board incrementally over many lines.
- The current public contract should favor `board(id)` over `add(Board{...})`
  because staged assembly is the locked scalable authoring style.

### `Board`

A page, tab, or canvas inside a studio.

Responsibilities:

- owns layout metadata
- owns tiles/cards
- can be refreshed independently
- accepts explicit `control(...)` and `view(...)` registration

Suggested downstream shape:

```cpp
auto& board = studio.board("overview");
board.title("Credit Overview");
board.grid(12, 8);

board.control(segment);
board.view(risk);
```

Notes:

- `Board` is the stable assembly surface for one dashboard page.
- Explicit `control(...)` and `view(...)` registration is preferred over one
  undifferentiated `add(...)` because BI needs to preserve the semantic
  distinction between inputs and rendered analytic surfaces.

### `Tile`

The smallest dashboard placement unit.

Responsibilities:

- holds one `View` or one `Control`
- stores layout coordinates and sizing rules
- exposes title/subtitle/footnote chrome

`Tile` is primarily the internal placement abstraction that `Board` manages.
Common downstream code should usually configure placement through `.place(...)`
on controls and views rather than constructing `Tile` directly.

### `View`

Declarative plot binding.

Responsibilities:

- references a named workbench source, stored plan, or derived result inside
  `pluss::bundle::Nexus`
- carries a `QuerySpec`
- carries a `BindSpec`
- resolves to a Flash chart or scene

Examples:

- line visual
- bar visual
- heatmap visual
- pivot table visual
- detail table visual
- KPI card visual
- scatter matrix visual

Suggested downstream shape:

```cpp
auto risk = bar("risk_mix");
risk.query(from("loans"))
    .x("risk_band")
    .title("Risk Mix")
    .place(0, 1, 6, 3);

board.view(risk);
```

### `Control`

Declarative interaction input.

Responsibilities:

- binds to one or more source features
- writes into shared session state
- supports category filters, ranges, search, date windows, and toggles

Suggested downstream shape:

```cpp
auto segment = category("segment");
segment.feature("segment")
       .label("Segment")
       .place(0, 0, 3, 1);

board.control(segment);
```

### `Filter`

Serializable filter clause.

Responsibilities:

- feature path
- operator
- literal/set/range payload
- optional scope

### `Session`

Central runtime state.

Responsibilities:

- current filter set
- current parameter values
- current cross-highlight selection
- cached query results
- render invalidation bookkeeping

## Centralized Specs

These specs should be plain data carriers and should stay reusable across
Studio, Board, View, and export layers.

### `QuerySpec`

Describes how a view derives its table.

Fields should eventually cover:

- source key in `pluss::bundle::Nexus`
- optional stored-plan key in `Nexus`
- optional traversal/program shape when the source is derived fluently
- selected features
- filter references
- local filters
- sort spec
- group spec
- pivot spec
- window spec
- limit/head/tail
- reshape spec

### `BindSpec`

Describes how a resolved table maps into a Flash visual.

Fields should cover:

- visual kind
- feature bindings such as x/y/z/color/size/text/facet
- explicit aggregation when needed
- plot options payload
- null handling policy

### `LayoutSpec`

Describes grid and responsive placement.

Fields should cover:

- row / column placement
- span
- breakpoint-aware resize behavior
- min/max tile constraints
- optional collapsible sections or panes

## SQL Over `Nexus`

SQL support over `Nexus` should be treated as a likely future surface, not as
the first implementation center of gravity.

That means:

- the fluent workbench planner remains the semantic foundation
- `NexusTraverse` remains the primary native multi-table planning path
- `NexusQuery` remains the SQL-facing workspace path
- BI query execution should be implemented once in shared planner machinery
- any future SQL parser or SQL-like surface should translate into that same
  machinery rather than bypassing it

This keeps `pluss::bundle::Nexus` closer to an in-memory analytical workbench
than to a database server clone. The goal is not to recreate MySQL Workbench
wholesale. The goal is to make multi-table in-memory work principled,
inspectable, reusable, and eventually SQL-addressable without splitting the
engine in two.

### `ThemeSpec`

Shared typography, palette, spacing, surface, and chrome settings.

### `ExportSpec`

Saved artifact format and runtime mode.

Fields should cover:

- native PlussView package
- static browser bundle
- embedded-data bundle
- external-data bundle

## Data Flow

The intended dashboard pipeline should be:

1. `Studio` references a `pluss::bundle::Nexus`
2. `Board`s are requested from `Studio` and configured in place
3. each `View` names a source, stored plan, or derived query path inside that
   `Nexus`
4. `Session` contributes active shared filters and parameters
5. `QuerySpec` resolves to a `DataFrame`
6. `BindSpec` converts the frame into the correct Flash plot call
7. the runtime host renders the result and wires interaction events back into
   `Session`

That means Flash now benefiting from direct `DataFrame` plot overloads is not a
nice-to-have; it is foundational for PlussBI. The BI layer should be able to
hand a resolved table straight into Flash without detouring through manual
payload extraction everywhere.

The query resolution step should be able to lower into one of three already
native `Nexus` modes:

- fluent traversal through `NexusTraverse`
- stored join-plan execution through `run(...)` / `materialize(...)`
- SQL-style workbench queries through `NexusQuery`

## Why `Dataset` Still Matters

Direct frame plotting should be the common BI path.

`flash::Dataset` is still useful as a lower-level staging object when:

- a visual wants a plot-native reusable bundle
- a dataset is assembled from mixed table and non-table sources
- a render pipeline wants to cache a plotting payload after query resolution

So the BI stack should prefer:

- `DataFrame -> Flash` for ordinary dashboard rendering
- `DataFrame -> flash::Dataset -> Flash` only when plot-payload staging adds
  value

## Rendering Layers

### Browser Host (primary)

First host is a real OS browser tab, implemented in `include/pluss/bi/detail/` +
`src/PlussBI/runtime/BrowserExport.cpp`: `Studio::show()` renders the
studio, builds the same dashboard HTML/SVG document `export_html()` writes
to disk, and hands that document straight to the system default browser
(`open` on macOS, `xdg-open` on Linux, `ShellExecute` on Windows) — no
window toolkit, no browser-engine framework linkage, no modal event loop.

Needed pieces, in rough build order:

- ✅ static dashboard HTML/SVG generation (`detail::dashboard_html_`)
- ✅ cross-platform "open a real browser tab" launcher (`pluss::bi::detail`)
- ✅ dashboard chrome, selectable per render via `DashboardChrome` (`show()`/
  `export_html()`/`serve()`'s trailing `chrome` parameter, default
  `Sidebar`): `Sidebar` is a fixed, non-scrolling top navbar (`.bi-topbar`)
  and a fixed, non-scrolling left sidebar (`.bi-sidebar`, one tab per
  `Board`), matching how this repo's own Sphinx API-reference site renders
  — only `.bi-content` scrolls, and the sidebar collapses to a horizontal
  strip under a responsive breakpoint. `None` is PlussBI's original plain
  scrolling single-column document (boards simply stack, nothing pinned)
  for dashboards that don't want navigation chrome at all — a small
  embedded report, or a single-board view.
- ✅ an embedded HTTP + JSON query API (`Studio::serve()`,
  `include/pluss/bi/serve.h`, vendored `third_party/httplib`) so a filter
  control re-queries the live `Studio`/`Session` via `POST /api/filter`
  instead of requiring a new C++-side `show()` call — guarded by a
  `std::mutex` since httplib dispatches on multiple threads
- ✅ client-side JS (`plussBiApplyFilter`/`plussBiShowBoard`) that calls
  that API via `fetch()` and swaps `#bi-content`/`#bi-session-chips` in
  place, replacing the old inert `plussBiSyncSelect`/`plussBiSyncRange`
  chip-label stubs; falls back to a "Static preview" status label when no
  live server answers (e.g. a plain `export_html()` file opened directly)

`serve.h` is deliberately **not** included by `pluss/bi/bi.h` (and so not by
the umbrella `pluss/tils.h`): it is the only place that needs the vendored
`httplib.h`, and most `Studio` consumers never call `serve()`. Only code
that actually starts a live server includes `pluss/bi/serve.h` directly
(see `PlussMic/examples/table/bi_demo_suite.cpp`'s `serve` mode).

**The dashboard's CSS/JS/HTML shell are real, modular source files, not
C++ string literals.** `detail::dashboard_html_` used to build the whole
document (including a ~150-line CSS block and a ~50-line JS block) inline
inside a C++ `ostringstream`, which is unpleasant to edit and gets no
CSS/JS tooling. They now live as genuine files under
`src/PlussBI/frontend/` — `static/css/dashboard.css`, `js/dashboard.js`,
`html/dashboard_shell.html` — and are embedded byte-for-byte into a
generated header (`include/pluss/bi/dashboard_assets.h`, gitignored) at
CMake configure time (`src/PlussBI/CMakeLists.txt`, `CMAKE_CONFIGURE_DEPENDS`
watches the three source files so editing one triggers regeneration). The
handful of values that legitimately vary per render (max dashboard width, a
plot's minimum height) are threaded through as CSS custom properties set
inline on the root wrapper, never by rewriting the stylesheet's text, so
the embedded CSS stays byte-identical regardless of render parameters.
`dashboard_html_` assembles the final document with a single-pass
`{{PLUSSBI_TITLE}}`/`{{PLUSSBI_STYLE_TAG}}`/`{{PLUSSBI_SCRIPT_TAG}}`/
`{{PLUSSBI_BODY}}` token substitution into the shell template — no template
engine, no recursive substitution risk (each token appears exactly once).
`show()`/`export_html()` inline the CSS/JS (`inline_assets=true`) to keep
the exported document a single portable file; `serve()` instead serves them
as genuinely separate, cacheable `GET /assets/dashboard.css`/`.js` routes
(`inline_assets=false`), the more idiomatic modular-web-app shape.

### Native Host (fallback)

PlussView (native `WebKit`/`WebView2`/GTK-WebKit, or the in-house
software-rendered fallback) hosts the identical dashboard document for
offline, embedded, or sandboxed environments where launching a system
browser process is undesirable or unavailable. It is requested explicitly
via `HtmlWindowOptions::backend_preference` and otherwise stays out of the
default path.

Needed pieces:

- a board container
- tile layout engine
- event bridge from frontend controls to C++ session state
- render refresh hook for individual visuals and full board refreshes

Both hosts render the exact same generated document — there is one
dashboard artifact, not a native definition and a separate browser export.

### Live Server Hardening (Phase 3)

Phase 2 shipped the live server; Phase 3 made it safe to actually run
unattended and gave it real automated coverage (previously `serve()` had
none — only manual browser verification). Landed:

- ✅ **`ServeHandle` / `Studio::start_serving()`.** `serve()`'s own doc
  comment used to promise a "returned handle" that didn't exist — `serve()`
  returned `void` and blocked forever, stoppable only by killing the
  process. `start_serving()` is now the real non-blocking primitive: it
  returns a move-only `ServeHandle` immediately (`.stop()` from any thread,
  `.wait()` to block, `.running()`/`.port()`/`.url()` to inspect), and
  `serve()` is a two-line wrapper (`start_serving(...).wait()`) kept purely
  for the common blocking-CLI-call case. A handle's destructor stops and
  joins automatically, so a `ServeHandle` going out of scope can never leak
  a background thread or a bound socket. Every request handler now captures
  a heap-allocated, `shared_ptr`-owned `detail::ServeContext` by value
  (never `[&]` over `start_serving()`'s own stack frame, which is gone by
  the time the caller gets the handle back) — the earlier Phase 2 draft of
  this function captured stack locals by reference and would have been a
  use-after-free the moment it was made non-blocking.
- ✅ **A 1 MiB cap on `/api/filter` request bodies**
  (`detail::kMaxFilterPayloadBytes`, via httplib's `set_payload_max_length`)
  — the wire format is a handful of short string fields, so anything
  actually needing more is hostile or broken, not a real client. httplib
  answers an oversized body with `413 Payload Too Large` rather than
  dropping the connection.
- ✅ **Audited input handling** end to end: filter values round-trip
  through the existing `escape_html_` calls already present at every render
  site (confirmed safe, not a gap); every `PLUSS_CHECK`/`PLUSS_FAIL` in the
  request path throws `pluss::Error : std::runtime_error`, caught by
  `POST /api/filter`'s existing `catch (const std::exception&)`; malformed
  JSON, an unknown control id, and a filter kind that doesn't match the
  target control's `ControlKind` all resolve to a clean `400`, verified
  by a real HTTP client hitting a real running server, not just as static
  code review. The one real gap the audit found — an unparseable temporal
  bound (`pluss::make_timeline` on garbage text) wasn't wrapped in a
  try/catch — is fixed in `apply_filter_payload_`.
- ✅ **A real automated test suite**
  (`PlussMic/test/pluss_bi_serve_test.cpp`, target
  `plussmic_bi_serve_tests`), using `httplib::Client` against a real
  `start_serving()`-started server on an ephemeral port: `GET /` in both
  chrome modes, `GET /assets/dashboard.{css,js}` byte-matching the embedded
  constants, `POST /api/filter` for all three control kinds (category,
  numeric-range, temporal-range) actually mutating `Studio` state (verified
  via a follow-up `GET /`, not just the response echo), every adversarial
  payload above returning 400/413 without crashing the server, a 12-way
  concurrent-request smoke test, and both `ServeHandle` shutdown paths
  (explicit `.stop()`+`.wait()`, and destructor-on-scope-exit) actually
  closing the socket (a follow-up connection attempt fails to connect).

Deliberately **out of scope**: authentication, CORS, non-localhost binding,
rate limiting. None of that is warranted for a dev-tool that only ever
binds to `127.0.0.1`; adding it now would be hardening against a threat
model this server doesn't have.

## Caching and Performance

The system should centralize performance-sensitive behavior early.

Required shared machinery:

- query-result cache keyed by `QuerySpec + Session state`
- visual render cache keyed by resolved data fingerprint + plot options
- incremental invalidation so one control change does not rebuild everything
- explicit dependency graph between controls and visuals

The dashboard layer should delegate vectorized work downward into:

- `DataFrame`
- `Slate`
- `Series`
- `Note`
- Flash

It should avoid reintroducing row loops at the BI layer whenever a lower layer
already has a bulk path.

## Recommended First Buildable Tranche

The first real implementation tranche should be:

1. `Studio`
2. `Board`
3. `Tile`
4. `Session`
5. `QuerySpec`
6. `BindSpec`
7. one native render path for:
   - KPI card
   - bar chart
   - line chart
   - pivot table view
8. one working cross-filter control

That produces a real usable BI seed instead of a large but inert type graph.

## Execution Roadmap

For implementation tracking, PlussBI should be treated as a 9-phase program.

1. Core public surface.
   `Studio`, `Board`, `Tile`, `View`, `Control`, `Filter`, `Session`,
   `QuerySpec`, and `BindSpec` exist as usable public types.
   The staged public authoring contract is the reference surface.

2. First executable seed.
   The shared planner resolves source, stored-plan, SQL, and traverse queries;
   KPI, line, bar, pivot-table, and detail-table views render; session-driven
   filters work end to end under tests.

3. Planner ordering and control materialization.
   The planner gains explicit query ordering, and control tiles appear in
   render output with their active shared-state bindings exposed.

4. Native host board runtime.
   PlussView-native board composition, board-level refresh, and dashboard
   chrome become real instead of each plot tile returning an isolated studio.
   This phase should make the locked `Studio` / `Board` / `control(...)` /
   `view(...)` staged downstream contract fully real.

5. Shared cache and invalidation graph.
   Query-result caching, render caching, and dependency-aware invalidation are
   introduced centrally.

6. Richer query planner.
   Query-level derived table planning expands inside the same shared resolver.
   This phase should make patterns like the following first-class:

   ```cpp
   auto risk = bar("risk_mix");
   risk.query(
       from("loans")
           .group({"risk_band"})
           .agg({
               {"loan_count", "", pluss::table::GroupOp::Count},
               {"principal_sum", "principal", pluss::table::GroupOp::Sum},
           })
           .arrange("loan_count", false))
       .x("risk_band")
       .y("loan_count");

   auto principal = line("principal_curve");
   principal.query(
       from("loans")
           .resample("opened_at", {"principal"}, "M",
                     pluss::subsp::stats::ts::ResampleOp::Sum))
       .x("opened_at")
       .y("principal");

   auto pivoted = table("principal_pivot");
   pivoted.query(
       from("loans")
           .pivot({"segment"}, {"risk_band"}, {"principal"},
                  pluss::table::GroupOp::Mean));
   ```

   The important rule is that BI still delegates grouping, pivoting, and
   temporal aggregation into the existing `DataFrame` and time-series kernels
   rather than growing a second compute engine.

7. Richer visual surface.
   The next visual tranche should add:
   - scatter views
   - heatmap views
   - richer table presentation and cell styling
   - higher-order analytic views such as distribution and pairwise summaries

   The public shape should remain:

   ```cpp
   auto scatter_view = scatter("principal_vs_payment");
   scatter_view.query(from("loans")).x("principal").y("payment");
   board.view(scatter_view);
   ```

8. Export and serialization.
   One studio definition should become persistable and exportable through:
   - native snapshots
   - packaged browser artifacts
   - saved board/studio definitions
   - restorable session state

   Example target shape:

   ```cpp
   studio.save("credit.plussbi");
   studio.export_html("credit_board");
   studio.export_png("credit_board.png");
   ```

9. Hardening pass.
   Before BI is treated as stable, the final pass should land:
   - broader Catch2 coverage across planner, controls, visuals, and exports
   - stricter `pluss::Error` guards on malformed bindings and invalid layouts
   - performance audits on query reuse, derived-table caching, and redraw costs
   - API tightening to remove any redundant public surfaces discovered in
     phases 1 through 8

## Immediate Implementation Split

Suggested file grouping:

- `src/PlussBI/core/`
  - `Studio.*`
  - `Board.*`
  - `Tile.*`
  - `Session.*`
- `src/PlussBI/query/`
  - `QuerySpec.*`
  - `Planner.*`
  - `Cache.*`
- `src/PlussBI/bind/`
  - `BindSpec.*`
  - `ViewFactory.*`
- `src/PlussBI/runtime/`
  - `NativeHost.*`
  - `BrowserExport.*` — ✅ implemented: cross-platform browser launcher, built
    by `src/PlussBI/CMakeLists.txt` as the real `PlussBI` static library
    target (previously this whole directory was an unwired stub)
- `src/PlussBI/frontend/`
  - ✅ implemented: `static/css/dashboard.css`, `js/dashboard.js`,
    `html/dashboard_shell.html` — the dashboard's real, hand-edited
    CSS/JS/HTML source, embedded into `include/pluss/bi/dashboard_assets.h`
    at CMake configure time (see "Rendering Layers" above; previously this
    whole directory was an unwired, empty stub)
- `src/PlussBI/backend/`
  - ✅ implemented as header-only instead of this directory: the live HTTP
    + JSON query API from "Rendering Layers" above is `Studio::serve()` in
    `include/pluss/bi/serve.h`, matching the header-only style of the rest
    of `include/pluss/bi/`. `include/pluss/bi/detail/json_lite.h` (a deliberately
    minimal flat-JSON encode/decode, not a general JSON library) and the
    vendored `third_party/httplib/httplib.h` (cpp-httplib, MIT) back it.

Public headers should eventually mirror that shape under `include/pluss/bi/`.
`BrowserExport`'s internal header lives under `include/pluss/bi/detail/` instead,
per `explore.h`'s original split: `include/pluss/bi/` is the core dashboard
object model (`Studio`/`Board`/`View`/...), `include/pluss/bi/detail/` is
supporting infrastructure the core model calls into but that isn't itself a
dashboard authoring type.

## Non-Goals For The First Pass

These should not block the first tranche:

- drag-and-drop authoring
- spreadsheet formula parity
- full browser collaboration
- permissioned multi-user editing
- pixel-perfect parity with mature BI suites

The first pass should aim for a strong programmatic dashboard core with clean
layering and cross-filter semantics.
