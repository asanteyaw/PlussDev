# Lymph Workflow Rival Plan

This note is for the demo/design pass before broadening `lymph::Workflow`.
It documents what was read, what the current Lymph surface can already show,
and what must change for Workflow to rival Python pipeline composition while
staying Pluss-native.

## References Read

- scikit-learn Pipeline user guide: `https://scikit-learn.org/stable/modules/compose.html#pipeline`
- scikit-learn Pipeline API docs: `https://scikit-learn.org/stable/modules/generated/sklearn.pipeline.Pipeline.html`
- scikit-learn Pipeline source: `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/pipeline.py`
- scikit-learn ColumnTransformer source: `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/compose/_column_transformer.py`
- scikit-learn transformed target docs: `https://scikit-learn.org/stable/modules/compose.html#transformed-target-regressor`
- scikit-learn FunctionTransformer docs: `https://scikit-learn.org/stable/modules/generated/sklearn.preprocessing.FunctionTransformer.html`
- scikit-learn FunctionTransformer source:
  `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_function_transformer.py`
- scikit-learn custom estimator guide:
  `https://scikit-learn.org/stable/developers/develop.html#rolling-your-own-estimator`
- scikit-learn metadata routing guide:
  `https://scikit-learn.org/stable/metadata_routing.html`
- scikit-learn metadata request source:
  `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/utils/_metadata_requests.py`
- scikit-learn TimeSeriesSplit docs:
  `https://scikit-learn.org/stable/modules/generated/sklearn.model_selection.TimeSeriesSplit.html`
- scikit-learn split source:
  `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/model_selection/_split.py`

## What Pipeline Does Well

1. It makes a chain of fitted transforms plus a final estimator behave like one estimator.
2. It keeps preprocessing inside validation/search, reducing train/test leakage.
3. It exposes final-estimator actions from the composed object: prediction, scores,
   transforms, inverse transforms, probability-like outputs, and sample scores where the
   final estimator supports them.
4. It gives each step a stable name, supports step inspection, slicing, replacement, and
   nested parameter tuning.
5. It composes with column routing, feature unions, target transforms, caching, and newer
   metadata routing for inputs such as validation sets or sample weights.

## Pipeline Pressure Points Lymph Should Beat

1. Nested parameter routing is string-heavy. Lymph should expose typed route handles and
   tabular ledgers while still allowing compact named parameter paths for search.
2. Caching can clone fitted transformers, so the object the caller originally passed may not
   be the fitted one. Lymph `Model` already uses cheap intrusive handles with copy-on-write;
   Workflow caching should preserve inspectability through fitted step views.
3. Feature union and column-transformer-style composition can allow duplicate feature names
   unless callers guard them. Lymph should namespace branch output and fail loudly on
   collisions unless an explicit conflict policy says otherwise.
4. Pipeline is not a production deployment object by itself. Lymph should not overload
   Workflow with future production concerns; Workflow should be the composable ML execution
   object, while the later `Procedure` family should own production packaging, service
   runtime, audit gates, monitoring, and deployment metadata.
5. Metadata routing is powerful but complex. Lymph should route typed channels (`x`, `y`,
   sample weights, groups, validation frames, time keys) through explicit option structs and
   Workflow methods rather than ad-hoc keyword forwarding.
6. Output container control is bolted onto estimator protocol. Lymph should preserve
   `Series`, `Thread`, `Note`, `Timeline`, and `DataFrame` semantics directly, keeping GPU
   and Series paths first-class.

## Current Lymph Baseline

The demos in this directory cover the real surface available today:

- `lymph_supervised_workflow_demo.cpp`: raw frame cleanup, staged model prep, supervised
  training, reporting, workflow save/load, and raw incoming inference.
- `lymph_selection_search_demo.cpp`: Builder inventories, validation, benchmarking,
  spot-checking, top/bottom ranked selections, and model-aware search.
- `lymph_general_ml_demo.cpp`: shape profiling and gaussianizing plans, clustering,
  embedding, anomaly detection, and semi-supervised pseudo-labeling.

The current `Workflow` is still narrow: it is a `Policy + Model` wrapper. It can learn,
infer, assess, save, and load a raw-frame-shaped artifact, but composition breadth mostly
lives inside `Model::prepare()`, standalone `prep::` transformers, `workflow_dsl.h` branch
helpers, and separate families such as `validate`, `search`, and `spotcheck`.

## Target Workflow Contract

`Workflow` should become the trainable/runnable composition object, not a god object and not
a replacement for Builder/Search/Validate/SpotCheck. `Procedure`, designed later, will be
the production deployment object. The target line is:

