# PlussBI Architecture

## Mission

PlussBI should become the programmatic dashboard layer for the Pluss stack.
It is not a loose collection of plotting helpers. It should be a coherent
system that lets users:

- register one or more tables inside `pluss::table::Nexus`
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
   In the BI context, `Nexus` is the coordinated table workspace where named
   tables, derived views, relationship knowledge, and multi-table query flows
   live. This is the closest table-layer analogue to a workbench/workspace
   coordinator. PlussBI should build over that workbench rather than invent a
   parallel source registry.

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
   Today, the fluent `Nexus::from(...).join(...).via(...).pick(...).collect()`
   surface is the native multi-table query builder over the `Nexus` workbench.
   Raw SQL currently lives on `DataFrame::sql()` instead. Optional SQL over
   multi-table `Nexus` workspaces can be added later as a front-end, but it is
   not the present foundation and should not be confused with `NexusQuery`.

## Open Product Decisions

The following still need deliberate discussion before implementation grows
around them:

- layout model from point 7 onward
- public BI naming family
- exact v1 feature slice
- how much browser export should be present in the first usable release

## Design Principles

1. `Nexus` is the data root.
   Every dashboard query should resolve against a named source inside a
   `Nexus`, or against a derived view that is itself registered back into the
   same session graph.

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
   BI should treat `Nexus` as the orchestrated multi-table workspace and query
   coordinator. The dashboard layer should ask that workbench for resolved
   tables; it should not clone relationship planning or source coordination
   privately.

## Proposed Object Model

### `Portfolio`

Root dashboard object.

Responsibilities:

- owns a `Nexus` reference or handle
- owns one or more `Board`s
- owns shared `Session` state
- coordinates refresh, render, export, and serialization

Suggested shape:

```cpp
namespace pluss::bi {

class Portfolio {
public:
  Portfolio& source(const pluss::table::Nexus& nexus);
  Portfolio& add(Board board);
  Portfolio& theme(ThemeSpec spec);
  Portfolio& refresh();
  Portfolio& show();
  Portfolio& save(std::string_view path);
};

} // namespace pluss::bi
```

### `Board`

A page, tab, or canvas inside a portfolio.

Responsibilities:

- owns layout metadata
- owns tiles/cards
- can be refreshed independently

### `Tile`

The smallest dashboard placement unit.

Responsibilities:

- holds one `Visual` or one `Control`
- stores layout coordinates and sizing rules
- exposes title/subtitle/footnote chrome

### `Visual`

Declarative plot binding.

Responsibilities:

- references a source table name in `Nexus`
- carries a `QuerySpec`
- carries a `BindSpec`
- resolves to a Flash chart or scene

Examples:

- line visual
- bar visual
- heatmap visual
- pivot table visual
- KPI card visual
- scatter matrix visual

### `Control`

Declarative interaction input.

Responsibilities:

- binds to one or more source features
- writes into shared session state
- supports category filters, ranges, search, date windows, and toggles

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
Portfolio, Board, Visual, and export layers.

### `QuerySpec`

Describes how a visual derives its table.

Fields should eventually cover:

- source key in `Nexus`
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
- min/max size
- stacking rules
- breakpoints

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

1. `Portfolio` references a `Nexus`
2. each `Visual` names a source inside that `Nexus`
3. `Session` contributes active shared filters and parameters
4. `QuerySpec` resolves to a `DataFrame`
5. `BindSpec` converts the frame into the correct Flash plot call
6. the runtime host renders the result and wires interaction events back into
   `Session`

That means Flash now benefiting from direct `DataFrame` plot overloads is not a
nice-to-have; it is foundational for PlussBI. The BI layer should be able to
hand a resolved table straight into Flash without detouring through manual
payload extraction everywhere.

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

Later host should emit browser-ready assets from the same portfolio definition.

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

1. `Portfolio`
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

## Immediate Implementation Split

Suggested file grouping:

- `src/PlussBI/core/`
  - `Portfolio.*`
  - `Board.*`
  - `Tile.*`
  - `Session.*`
- `src/PlussBI/query/`
  - `QuerySpec.*`
  - `Planner.*`
  - `Cache.*`
- `src/PlussBI/bind/`
  - `BindSpec.*`
  - `VisualFactory.*`
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
