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
- render natively in PlussView first
- export a browser-ready artifact later without rewriting the dashboard

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

4. Runtime priority is native-first.
   The first-class runtime is PlussView. Browser export matters, but it is a
   second host for the same dashboard definition, not a co-equal design center
   from day one.

5. BI should normalize inputs to one central table contract.
   Low-level Flash may still admit `Thread`, `DataFrame`, `Atlas`, and other
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

- how much browser export should be present in the first usable release
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

### Native Host

First host should be PlussView.

Needed pieces:

- a board container
- tile layout engine
- event bridge from frontend controls to C++ session state
- render refresh hook for individual visuals and full board refreshes

### Browser Export

Later host should emit browser-ready assets from the same studio definition.

This should reuse:

- the same serialized layout
- the same serialized session defaults
- the same query and binding specs

It should not require rewriting the dashboard in HTML or JavaScript by hand.

## Caching and Performance

The system should centralize performance-sensitive behavior early.

Required shared machinery:

- query-result cache keyed by `QuerySpec + Session state`
- visual render cache keyed by resolved data fingerprint + plot options
- incremental invalidation so one control change does not rebuild everything
- explicit dependency graph between controls and visuals

The dashboard layer should delegate vectorized work downward into:

- `DataFrame`
- `Thread`
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
  - `BrowserExport.*`
- `src/PlussBI/frontend/`
  - static host assets only
- `src/PlussBI/backend/`
  - transport / session plumbing only

Public headers should eventually mirror that shape under `include/pluss/bi/`.

## Non-Goals For The First Pass

These should not block the first tranche:

- drag-and-drop authoring
- spreadsheet formula parity
- full browser collaboration
- permissioned multi-user editing
- pixel-perfect parity with mature BI suites

The first pass should aim for a strong programmatic dashboard core with clean
layering and cross-filter semantics.