```cpp
lymph::Model model = lymph::models::random_forest_regressor();
model.append(lymph::stage::simple_imputer(...), lymph::columnrouter::names({"income", "debt"}))
     .append(lymph::stage::standard_scaler(), lymph::columnrouter::names({"income", "debt", "age"}))
     .append(lymph::stage::one_hot_encoder(...), lymph::columnrouter::names({"region"}));

lymph::Workflow wf = lymph::workflow::make(model, cleanup);
wf.learn_(train_frame, data_options);
Series yhat = wf.infer(incoming_frame, data_options);
wf.save(workflow_path);
```

That shape should expand in capability, not namespace depth. Creation remains
`lymph::workflow::make(...)`; after existence, Workflow work is method-based.

## Lean Lifecycle Contract

The redesign must not add a swarm of public orchestration nouns just to mirror Python
examples. `Recipe`, `PrepPlan`, and `Experiment` are not public Lymph families in this
plan. The durable public lifecycle is:

1. `Policy` admits and cleans the raw frame at the schema/leakage boundary.
2. `Workflow` describes and executes preprocessing/feature composition.
3. `Model` owns estimator behavior.
4. `Builder`, `SpotCheck`, `Search`, and `Validate` consume `Model` or `Workflow` when
   they need to benchmark the whole path.
5. Result/report families expose ranking, metrics, and interpretation.

That means Workflow can be used in two forms:

```cpp
lymph::Workflow design = lymph::workflow::make(raw_policy);
design.workers(4);
design.on("num", numeric_columns)
    .add(lymph::stage::simple_imputer({.strategy = lymph::prep::SimpleImputerStrategy::Median}))
    .add(lymph::stage::standard_scaler());
design.on("cat", categorical_columns)
    .add(lymph::stage::simple_imputer({.strategy = lymph::prep::SimpleImputerStrategy::MostFrequent}))
    .add(lymph::stage::one_hot_encoder());
design.join();

lymph::Workflow candidate = design.with(lymph::models::logistic_regression());
candidate.learn_(train, data);
```

or, when no model comparison is needed:

```cpp
lymph::Workflow wf = lymph::workflow::make(lymph::models::random_forest_regressor(), raw_policy);
wf.on(numeric_columns)
    .add(lymph::stage::simple_imputer({.strategy = lymph::prep::SimpleImputerStrategy::Median}))
    .add(lymph::stage::standard_scaler());
wf.learn_(train, data);
```

`SpotCheck` and `Search` are allowed to consume a Workflow design because this is the
cleanest way to evaluate preprocessing plus estimator inside each validation fold. The
benefit is leakage safety: fitted imputers, scalers, encoders, samplers, and target
transforms learn only from the training fold/window before the held-out fold is scored.
This is not Workflow swallowing experimentation; it is other families consuming Workflow
as a reusable execution object.

Parallelism also stays separated by responsibility:

1. Workflow owns composition-level branch parallelism for independent `on(...)` routes.
2. `Validate`, `Search`, and `SpotCheck` own fold/trial/candidate parallelism.
3. NodeJ owns estimator and transform kernel parallelism.

The public API should stay lean: add method accessors only when they make an existing
object read better, not as a reason to create deeper namespaces. Namespaces should normally
stop at two `::` segments, for example `lymph::workflow::make(...)`; method chaining should
carry the rest.

## Policy Boundary

`Policy` is not the imputer, scaler, encoder, or feature engineer. It is the raw-frame
admission/schema layer that runs before Workflow stages:

```cpp
lymph::Policy raw = lymph::policy({
    .drop_columns = {"loan_id", "audit_flag"},
    .require_columns = {"income", "debt", "region", "target"},
    .forbid_columns = {"future_status"},
    .reject_missing = {"target"},
    .allow_missing = {"income", "debt", "region"},
});
```

Policy owns:

1. dropping ID/leakage/audit-only columns before modeling;
2. required raw-column checks;
3. forbidden raw-column checks;
4. explicit row admission such as rejecting rows with missing labels or time keys;
5. allowing feature missingness to pass onward to Workflow imputers.

Policy does not own:

1. imputation;
2. scaling;
3. encoding;
4. feature generation;
5. balancing;
6. model-driven cleaning.

The legacy broad `filter_nulls = true` behavior is retained for compatibility, but new code
should prefer `reject_missing = {...}`. If broad filtering is used, `allow_missing` marks
columns that are intentionally passed to imputation stages.

## Target Ergonomic Contract

The sklearn shape:

```python
numeric_pipeline = Pipeline([
    ("imputer", SimpleImputer(strategy="median")),
    ("scaler", StandardScaler()),
])

categorical_pipeline = Pipeline([
    ("imputer", SimpleImputer(strategy="most_frequent")),
    ("encoder", OneHotEncoder(handle_unknown="ignore")),
])

preprocessor = ColumnTransformer([
    ("num", numeric_pipeline, numerical_columns),
    ("cat", categorical_pipeline, categorical_columns),
])

final_pipeline = Pipeline([
    ("preprocessor", preprocessor),
    ("model", LogisticRegression(C=..., penalty=...)),
])
```

should map to this Lymph primary shape: composition is mostly object data, not a method-heavy
Workflow root.

```cpp
namespace lymph = pluss::ai::lymph;

auto numeric = lymph::workflow::branch("num", numerical_columns, {
    {"fill",  lymph::stage::imputer({.strategy = lymph::Fill::Median})},
    {"scale", lymph::stage::scaler()}
});

auto categorical = lymph::workflow::branch("cat", categorical_columns, {
    {"fill",   lymph::stage::imputer({.strategy = lymph::Fill::Mode})},
    {"encode", lymph::stage::encoder({.unknown = lymph::Unknown::Keep})}
});

lymph::WorkflowOptions flow{};
flow.workers = 4;

lymph::Workflow wf = lymph::workflow::make({
    numeric,
    categorical,
}, lymph::models::logistic_regression({
    .strength = 0.02,
    .penalty = lymph::Penalty::L2,
}), flow);

wf.learn_(train, data);
Series pred = wf.infer(test, data);
```

The method-builder form remains useful while interactively constructing an existing object,
but it is not the intended long-term center of gravity:

```cpp
lymph::Workflow wf = lymph::workflow::make(policy);
wf.on("num", numerical_columns).add(lymph::stage::imputer(...)).add(lymph::stage::scaler());
wf.on("cat", categorical_columns).add(lymph::stage::imputer(...)).add(lymph::stage::encoder(...));
wf = wf.with(lymph::models::logistic_regression(...));
```

Then search can target stable branch/model paths without recreating sklearn's double-underscore
spelling:

```cpp
lymph::Search s = lymph::search::grid({
    {.name = "model.strength", .values = {0.01, 0.1, 1.0}},
    {.name = "cat.encode.max_levels", .values = {32, 64, 128}},
});
```

The design promise is therefore:

1. sklearn-like compositional clarity without copying sklearn names.
2. Branches are declarative objects: `workflow::branch(name, cols, steps)`.
3. `WorkflowOptions` is the primary home for execution settings such as worker count,
   deterministic scheduling, cache policy, and future graph execution policy.
4. `wf.on(cols).add(stage)` is a convenience builder for an existing Workflow, not the
   canonical way to express every serious workflow.
5. Names are required for branch construction because they are needed for inspection,
   tuning, search, persistence, and explainability.
6. Short Pluss-native verbs are preferred only for stable root actions:
   `learn_`, `learn`, `map`, `learn_map`, `infer`, `infer_scores`, `assess`, `save`.
7. Workflow must not grow a method for every possible transformation. The escape hatch for
   breadth is `wf.on(cols).add(stage)`: `stage` owns transform construction and custom
   extension, while Workflow owns routing/execution.

## Required Upgrades

1. Add a typed workflow payload that can carry raw `DataFrame`, `FeatureBatch`,
   `LabeledBatch`, sample weights, groups, time keys, and validation data without stringly
   keyword forwarding.
2. Make Workflow composition branch-aware: column-routed stages, branch joins, passthrough,
   drop, explicit remainder policy, feature-name generation, and collision checks.
3. Add fit/map/infer method families based on final model task:
   `learn_`, `map`, `learn_map`, `infer`, `infer_scores`, `assess`, and detector/cluster/
   embed/semi result production where the final model task supports it.
4. Persist an experimentation/reproducibility artifact: policy, schema, feature names, label
   semantics, category maps, stage states, model state, training metadata, and compatibility
   version. Production hardening belongs to `Procedure`.
5. Add reversible-stage contracts: `inverse(...)` and target inverse for supported stages,
   with loud failure for non-invertible or lossy stages.
6. Add cache policy as a Workflow option: no cache by default, content-addressed stage cache
   when requested, fitted step views that remain inspectable, and cache keys based on stage
   blueprint plus input fingerprint.
7. Expose inspection without leaking internals: `steps()`, `step(name)`, `schema()`,
   `feature_names()`, `summary()`, and `board()`.
