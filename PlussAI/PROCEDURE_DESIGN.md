# PlussAI Procedure Design

Status: design contract.

Purpose: define a robust, efficient deployment and orchestration system above the
current PlussAI families. `Procedure` is not a replacement for Lymph `Workflow`,
NodeB `Workflow`, Ice `Model`, or NodeJ `Flow`; it is the production graph that
coordinates them.

## Sources Read

Local doctrine and current PlussAI surface read before this design:

- `docs/doctrine.md`
- `PlussAI/Lymph/LYMPH_REDESIGN_PLAN.md`
- `PlussAI/Lymph/LYMPH_GENERAL_ML_EXPANSION_PLAN.md`
- `PlussAI/Lymph/demos/LYMPH_WORKFLOW_RIVAL_PLAN.md`
- `PlussAI/include/pluss/ai/lymph/api.h`
- `PlussAI/include/pluss/ai/lymph/model.h`
- `PlussAI/include/pluss/ai/lymph/workflow.h`
- `PlussAI/include/pluss/ai/lymph/policy.h`
- `PlussAI/include/pluss/ai/lymph/payload.h`
- `PlussAI/include/pluss/ai/lymph/stage.h`
- `PlussAI/include/pluss/ai/lymph/time.h`
- `PlussAI/include/pluss/ai/lymph/builder.h`
- `PlussAI/include/pluss/ai/lymph/spotcheck.h`
- `PlussAI/include/pluss/ai/nodej/compose.h`
- `PlussAI/include/pluss/ai/nodej/io.h`
- `PlussAI/Ice/ICE_BLUEPRINT.md`
- `PlussAI/include/pluss/ai/ice.h`
- `PlussAI/include/pluss/ai/ice/model.h`
- `PlussAI/include/pluss/ai/ice/outline.h`
- `PlussAI/include/pluss/ai/ice/coach.h`
- `PlussAI/include/pluss/ai/ice/io.h`
- `PlussAI/NodeB/NODEB_BLUEPRINT.md`
- `PlussAI/include/pluss/ai/nodeb/workflow.h`

External design context:

- Shared Procedure discussion:
  `https://chatgpt.com/share/6a790faf-9a98-83e8-9bc4-a7badb48b181`

## Positioning

`Procedure` belongs at the `PlussAI` level first:

```text
PlussTils
  Optional future substrate only: generic DAG ids, artifact ids, status/result
  containers, and scheduler primitives. No ML, inference, monitoring, registry,
  or deployment semantics.

NodeJ
  Classical ML kernels, estimators, transforms, metrics, splitters, sparse
  execution, low-level flow bundles.

Lymph
  Pluss-native classical/general ML user surface. `Workflow` is the sklearn
  Pipeline rival: raw admission + preprocessing/feature composition + model
  learn/infer/assess/save.

NodeK / Ice
  NodeK is the tensor/module/kernel engine. Ice is the preferred public bridge
  from Procedure into NodeK-backed work: Ice owns model training, inference,
  checkpointing, state restoration, and deep/time-series/NLP model conveniences.
  Procedure is therefore concerned with NodeK, but normally through Ice rather
  than by exposing raw NodeK modules as first-class deployment steps.

NodeB
  NLP workflow and task surface, consuming NodeJ and NodeK/Ice as needed.

Procedure
  Production deployment graph over all of the above, plus raw Pluss functions
  built on Series/DataFrame/Note/Thread/Timeline.
```

Do not put the full system in `PlussTils` now. A production Procedure will need
AI-specific semantics: model/workflow artifacts, inference contracts, drift
checks, retraining hooks, champion/challenger rollout, metrics, lineage, payload
schemas, and deployment metadata. Those do not belong in a generic utility
package. If the internal graph/runtime substrate later proves useful outside
PlussAI, extract only that substrate downward.

## Core Distinction

```text
Model
  A trainable/runnable estimator or transform model.

Workflow
  One ML composition path that can learn and infer: policy, payload roles,
  branch preprocessing, target transforms, estimator, and ML-local persistence.

Procedure
  A production graph that may contain many workflows, models, functions, gates,
  validations, reports, sinks, and decision steps.
```

`Workflow` answers: "How does this one ML path turn raw input into predictions?"

`Procedure` answers: "How does this deployable system admit data, run the right
components in the right order, record what happened, enforce safety rules, and
emit artifacts/results?"

## Design Goals

1. Keep the public API lean. Procedure should coordinate, not swallow every
   concern into one giant object.
2. Preserve the Pluss family rule: factories create values; methods operate on
   values already in existence.
3. Avoid sklearn/cloud-orchestrator naming. Use Pluss-native names and concise
   verbs.
4. Support Lymph, NodeJ, Ice, NodeB, and raw Pluss computations through adapters.
   NodeK support is required through Ice first, with raw NodeK adapters kept as
   an advanced escape hatch for low-level users.