8. Keep Builder/Search/Validate/SpotCheck as consumers of `Model` or `Workflow`, not methods
   on Workflow. Workflow owns composition and end-to-end execution; model comparison and
   tuning remain separate families.

## Phase Ledger

1. **Workflow Core - implemented first pass.** Make `Workflow` copy-ergonomic and broaden it from
   raw-frame `learn_/infer/assess` to the same core model-shaped actions: `learn`,
   `map`, `learn_map`, `infer_scores`, batch paths, metadata inspection, and explicit
   row-filter safety for separate `x/y` calls.
2. **Step Surface - implemented first pass, trimmed in phase 6.** Keep `wf.prep()` for
   the existing model prep accessor and `wf.on(cols)` for column-routed work, but keep
   `WorkflowRoute` itself lean: the durable operation is `add(Stage)`. Convenience route
   verbs such as `fill`, `scale`, `encode`, `power`, and `gaussianize` are deferred until
   the artifact surface is stable and each one can justify its weight.
3. **Branch DAG - implemented first pass.** Add named/unnamed branch graph metadata,
   route handles, `join()`, `workers(n)`, and `with(model)` so a reusable Workflow design
   can absorb the selected estimator without rebuilding preprocessing. Execution still
   delegates to the existing linear Model/NodeJ path until the NodeJ branch executor lands.
4. **Typed Payload - implemented first pass.** Introduce `Payload` plus
   `lymph::payload::make(...)` as the shared carrier for frames, feature batches, labeled
   batches, labels, weights, groups, and time roles. Workflow consumes Payload through
   `learn_`, `learn`, `map`, `learn_map`, `infer`, `infer_scores`, and `assess` overloads.
   The carrier delegates frame conversion to existing NodeJ/Lymph batch builders so routed
   stages still resolve against DataFrame column names. DataFrame-backed payload weights
   must currently be declared as a weight column so Policy row filtering cannot desync a
   separate Series.
5. **Time Safety - implemented first pass.** Add shared `TimeOptions`, row-step helpers,
   Payload/Workflow time roles, `validate::time()`, and validation guards. Time-aware
   Workflows must use time validation, and time validation requires explicit Payload time
   roles. Actual split materialization continues to use NodeJ's existing TSCrossVal kernel
   (`time_gap`, `time_test_size`, `time_max_train_size`) for performance and consistency.
6. **Inverse Contracts - implemented first pass.** Add `TransformContract` on `Stage`
   with conservative metadata for reversibility, lossiness, statefulness, feature-count
   changes, and whether inverse execution is currently available. Feature-stage inverse
   execution remains unavailable until NodeJ exposes fitted-state reverse kernels. Target
   transforms already exist through `TargetTransform` and continue to use Series-native
   forward/inverse functions with round-trip checks.
7. **User Extensions - implemented first pass.** Add `lymph::stage::custom(...)` as a
   named callable stage value backed by NodeJ's `prep::FunctionTransformer`, plus
   `Model::add(custom_stage)` and `Workflow::add(custom_stage)` for whole-feature-space
   transforms. This intentionally does not fake routed custom callables yet: `wf.on(cols)`
   still accepts built-in `Stage` values only until NodeJ has a route-aware custom
   `TransformSpec`/registry path. Custom model contracts remain planned rather than
   wrapped prematurely.
8. **Family Interop - implemented first pass.** `Validate`, `SpotCheck`, `Search`, and
   `Reports` now consume `Workflow + Payload` where that pairing is semantically valid.
   A shared internal adapter applies Workflow `Policy`, preserves DataFrame label-column
   alignment, rejects unsafe external Series weights/labels when policy can drop rows, and
   falls back to Series-native `LabeledBatch` paths when the payload is already batched.
   Results intentionally stay lean: SpotCheck/Search still return ranked trained `Model`
   handles, and Reports still return metric/report values. Workflow does not grow
   benchmark/search/report methods.
9. **Persistence And Demos - implemented first pass.** `Workflow::save(path, payload,
   train_options, metadata)` now persists through the same typed Payload adapter used by
   Validate, SpotCheck, Search, and Reports, then writes the Workflow policy sidecar. This
   gives the compact deployable experimentation shape without forcing callers back to
   `DataFrameBatchOptions` plumbing after they have already declared a Payload. The new
   `demos/suite` directory adds 36 explicit demo targets covering synthetic Series
   workflows and real CSV workflows across supervised, time-series, selection/search,
   reporting, shape, clustering, embedding, anomaly detection, and semi-supervised
   families. Production service/runtime concerns remain reserved for the future
   `Procedure` family.

### Phase 9 Construction Surface

The construction-style Workflow shape is now real enough for demos and API-reference
drafting:

```cpp
lymph::WorkflowOptions flow{};
flow.workers = 4;

lymph::Workflow wf = lymph::workflow::make(
    lymph::policy({.reject_missing = {"target"}}),
    std::vector<lymph::WorkflowBranchSpec>{
        lymph::workflow::branch("num", {"x1", "x2"}, {
            {"fill", lymph::stage::simple_imputer()},
            {"scale", lymph::stage::standard_scaler()},
        }),
        lymph::workflow::branch("cat", {"region"}, {
            {"encode", lymph::stage::one_hot_encoder()},
        }),
    },
    lymph::models::logistic_regression(),
    flow);

wf.learn_(payload);
wf.save(path, payload);
```

This is intentionally not a new experiment family. Workflow remains the composition object;
Builder, Validate, SpotCheck, Search, Reports, Cluster, Embed, Detect, Shape, and Semi keep
their own lines of work and consume Workflow only where that is the right boundary.

Current implementation status:

1. Construction-time `WorkflowOptions` stores worker count and policy metadata.
2. `workflow::branch(name, columns, steps)` creates named branch specs with named stages.
3. `workflow::make(policy, branches, model, options)` and related overloads build a routed
   Workflow without interactive route plumbing.
4. Branches still execute through the existing linear NodeJ Flow path; true branch-DAG
   parallel execution remains a NodeJ executor expansion.
5. Payload-backed save removes the last major ergonomic mismatch in the compact
   experimentation-to-Workflow path.

## Parallel Workflow Semantics

sklearn Pipeline is fundamentally sequential; parallelism generally lives outside it or
inside specific estimators. Lymph Workflow should support composition-level parallelism
where the graph allows it.

Target shape:

```cpp
lymph::WorkflowOptions flow{};
flow.workers = 4;
flow.schedule = lymph::Schedule::Deterministic;
flow.cache = lymph::Cache::Off;

auto numeric = lymph::workflow::branch("numeric", {"income", "debt", "age"}, {
    {"fill", lymph::stage::imputer({.strategy = lymph::Fill::Median})},
    {"scale", lymph::stage::scaler()},
    {"shape", lymph::stage::power()}
});

auto category = lymph::workflow::branch("category", {"region", "segment"}, {
    {"fill", lymph::stage::imputer({.strategy = lymph::Fill::Mode})},
    {"encode", lymph::stage::encoder({.unknown = lymph::Unknown::Keep})}
});

lymph::Workflow wf = lymph::workflow::make(cleanup, {
    numeric,
    category,
}, model, flow);
```

Parallelism is primarily construction/configuration based, not a method family. The
canonical durable setting is `WorkflowOptions`, passed to `workflow::make(...)`, because
search, validation, persistence, and reproducibility need the execution policy to be part
of the Workflow specification. A tiny method such as `wf.workers(4)` may remain as an
interactive convenience for an already-existing object, but it should be sugar over the
same stored options rather than a separate execution path.

Concrete usage should stay separated by owner:

```cpp
lymph::WorkflowOptions flow{};
flow.workers = 4;                         // branch graph workers

lymph::Workflow design = lymph::workflow::make(policy, {
    lymph::workflow::branch("num", numeric_cols, {
        {"fill", lymph::stage::simple_imputer(median_fill)},
        {"scale", lymph::stage::standard_scaler()},
    }),
    lymph::workflow::branch("cat", category_cols, {
        {"fill", lymph::stage::simple_imputer(mode_fill)},
        {"encode", lymph::stage::one_hot_encoder(unknown_keep)},
    }),
}, flow);

auto split = lymph::validate::cross()
    .folds(5)
    .workers(5);                          // fold workers

auto ranked = lymph::spotcheck::models(candidates)
    .workers(3)                            // candidate/strategy workers
    .run(design, data, split, assess);

auto tuned = lymph::search::grid(space)
    .workers(6)                            // trial workers
    .run(ranked.top(5), design, data, split, objective);
```

Current implementation status: `Workflow::workers(n)` records branch-worker metadata and
validation already owns fold-level workers. The full branch-DAG executor and
construction-time `WorkflowOptions` overloads still need NodeJ implementation before
Workflow branch execution is actually parallel.

Execution rules:

1. A plain chain runs sequentially because each stage consumes the prior stage output.
2. Independent branches over disjoint routes may learn/map concurrently.
3. Branch output is joined only after every dependency is complete.
4. Fold-level parallelism remains in `validate`, `search`, and `spotcheck`.
5. Estimator-internal parallelism remains in NodeJ.
6. Workflow should use a typed execution plan (`workers`, deterministic scheduling,
   branch dependency graph), not ad-hoc threads hidden behind individual methods.