5. Keep `Series` and vectorized batch execution first-class. Procedure must not
   introduce row-by-row host loops.
6. Make production safety default: schema checks, time/leakage checks, artifact
   integrity, deterministic replay, and centralized Pluss errors.
7. Separate runtime from graph shape. The same Procedure should run locally first
   and later on parallel/distributed backends without rewriting user code.
8. Make persistence inspectable and durable: versioned manifests, checksums,
   component bundle references, and explicit compatibility rules.

## Non-Goals

1. Procedure is not the new place for model training algorithms.
2. Procedure is not the new Lymph experimentation API. Builder, SpotCheck,
   Search, Validate, Metrics, Shape, Detect, Cluster, Embed, and Semi stay in
   Lymph.
3. Procedure is not a replacement for Ice `Coach` or NodeB `Workflow`.
4. Procedure is not a distributed system in phase one.
5. Procedure should not require every component to look like `learn/transform/
   predict`. Components advertise capabilities instead.

## Public Families

Keep the public surface intentionally small:

```text
pluss::ai::

  Procedure       deployable graph value
  Step            named graph component
  Link            directed dependency between steps
  Artifact        typed value moving through a Procedure run
  ProcedureResult completed run output, metrics, trace, and artifact lookup

pluss::ai::procedure::

  make(...)       create a Procedure
  step(...)       create a named Step
  link(...)       create a Link
  input(...)      create a named run input artifact
  sink(...)       create an output/persistence Step target
  load(...)       restore a saved Procedure manifest
```

Option structs stay designated-init:

```cpp
ProcedureOptions op{};
op.workers = 8;
op.schedule = ProcedureSchedule::Deterministic;
op.cache = ProcedureCache::ReadyArtifacts;
op.fail_fast = true;

RunOptions run{};
run.mode = ProcedureMode::Infer;
run.audit = true;
```

This mirrors the Lymph rule: `procedure::make(...)` brings the object into
existence; then `proc.run(...)`, `proc.save(...)`, `proc.check(...)`, and
`result.trace()` are methods.

## Capability Contracts

Do not force all steps into one estimator protocol. A step advertises one or
more capabilities:

```text
Admit       validate/filter incoming raw data
Map         transform one or more artifacts into new artifacts
Learn       learn/update state from training artifacts
Infer       produce predictions/outputs from input artifacts
Assess      produce metrics/reports from outputs and truth
Gate        accept/reject/route based on a condition
Join        combine multiple upstream artifacts
Sink        persist or emit artifacts outside the graph
Watch       record telemetry, drift, data quality, and run diagnostics
```

Most user code should not implement these interfaces directly. Existing PlussAI
objects get adapters:

```text
lymph::Workflow   -> Admit/Infer/Assess/Learn/Save adapter
lymph::Model      -> Infer/Learn adapter for already-shaped data
ice::Model        -> Infer/Learn/Assess/Save adapter over NodeK-backed state
nodeb::Workflow   -> Infer/Learn/Assess/Save adapter for docs/text
Series function   -> Map adapter
DataFrame function-> Map adapter
NodeK module      -> advanced Map/Infer adapter only when no Ice wrapper exists
custom class      -> capability adapter with optional save/load hooks
```

NodeK should not be ignored by Procedure, but it should not define the ordinary
Procedure user surface either. The stable public path is:

```text
Procedure -> Ice -> NodeK
```

Raw NodeK integration is valuable for expert users and internal systems, but it
should remain an explicit low-level adapter with a declared artifact contract,
device policy, and persistence policy. That keeps Procedure from absorbing
tensor/module/kernel concerns that already belong to NodeK and Ice.

## Artifact Model

`Artifact` is the typed value passed between Procedure steps. It should be
cheap-handle based where possible, preserving the existing Pluss ownership
style.

Initial variants should cover:

```text
Series
FeatureBatch
LabeledBatch
DataFrame
Note
Thread
Timeline / time metadata when available
lymph::Payload
lymph::MetricReport
nodeb::Doc / vector<Doc>
scalar numeric values
string/log metadata
saved artifact handle
```

Avoid a public `std::any`-style API. Internally, type erasure is acceptable, but
the public edge contract should know what each step consumes and emits. Bad
connections should fail at Procedure construction or `check()`, not halfway
through production inference.

## Runtime Model

Procedure runtime has three layers:

```text
Graph planner
  Validates dependencies, required inputs, output names, cycles, and capability
  compatibility.

Executor
  Runs ready steps. Starts with local deterministic execution. Adds parallel
  ready-step execution next. Distributed execution is a later backend.

Kernel owner
  Stays inside NodeJ, NodeK/Ice, Lymph, NodeB, Series, and DataFrame. Procedure
  does not implement numerical kernels.
```

Parallelism is meaningful but controlled:

1. Independent Procedure steps can run concurrently when their dependencies are
   satisfied.
2. Lymph `Workflow` owns branch-level preprocessing parallelism.
3. Validate/Search/SpotCheck own fold/trial/candidate parallelism.
4. Ice owns NodeK-backed batch/chunk training and inference execution through
   `Coach`, `Source`, and model checkpoint/state machinery.
5. NodeJ/NodeK/Series own vectorized kernels and device execution.

Procedure must not split a single `Series` into host-side row loops unless a step
explicitly declares that it is scalar/row-wise and accepts the performance cost.

## Persistence

Procedure persistence should be manifest-first:

```text
procedure.bundle/
  manifest.plussproc
  steps/
    lymph_risk.flow
    lymph_risk.flow.policy
    ice_forecaster.params
    nodeb_text.pack
  audit/
    schema.json-like compact metadata
    lineage table
    checksums
```

The manifest records:

1. Procedure schema version.
2. Pluss producer/version metadata.
3. Step names, stable ids, capabilities, input/output artifact contracts.
4. Links and output names.
5. Runtime options.
6. Time/leakage policy metadata.
7. Component artifact paths, checksums, and component schema versions.
8. Whether each component is trained, frozen, reloadable, or custom/non-persistable.

Persistence should reuse existing lower-level writers:

```text
Lymph Workflow -> Workflow::save / workflow::load
NodeJ Flow     -> nodej::io::save / load / restore
Ice Model      -> ice::io parameter/checkpoint writers
NodeB Workflow -> nodeb::Workflow::save / load
```

For custom functions/classes, Procedure should support two tiers:

```text
Callable-only
  Allowed for local runs and demos. Marked non-persistable. save() fails loudly
  unless the caller chooses a manifest policy that explicitly allows external
  code references.

Persistable custom step
  Requires a stable name, version, declared contract, and user-provided save/load
  hooks or registry key.
```

## Time And Leakage Safety

Procedure must preserve the time discipline already started in Lymph:

1. Steps can declare whether they learn from future data, aggregate over windows,
   or require ordered input.
2. Procedure validates that time-aware inputs carry a time role before running
   time-sensitive steps.
3. A training Procedure may learn transforms only inside the allowed training
   window.
4. An inference Procedure must reject steps that require fitting unless a fitted
   artifact is already present.
5. Gap, horizon, lookback, and observed/target time semantics should be carried
   through a shared Procedure time contract that can consume Lymph `TimeOptions`
   initially and later Timeline-aware metadata.
6. Any step that changes row count must declare it. Downstream label/weight/time
   alignment then becomes a runtime-checked contract, not a caller hope.

## Error And Diagnostics

All Procedure implementation must use centralized Pluss errors:

```text
PLUSS_CHECK / PLUSS_FAIL / ::pluss::throw_error / Status / StatusOr
```

Diagnostics should be structured:

```text
ProcedureResult::ok()
ProcedureResult::trace()
ProcedureResult::artifacts()
ProcedureResult::get<T>(name)
ProcedureResult::metrics()
ProcedureResult::warnings()
ProcedureResult::failed_step()
```

The trace must include:

1. step start/end times;
2. input/output artifact names and shapes;
3. cache hits/misses;
4. component versions/checksums;
5. row-count changes;
6. warning/error messages.

## Lean API Sketch

Final names can still be tightened, but the target style should be close to:

```cpp
namespace ai = pluss::ai;
namespace lymph = pluss::ai::lymph;
namespace ice = pluss::ai::ice;

lymph::Workflow risk = lymph::workflow::load("risk.flow");
ice::Outline forecaster = ice::Outline(...);
forecaster.load("forecast.params");

ai::ProcedureOptions op{};
op.workers = 8;
op.schedule = ai::ProcedureSchedule::Deterministic;
op.cache = ai::ProcedureCache::ReadyArtifacts;

ai::Procedure proc = ai::procedure::make({
    ai::procedure::step("gate", lymph::policy({
        .require_columns = {"timestamp", "region", "risk_score"},
        .reject_missing = {"timestamp"},
    })),
    ai::procedure::step("risk", risk),
    ai::procedure::step("forecast", forecaster),
    ai::procedure::step("store", ai::procedure::sink("risk_forecast.out")),
}, {
    ai::procedure::link("gate", "risk"),
    ai::procedure::link("risk", "forecast"),
    ai::procedure::link("forecast", "store"),
}, op);

ai::ProcedureResult out = proc.run(ai::procedure::input("raw", incoming));
proc.save("risk_forecast.proc");
```

The sketch intentionally hides detailed port syntax. The implementation should
support explicit input/output names, but the common path must not force verbose
port plumbing when each step has one obvious input and output.

For a raw Pluss computation:

```cpp
ai::Procedure p = ai::procedure::make({
    ai::procedure::step("returns", [](const Series& prices) {
        return prices.diff() / prices.lag(1);
    }),
    ai::procedure::step("vol", [](const Series& returns) {
        return returns.rolling(30).std();
    }),
}, {
    ai::procedure::link("returns", "vol"),
});

Series vol = p.run(ai::procedure::input("prices", prices)).get<Series>("vol");
```

The above is an API direction, not final implementation. The important rule is
that construction happens through the family factory and runtime behavior stays
on the `Procedure`/`ProcedureResult` objects.

## Interaction With Lymph Experimentation

The expected classical ML lifecycle remains:

```text
1. Use Lymph families for experimentation:
   Policy, Payload, Workflow, Builder, SpotCheck, Search, Validate, Metrics.

2. Select a champion Workflow or set of workflows.

3. Save/freeze those workflows.

4. Build a Procedure that coordinates the frozen workflows with sources,
   gates, auxiliary computations, reports, and sinks.
```

Procedure may later run scheduled retraining, but it should do so by consuming
Lymph `Search`/`Validate`/`Workflow` artifacts or a dedicated retrain step. It
must not absorb Lymph's experimentation families into its root API.

## Implementation Phases

### Phase 1: Design Scaffold

- Add `PlussAI/PROCEDURE_DESIGN.md`.
- Create `PlussAI/Procedure/` and public include skeletons.
- Define `ProcedureOptions`, `RunOptions`, `Step`, `Link`, `Artifact`,
  `Procedure`, and `ProcedureResult`.
- Implement construction validation: duplicate names, missing endpoints, cycles,
  empty graph, invalid contracts.

### Phase 2: Local Sequential Executor

- Implement deterministic topological execution.
- Add artifact table and typed lookup.
- Add `Procedure::run(...)`, `Procedure::check()`, and `ProcedureResult::trace()`.
- Support raw `Series` and `DataFrame` map steps first.

### Phase 3: Lymph Adapter

- Adapt `lymph::Policy`, `lymph::Payload`, `lymph::Workflow`, and trained
  `lymph::Model`.
- Preserve label/weight/time alignment checks.
- Reuse `Workflow::save/load` and NodeJ bundle writers.
- Add tests for inference-only and learn-then-infer procedures.

### Phase 4: Ice, NodeK, And NodeB Adapters

- Adapt `ice::Model`/`ice::Outline` for Series/DataFrame inference and optional
  learning jobs.
- Treat this as the main NodeK integration path: Procedure delegates tensor
  execution, device placement, and model state to Ice/NodeK instead of handling
  raw modules itself.
- Add a limited raw NodeK adapter only for explicit low-level steps with declared
  input/output artifact types and persistence policy.
- Adapt `nodeb::Workflow` for text/doc inference and assessment.
- For Ice custom subclasses, require explicit restoration policy before
  Procedure save is allowed.

### Phase 5: Parallel Local Runtime

- Add ready-step parallel execution with deterministic output ordering.
- Add cache policies for ready artifacts and frozen component outputs.
- Ensure parallel Procedure execution does not fight Lymph/NodeJ/Ice internal
  worker settings.

### Phase 6: Persistence And Registry Hooks

- Implement manifest save/load.
- Add checksums, schema versions, component artifact references, and compatibility
  checks.
- Add custom-step persistability contracts.
- Keep registry optional; Procedure should save/load locally without requiring a
  service.

### Phase 7: Safety And Observability

- Add schema/time/leakage gate enforcement.
- Add row-count and artifact-shape trace records.
- Add warnings, failure reports, and component-level diagnostics.
- Add monitoring hooks without turning Procedure into a monitoring product.

### Phase 8: Production Hardening

- Add rollback-friendly bundle layout.
- Add dry-run validation.
- Add batch/chunk policies for large artifacts.
- Add demos covering Lymph, Ice, NodeB, raw Series functions, branching, joins,
  persistence, and failure cases.

### Phase 9: Optional Substrate Extraction

- If non-AI Pluss modules need the same graph/runtime substrate, extract only
  generic pieces into PlussTils:
  - graph ids;
  - topological planner;
  - artifact id/status containers;
  - local scheduler primitive.
- Keep AI semantics in PlussAI.

## Definition Of Done

The first durable Procedure implementation is complete when:

1. A Procedure can run a graph containing at least one raw Series/DataFrame step
   and one Lymph Workflow step.
2. A Procedure can save/load a manifest and restore component artifacts.
3. A Procedure can reject invalid graphs before execution.
4. A Procedure can run independent branches in parallel with deterministic
   output names.
5. A Procedure can report a useful trace without exposing internal private
   headers or implementation-only types.
6. No numerical work is implemented as Procedure-level host loops when a Series
   or lower-family operation should own it.
7. Public demos show end-to-end deployment use without helper-heavy boilerplate.