7. Determinism must be explicit: identical seeds plus deterministic scheduling must produce
   reproducible branch ordering and feature ordering.

This gives Lymph a real advantage: Workflow can be a DAG when the dataflow is a DAG, while
still reading like one object to the user.

## Time-Series Workflow Safety

Workflow must be time-aware when temporal roles are declared. It should not merely accept a
timeline column; it should enforce prediction-time availability.

Target shape:

```cpp
lymph::Workflow wf = lymph::workflow::make(model, cleanup);
wf.time()
  .index("event_time")
  .observed_at("known_at")
  .horizon(30)
  .gap(7)
  .lookback(365);
```

Safety rules:

1. Temporal rows preserve order unless the user explicitly chooses a non-temporal workflow.
2. Validation/search over a time-aware Workflow must use a time-ordered split plan or fail
   loudly.
3. Rolling, lag, expanding, and window features must be past-only relative to the prediction
   timestamp.
4. Fit-time preprocessing inside every fold must learn on the training window only.
5. Columns can carry availability roles: event-time value, observation-time value,
   target-time value, known-before-prediction, known-after-prediction.
6. Workflow should reject features marked unavailable at inference time.
7. Gap and horizon are first-class: a row can be valid for one horizon and invalid for
   another.

The purpose is to prevent the common time-series leakage failures that survive ordinary
train/test split discipline.

### Time-Series Experiment-To-Workflow Shape

```cpp
namespace lymph = pluss::ai::lymph;

DataFrame raw = DataFrame::load_csv("energy_demand.csv");

lymph::Policy raw_gate = lymph::policy({
    .require_columns = {"timestamp", "region", "temperature", "humidity", "demand"},
    .reject_missing = {"timestamp", "demand"},
    .allow_missing = {"temperature", "humidity", "region"},
});

lymph::Payload data = lymph::payload::make(raw)
    .features({"temperature", "humidity", "region"})
    .label("demand")
    .time("timestamp")
    .note(lymph::DataNotePolicy::Factorize);

auto numeric = lymph::workflow::branch("num", {"temperature", "humidity"}, {
    {"fill", lymph::stage::imputer({.strategy = lymph::Fill::Median})},
    {"scale", lymph::stage::scaler()}
});

auto category = lymph::workflow::branch("cat", {"region"}, {
    {"fill", lymph::stage::imputer({.strategy = lymph::Fill::Mode})},
    {"encode", lymph::stage::encoder({.unknown = lymph::Unknown::Keep})}
});

lymph::WorkflowOptions flow{};
flow.workers = 4;

lymph::Workflow design = lymph::workflow::make(raw_gate, {
    numeric,
    category,
}, flow)
.time({
    .index = "timestamp",
    .horizon = lymph::days(7),
    .gap = lymph::days(1),
    .lookback = lymph::days(365),
});

lymph::Builder candidates = lymph::builder::build({
    {"linear", lymph::models::linear_regression()},
    {"forest", lymph::models::random_forest_regressor({.trees = 300})},
    {"boost", lymph::models::gradient_boost_regressor({.trees = 200, .rate = 0.04})},
});

auto split = lymph::validate::time()
    .folds(5)
    .gap(lymph::days(1))
    .horizon(lymph::days(7));

auto ranked = lymph::spotcheck::models(candidates)
    .run(design, data, split, {
        .metrics = {lymph::Metric::MAE, lymph::Metric::RMSE, lymph::Metric::R2},
        .rank_by = lymph::Metric::RMSE,
        .goal = lymph::Goal::Minimize,
    });

auto tuned = lymph::search::grid({
    {"model.depth", {4, 6, 8}},
    {"model.min_leaf", {10, 25, 50}},
    {"num.fill.strategy", {lymph::Fill::Median, lymph::Fill::Mean}},
})
.run(ranked.top(3), design, data, split, {
    .rank_by = lymph::Metric::RMSE,
    .goal = lymph::Goal::Minimize,
});

lymph::Workflow final = design.with(tuned.best());
final.learn_(data);
Series forecast = final.infer(lymph::payload::make(incoming)
    .features({"temperature", "humidity", "region"})
    .time("timestamp")
    .note(lymph::DataNotePolicy::Factorize));
```

The key rule is the sklearn Pipeline lesson: validation and tuning consume the whole
Workflow design so every fold/window fits preprocessing only on the training slice. The
final Workflow is then the same design plus the selected champion model, trained once on
the accepted training data.

### Family Interop Shape

The same Workflow design can be consumed by the families that should own evaluation:

```cpp
lymph::Workflow design = lymph::workflow::make(raw_gate);
design.on("num", numeric_cols).add(lymph::stage::simple_imputer(median_fill));
design.on("cat", category_cols).add(lymph::stage::one_hot_encoder(unknown_keep));

lymph::Payload data = lymph::payload::make(raw)
    .features({"income", "debt", "region"})
    .label("defaulted");

auto folds = lymph::validate::stratified().folds(5).workers(4);

auto cv = folds.run(design.with(lymph::models::logistic_regression()), data, train, assess);

auto ranked = lymph::spotcheck::models(candidates)
    .strategy("plain", train)
    .run(design, data, folds.options(), assess, lymph::MetricKind::RocAuc);

auto tuned = lymph::search::grid(space, folds.options())
    .run(make_trial_model, design, data, train, assess, lymph::MetricKind::RocAuc);

lymph::Workflow final = design.with(tuned.best());
final.learn_(data, train);

auto table = lymph::reports::confusion_table(final, data);
auto summary = lymph::reports::classification_summary(final, data);
```

This is the separation-of-powers rule in practice: Workflow owns composition, Validate owns
fold scoring, SpotCheck owns candidate ranking, Search owns hyperparameter exploration, and
Reports owns evaluation artifacts.

## Transform And Inverse-Transform Semantics

Every transform stage should expose a small contract:

```cpp
struct TransformContract {
  bool reversible;
  bool inverse_available;
  bool lossy;
  bool stateful;
  bool target_aware;
  bool changes_sample_count;
  bool changes_feature_count;
};
```

Workflow inverse rules:

1. `standardize`, `minmax`, `maxabs`, many power transforms, and fitted quantile maps are
   reversible when fitted state exists.
2. `one_hot_encoder` is conditionally reversible only when category state is complete and
   no unknown bucket or collision makes recovery ambiguous.
3. Imputation is not truly reversible unless Workflow stores a missingness mask in an
   inverse-memory side channel.
4. Feature dropping, selection, and polynomial lifts are generally lossy.
5. Target transforms invert predictions by default for regression-style inference.
6. `workflow.inverse(...)` must fail loudly when a requested inverse is mathematically
   invalid or when required side-channel memory is absent.

Target shape:

```cpp
lymph::Workflow wf = lymph::workflow::make(model);
wf.inverse_memory(lymph::InverseMemory::MasksAndDroppedColumns);

Series mapped = wf.map(raw);
Series restored = wf.inverse(mapped);
Series yhat = wf.infer(raw);          // original target units
Series zhat = wf.infer_space(raw);    // transformed target space, explicit
```

This should outperform sklearn-style inverse handling by making reversibility explicit,
auditable, and failure-oriented instead of relying only on whether a step happens to expose
an inverse method.

## User-Written Transforms And Models

sklearn supports two important extension shapes: callable transforms through
`FunctionTransformer`, and custom estimator classes implementing the estimator/mixin
protocol. Lymph should support both ideas, but in Pluss-native form.

### Callable Stages

Current NodeJ already has `prep::FunctionTransformer`, and Lymph now exposes it as
`stage::custom(...)` for quick Series-native transforms. Workflow should not add a method
per custom use case. The public stage shape is:

```cpp
wf.add(stage);
```

where `stage` can be built from a Lymph built-in, a lambda, an existing free function, a
functor, or a full stateful class contract.

Target ergonomic shape:

```cpp
lymph::Model model = lymph::models::linear_regression();
lymph::Workflow wf = lymph::workflow::make(model);

wf.add(lymph::stage::custom("log_income", log_income_fn));
wf.add(lymph::stage::custom("winsor", Winsor{.lo = 0.02, .hi = 0.98}));
```

First-pass limitation: this is whole-feature-space execution because NodeJ's current
callable transformer receives the `FeatureBatch`'s underlying `Series`. Routed custom
stages such as `wf.on({"income"}).add(stage::custom(...))` are deliberately deferred
until NodeJ can preserve route semantics, feature names, persistence keys, and branch
parallel safety for arbitrary user code.

The first-pass callable signature accepts either `Series(const Series&)` for infallible
Series-native transforms or `StatusOr<Series>(const Series&)` when the user wants to fail
with a Pluss/NodeJ status.

Design rules:

1. Callable stages are for light, local transforms.
2. Existing user functions and classes are first-class; inline lambdas are only the shortest
   example, not the only supported shape.
3. They must operate on `Series`/`FeatureBatch`, not host vectors, unless the user makes an
   explicit CPU escape hatch.
4. They may declare feature-name behavior: one-to-one, generated names, or custom named
   output.
5. They may declare inverse callable support; otherwise they are non-reversible.
6. Callable stages are not assumed persistable. Lambdas and captures are rejected by
   `save(...)` today through NodeJ's opaque-transform guard. Persistable custom stages
   require a future stable registry key and state serializer.

### Named Custom Stages

For reusable and persistable user transforms, Lymph should provide a small value-facing
custom-stage contract rather than asking users to inherit NodeJ internals directly.

Target shape:

```cpp
class WinsorStage {
 public:
  lymph::StageInfo info() const;
  lymph::StageState learn(const lymph::FeatureBatch& x, lymph::TrainOptions options);
  Series map(const lymph::FeatureBatch& x, const lymph::StageState& state) const;
  Series inverse(const lymph::FeatureBatch& x, const lymph::StageState& state) const;
};

lymph::Stage winsor = lymph::stage::custom("winsor", WinsorStage{.lower = 0.01, .upper = 0.99});
```

The public authoring surface should be composition or concepts, not public inheritance from
`TransformerBase`. NodeJ can still adapt the custom stage internally to `TransformerBase`.
This keeps the Lymph user API stable even if NodeJ internals change.

Required custom-stage metadata:

1. Name and stable registry key for persistence.
2. Input/output kind: feature transform, target transform, row filter, sampler, detector,
   scorer, or estimator.
3. Shape contract and sample-count behavior.
4. Feature-name contract.
5. Reversibility contract.
6. State serialization functions.
7. Determinism and random-state handling.
8. Parallel-safety declaration.
9. Time-safety declaration: whether it looks ahead, requires ordering, or consumes windows.

### Custom Models

For full estimator-like extensions, Lymph should avoid a public clone of
`BaseEstimator`/`TransformerMixin`. The Pluss-native approach should be:

```cpp
class MyRegressor {
 public:
  lymph::ModelInfo info() const;
  lymph::ModelState learn(const lymph::LabeledBatch& batch, lymph::TrainOptions options);
  Series infer(const lymph::FeatureBatch& batch, const lymph::ModelState& state) const;
};

lymph::Model model = lymph::models::custom("my_regressor", MyRegressor{});
```

Rules:

1. Construction takes configuration only; data goes to `learn`/`learn_`.
2. Options are structs, preferably enum-based, not string dictionaries.
3. Learned state is separate from configuration and must be serializable if the model is to
   survive Workflow save/load.
4. The model declares capabilities: regression, classification, transform, detector,
   clustering, embedding, semi-compatible, incremental, probabilistic scores, inverse, etc.
5. Workflow dispatch uses declared capabilities instead of fragile duck typing.
6. Search/Builder/Validate consume the resulting `lymph::Model` exactly like built-in
   models.

This gives the same extension power sklearn users get from custom estimator classes, but
without exposing NodeJ inheritance or forcing users into Python-style mixin conventions.

## Workflow Vs Procedure Boundary

Workflow:

- ML composition object.
- Trains and runs chained/branched stages and a final model.
- Owns leakage-safe execution semantics, feature routing, inverse semantics, and fitted
  workflow persistence for experimentation.
- Consumed by validation/search/spotcheck where appropriate.

Procedure:

- Future production system.
- Owns deployment policy, service/runtime packaging, monitoring, drift checks, audit trails,
  version gates, rollback, external I/O contracts, and enterprise operational concerns.

This boundary matters. Workflow should become powerful enough to rival sklearn Pipeline,
but it should not grow into the production orchestration system that Procedure is intended
to become.

## NodeJ Responsibilities

NodeJ should receive any missing kernels and state machinery first: branch composition,
cacheable transform execution, reversible transform metadata, temporal window kernels,
schema validation, workflow-state persistence, custom-stage adapters, and Series-first
transform/inference kernels. Lymph should coordinate those capabilities and present the
compact public surface.

## Design Guardrails

- No sklearn-identical API names when a Pluss-native name is available.
- No public `Pipeline` alias; the public type is `Workflow`.
- No hidden CPU materialization in implementation paths where Series operations can do the
  work.
- No broad public free-function sprawl. Factories create family objects; object work is done
  through methods.
- No demo-only API. Anything added for Workflow must serve general classical ML workflows:
  supervised, unsupervised, semi-supervised, anomaly, feature engineering, validation,
  search, reporting, and deployment.
