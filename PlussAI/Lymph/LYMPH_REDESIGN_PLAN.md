# Lymph Redesign Plan — "mini pluss inside pluss"

Status: **redesign closed**. This file started as a design-only plan and then became the
implementation ledger. The public Lymph surface now follows the family/object redesign;
future work should expand estimator/preprocessing/validation coverage on top of this surface
rather than revive the retired free-function/facade style.

Companion to LYMPH_BLUEPRINT.md, which documents what exists today and why. This
document is the target shape and the plan to get there.

## 1. Design goals

1. Object-based, dot-chainable, the way `pluss::Series`/`pluss::Note`/`pluss::table::DataFrame`
   already work: one namespace of small factory functions constructing real family
   values, then everything else happens through methods and accessors on the value
   you got back.
2. A caller never writes `std::move` to *unwrap a result*. `std::move` still shows up at
   ownership-transfer points the caller writes on purpose, but ordinary model construction,
   result selection, and builder population use cheap-copy `Model` handles.
3. Grouping by *role*: a scaler must be a `Stage`, a regression/classifier/clusterer/
   detector must be a `Model`, a hyperparameter sweep must be a `Search`, a model
   inventory must be a `Builder`, a broad algorithm comparison must be a `SpotCheck`,
   validation plans must live in validation, and a raw-frame-in/prediction-out
   deployment artifact must be a `Workflow`. One namespace of factories per role:
   `lymph::models::`, `lymph::stage::`, `lymph::search::`, `lymph::builder::`,
   `lymph::spotcheck::`, `lymph::validate::`, `lymph::workflow::`, `lymph::columnrouter::`,
   `lymph::transform::`.
4. Value types with an internal tagged representation (mirroring `ModelSpec`/`TransformSpec`
   already in nodej), **not** a new inheritance hierarchy. The abstract-base-class layer
   (`TransformerBase`/`ClassifierBase`/`RegressorBase`) already exists one level down in
   NodeJ and stays exactly where it is, invisible to a lymph caller.
5. No sklearn-identical names, no "sklearn" in code/comments — same discipline already
   applied to `GridAxis`, `selection::compare`, and the `Search` family this session.
6. Scope is general classical ML, not supervised-only. Lymph should cover supervised,
   unsupervised, semi-supervised, anomaly, feature engineering, preprocessing, validation,
   search, calibration, imbalance, reporting, and deployment workflows. Deep learning
   remains the NodeK/Ice domain.
7. Do not solve one downstream by creating a giant convenience object. Families own their
   own work; interop happens through compact shared values. `Builder` coordinates named
   model inventories, but it does not become the home for validation, preprocessing,
   search, metrics, deployment, or reporting.
8. Do not let `Model` become a god object. `Model` owns model-shaped work: attaching
   `Stage`s, fitting, inference/scoring on already-supplied data, model state inspection,
   model persistence, and narrow model-local transforms such as target transforms. It must
   not grow validation planning, broad algorithm comparison, confusion/classification
   reporting, imbalance strategy orchestration, feature selection orchestration, or
   deployment-policy responsibilities. Those belong to their own families and may consume a
   `Model` when doing so creates or runs that family's object.

## 2.0 Construction vs. method rule (settled through chat iteration, binding on everything below)

1. **Fundamental family rule: factories bring objects into existence; methods operate on
   objects already in existence.** A free function/namespaced call is for construction only.
   Once a family object exists, anything naturally done *by* that object is a method, full
   stop. The one accepted-looking exception is not really an exception: another family may
   consume an existing object through a free factory only when that call creates a new object
   of the consuming family. `lymph::workflow::make(champion, policy)` consumes a `Model` and
   `Policy` to create a new `Workflow`; after that, `workflow.save(...)` and
   `workflow.infer(...)` are methods. `lymph::spotcheck::models(builder)` consumes a
   `Builder` to create a `SpotCheck`; after that, `spotcheck.run(...)` is a method.
   Therefore: `prep_policy.apply(frame)` and `spotcheck.run(...)` express existing-object work
   through methods.
1.1. **Derived-object methods are allowed, but they are exceptions, not the default.** A
   method on an existing object may naturally produce a new object, including an object from
   another family, when that result is best understood as being derived from the receiver's
   state or semantics. Pluss already has this pattern (`DataFrame` methods can produce values
   outside the table family, such as catalog-like summaries). Lymph can do the same when it is
   the clearest expression: `model.learn(...) -> Model`, `result.board() -> DataFrame`,
   `workflow.score_table(...) -> DataFrame`, etc. The guardrail is that this should not
   become a back door for scattered free-function style or giant cross-family objects. If the
   caller is merely creating an independent family object, use the lowercase factory. If the
   caller is asking an existing object to derive, summarize, train, score, map, persist, or
   expose something from its own state, a method is appropriate.
1.2. **Internal free functions are allowed; competing public free functions are not.** Existing
   free functions may be kept, moved, renamed, or deleted according to one rule: if they are
   reusable implementation engines, they can live under `detail`/internal headers and power
   the object/family API. If they are public entry points that compete with the redesigned
   family surface, they must be retired once the replacement family object exists. The
   redesign must not preserve scattered public free-function APIs merely because the current
   implementation already has them.
1.3. **Interop does not erase ownership boundaries.** A family may consume another family's
   value only to do its own line of work: `workflow::make(model, policy)` creates a deployable
   `Workflow`; `spotcheck::models(builder)` creates a comparison object; `validate::cross(...)`
   creates a validation plan that can run against a `Model`; a reporting family may consume
   predictions, labels, or a model to produce report values. That consumption does not make
   the consumed family responsible for the consuming family's methods.
2. **Construction always goes through a lowercase namespace, never a static method on the
   type.** `Type::factory()` syntax forces the left of `::` to be the exact class name
   (case-sensitive), so it can never be lowercased — the fix is moving construction off the
   class entirely into a real namespace. `ColumnRouter::names(...)` becomes free functions in
   `lymph::columnrouter::names(...)` / `where(...)` / `remaining_of(...)`; `Workflow::load(...)`
   becomes `lymph::workflow::load(...)`, sibling to `lymph::workflow::make(...)`. No
   exceptions — every constructible lymph type gets a matching lowercase namespace, even ones
   with only one construction shape (`lymph::policy(...)` for the one-shape `Policy` type,
   `lymph::random::seeded(...)` if that ends up worth having at all).
3. **Types named `*Options` are exempt from the factory requirement** — designated-init
   (`{.field = value}`) or default-then-mutate are both fine, no namespace needed
   (`DataFrameBatchOptions`, `AssessOptions`, `TrainOptions`, `InferOptions`,
   `CrossValidationOptions`). They're keyword-argument bags passed *into* a call, not
   first-class values with their own identity that get stored, reused, or passed around on
   their own — `CrossValidationOptions` briefly got a `lymph::cv::stratified_kfold(...)`
   factory over its `plan`/`folds`/`groups` correlation, then got reverted once the naming
   rule made that unnecessary: the `*Options` suffix is the signal, not a
   field-by-field correlation judgment call repeated on every struct.
4. Everything **not** named `*Options` that has correlated/mutually-exclusive fields gets a
   factory (`BalanceTarget` -> `lymph::balance::keep()` / `equalize_reuse_rows(n)` /
   `equalize_blend_neighbors(n)`, since `growth`/`neighbors` only mean anything when
   `kind == Equalize`).
4.1. **Second accepted exemption: flat, uncorrelated non-`*Options` structs** (`GridAxis`,
   `RandomState`) stay designated-init too, same reasoning as rule 3 — every field is
   independent, there is no invalid combination a factory would guard against, so a factory
   wrapper would add a call-site hop with no new information. `BalanceTarget` earns one
   because `kind`/`growth`/`neighbors` are genuinely correlated; `GridAxis`'s `name`/`values`
   and `RandomState`'s `seed`/`deterministic` are not. The full rule, stated once: a factory
   namespace exists to make an invalid combination unrepresentable, or to distinguish several
   real construction shapes for one type (`Model`, `Stage`, `ColumnRouter`) — not as a uniform
   tax on every struct regardless of whether it has anything to guard.
5. **`auto` is reserved for cases where the real type genuinely can't be named at the call
   site**: structured bindings (`for (const auto& [name, value] : list)`) and lambda storage
   (`const auto make_variant = [&](...) {...};`). Every other local gets its real type spelled
   out — `const lymph::FrameHoldoutSplit split = lymph::holdout_split(...)`, not
   `const auto split = ...`.
6. **`Policy`** is the single-shape raw-frame admission/schema type:
   `lymph::Policy raw = lymph::policy({.drop_columns = {...}, .require_columns = {...}, .reject_missing = {"target"}});`.
   It owns raw column admission, required/forbidden checks, explicit row rejection, and
   missingness pass-through for downstream imputers. It does not own imputation, scaling,
   encoding, feature engineering, or balancing. The legacy `filter_nulls` field remains for
   compatibility, but new code should prefer explicit `reject_missing`.

## 2.1 Implementation doctrine for future tranches

1. **NodeJ powers Lymph.** Lymph coordinates, names, composes, and presents a Pluss-native
   surface. Numerically substantive engines, reusable algorithm kernels, split planning,
   estimator internals, vectorizable metrics, balancing mechanics, and persistence primitives
   belong in NodeJ when they are not already there. Lymph should not grow private duplicate
   engines just to make a high-level API work; it should add the missing NodeJ capability,
   then wrap/coordinate it.
2. **Read upstream source and docs for correctness before implementing, but do not copy
   names or surface shape.** The required reference habit for sklearn-adjacent work is:
   read the relevant user-guide page and the matching current source file, then implement
   a Pluss-native design. Useful starting points:
   - `https://scikit-learn.org/stable/user_guide`
   - `https://scikit-learn.org/stable/model_selection`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/model_selection/_split.py`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/model_selection/_validation.py`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/pipeline.py`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/compose/_column_transformer.py`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_data.py`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_function_transformer.py`
   - `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/decomposition/_pca.py`
3. **Performance default: Series ops and vectorization first.** Prefer `Series` operations
   and existing Pluss vectorized kernels over host materialization and native C++ loops.
   Keep data on the input platform unless a CPU fallback is truly unavoidable. When loops
   are needed for metadata or inherently scalar bookkeeping, keep them linear, cache-friendly,
   and C++20-clean; do not hide numerical work in host loops when a Series-level operation
   exists.
4. **GPU/Series access must be first-class.** Any training, validation, search, spot-check,
   metric, or balancing entry point that accepts DataFrame input must also expose a
   `Series`/`FeatureBatch`/`LabeledBatch` path when the underlying task can support it.
   DataFrame supplied data is CPU-only; Series supplied data lets the caller's platform
   choice flow through NodeJ.
5. **Use centralized errors.** Product/runtime code must keep using Pluss error paths
   (`Status`, `StatusOr`, `PLUSS_CHECK`, `PLUSS_FAIL`, `::pluss::throw_error`, or the
   public validate facade where appropriate), not raw `std::runtime_error`.
6. **Do not broaden Lymph with aspirational factories before NodeJ support exists.** If a
   target family needs a real algorithm that NodeJ does not yet carry, implement or extend
   NodeJ first, then expose it through Lymph.

## 2. Target namespace/type map

```
pluss::ai::lymph::

  Model            -- any trained/trainable classical-ML estimator: supervised,
                       unsupervised, semi-supervised, anomaly, ranking/scoring, etc.
                       Carries task + family + options + an ordered list of Stages +
                       optional target transform where that concept applies.
  Stage            -- one preprocessing step attached to a Model: TransformKind + options +
                       (optionally) the ColumnRouter it's routed through. Replaces the
                       option-struct-passed-to-a-builder-method pattern (16 Model
                       methods -> 1: Model::append(Stage, ColumnRouter)). Named `Stage`, not
                       `Prep` as originally sketched -- prep.h already has a real, unrelated
                       `lymph::prep::` namespace (standalone StandardScaler/OneHotEncoder/...
                       objects with their own .learn(frame)/.map(frame), directly usable
                       without any Model at all), discovered only once actual implementation
                       started. Same factory names (standard_scaler(), one_hot_encoder(), ...)
                       would have collided with real, different-return-type functions that
                       already exist -- renamed the whole concept rather than force the
                       collision or make the two systems awkwardly share one name.
  ColumnRouter     -- already built this session (common.h). Unchanged; becomes the
                       standard second argument to Model::append.
  Search           -- a reusable hyperparameter-search configuration: axes + SearchKind +
                       trials + random. Wraps today's free-function GridAxis/TuneOptions
                       pair (selection.h) into one object with .run(...).
  Builder          -- Nexus-inspired coordinator for a named inventory of Models. It owns
                       inject/eject/list/report-style coordination and hands the inventory
                       to other families (spotcheck/search/validate), but does not own
                       validation, search, metrics, preprocessing, deployment, or reporting
                       semantics itself.
  Workflow         -- the raw-frame-in/prediction-out deployable artifact: Policy + Model.
                       This is the object the earlier "final stage that stacks everything"
                       question was actually asking about.
  SpotCheckResult  -- returned by SpotCheck::run(...). .board(), .top(k) ->
                       vector<pair<name, Model>> ranked, .best() -> Model
                       (= top(1).front().second).
  SearchResult     -- returned by Search::run(...). .report, .top(k)/.best() -- same shape as
                       SpotCheckResult, .top(k)'s name is the winning GridPoint's .label().

  models::          -- factories, replacing public linear_factory::/tree_factory::/
                       ensemble_factory::/neighbor_factory::/kernel_factory::/bayes_factory::
                       cluster_factory::/decompose_factory::/online_factory:: spelling
                       (same underlying calls, one namespace): logistic_regression(...),
                       decision_tree_classifier(...), random_forest_classifier(...),
                       gradient_boost_classifier(...), linear_regression(...),
                       decision_tree_regressor(...), online_linear_classifier(...), ...
  prep::           -- UNCHANGED, not part of this redesign: the real, pre-existing
                       lymph::prep:: namespace of standalone fittable transformer objects
                       (StandardScaler, OneHotEncoder, SimpleImputer, ...), each with its own
                       .learn(frame)/.map(frame)/.learn_map(frame) -- usable directly on a
                       DataFrame with no Model involved at all. This is exactly why Stage
                       (above) needed a different name and a different namespace (stage::,
                       below) instead of reusing these factory names for a second, unrelated
                       return type.
                       Distribution-shaping utilities live here too: power maps, quantile
                       maps, robust scaling, row normalization, binning, clipping, and custom
                       function maps. They address non-Gaussian / heavy-tailed / skewed input
                       distributions without creating a sklearn-named public family.
  stage::          -- Model::append()'s actual factory namespace: standard_scaler(),
                       one_hot_encoder(), min_max_scaler(), simple_imputer(), ... returning
                       Stage values. Same option-struct types prep.h's standalone wrappers
                       already use (StandardScalerOptions, OneHotEncoderOptions, ...) -- only
                       the return type and the role differ from lymph::prep::'s same-named
                       functions.
  search::         -- grid(axes), random(axes, trials, random_state)
  builder::        -- build(), build({{"name", model}, ...}), and map/vector registry
                       overloads constructing a Builder. See §3.3
                       for the naming recommendation and construction rules.
  spotcheck::      -- models(builder), strategies(...), etc.; owns broad algorithm/strategy
                       comparison. It consumes Builder inventories rather than forcing
                       Builder to become the comparison engine.
  validate::       -- holdout(...), cross(...), stratified(...), repeated_holdout(...),
                       time_ordered(...), rolling_origin(...). CrossValidationOptions stays
                       as an options bag where needed, but the validation family owns
                       validation-plan values and .run(...) execution.
  reports::        -- confusion_table(...), classification_summary(...),
                       classification_metric_series(...), threshold_curve(...),
                       best_threshold(...). This family consumes a public Model and produces
                       report values; Model does not grow reporting methods.
  transform::      -- log1p(), sqrt(), custom(forward, inverse) -- target-space transform
                       factories attached to Model via `Model::target_transform(...)`.
  select::         -- still allowed for narrow candidate selection helpers where useful, but
                       broad model/strategy comparison belongs to `spotcheck::` and model
                       inventory belongs to `builder::`.
  columnrouter::   -- names(...), where(...), remaining_of(...). Originally static methods on
                       the ColumnRouter class; moved to a real namespace per §2.0 rule 2
                       (Type::factory() can never be lowercased -- the type name after `::`
                       has to match the class exactly).
  policy::         -- lymph::policy({...}) -- Policy's one construction shape.
  balance::        -- keep(), equalize_reuse_rows(n), equalize_blend_neighbors(n) -- guards
                       BalanceTarget's kind/growth/neighbors correlation the way a flat
                       struct literal can't.
  cv::             -- none needed as a public family name. Prefer `validate::` for plan
                       construction; keep `CrossValidationOptions` as a plain *Options bag.
  feature::        -- rank_features(...), f_score_rank(...), f_regression_rank(...) already
                       exist in nodej::feature; carried into lymph unmodified. Per §7 below,
                       new feature-engineering additions (pca, rfe, select_k_best) land as
                       lymph::stage:: factories instead of a separate family, since they're
                       Stages in shape (narrow/transform the feature matrix) even where
                       they need more than the existing X-only stages do.
```

## 2.2 Coverage gaps to close without copying upstream names

The redesign map must cover more than estimators, validation, and reports. Upstream sklearn
is useful as a completeness checklist, but Lymph should expose Pluss-native families with
clear lines of work:

- `prep::` / `stage::`: scaling, imputation, encoding, polynomial lifts, custom functions,
  distribution shaping (`power_map`, `quantile_map`, `robust_scaler`, `vector_normalizer`),
  discretization/binning, and invertible maps where mathematically possible.
- `columnrouter::`: column selection and heterogeneous column routing, consuming
  `Stage` values instead of becoming an estimator/search/validation object.
- `select::`: feature ranking and feature-set reduction, returning feature selections or
  `Stage`s; broad model comparison still belongs to `spotcheck::`.
- `decompose::` / `embed::`: PCA-style decomposition and manifold/embedding work once NodeJ
  carries the actual algorithms.
- `cluster::`, `detect::`, `semi::`, `multiout::`, `calibrate::`, `inspect::`, `reports::`,
  `validate::`, `search::`, `balance::`, and `workflow::`: each owns its own object/result
  types and may consume `Model`, `Builder`, `Stage`, or validation plans only to do its own
  work.

Do not add empty public families just for checklist parity. If the algorithm or vectorizable
primitive is missing, implement it in NodeJ first with Series-aware paths, then expose the
small Lymph factory/method surface.

## 3. What each type looks like in use

### 3.1 Model — build, attach prep, train, predict

```cpp
namespace lymph = pluss::ai::lymph;

lymph::Model m1 = lymph::models::logistic_regression(
    {.regularization = lymph::RegularizationKind::L2, .regularization_strength = 0.02});

m1.append(lymph::stage::standard_scaler(), numeric_router)
  .append(lymph::stage::one_hot_encoder({.max_categories = 128, .include_unknown_bucket = true}),
          categorical_router);

m1.learn_(train_batch, train_options);              // mutates m1 in place (existing verb)
lymph::Series predicted = m1.infer(test_batch);      // existing verb, unchanged
lymph::MetricReport report = m1.assess(test_batch, assess_options);

// The existing non-mutating sibling stays too, unchanged in spirit:
lymph::Model m2 = m1.learn(train_batch, train_options);   // NEW Model, m1 untouched
```

`.append(Stage, ColumnRouter)` is the *one* method replacing today's 16
(`.standard_scaler(...)`, `.one_hot_encoder(...)`, `.min_max_scaler(...)`, ...) because a
`Stage` value already carries everything a stage needs (`TransformKind` + options); the
builder methods only existed because there was no value type to pass instead.

### 3.2 Search — reusable hyperparameter search, now Model-aware

```cpp
const std::vector<lymph::GridAxis> axes = {
    {.name = "regularization_strength", .values = {0.001, 0.01, 0.1, 1.0}},
};
const auto make_model = [&](const lymph::GridPoint& point) {
  lymph::Model m = lymph::models::logistic_regression(
      {.regularization_strength = point.at("regularization_strength")});
  m.append(lymph::stage::standard_scaler(), numeric_router)
   .append(lymph::stage::one_hot_encoder({.max_categories = 128}), categorical_router);
  return m;
};

lymph::CrossValidationOptions cv{};
cv.plan = lymph::CVPlanKind::StratifiedCrossVal;
cv.folds = 3;
cv.shuffle = true;
cv.random_state = 20260721ULL;

lymph::Search search = lymph::search::grid(axes, cv);
lymph::SearchResult tuned = search.run(
    make_model,
    train_batch,
    lymph::TrainOptions{},
    assess_options,
    lymph::MetricKind::RocAuc,
    lymph::TuneObjectivePolicy::Maximize,
    lymph::selection::FitOptions{.fit_top = 3});
lymph::Model best = tuned.best();               // no std::move at the call site
std::vector<std::pair<std::string, lymph::Model>> top3 = tuned.top(3);   // for tie-breaking
```

Deliberate improvement over the old spec-only tuning path: the factory closure now builds a
**full `Model`** (prep stages included), not a bare `ModelSpec`. Today's `GridAxis`-based
search trains on raw, unscaled/un-one-hot-encoded batches (documented as a scope cut in
LYMPH_BLUEPRINT.md §8.2) because `probe_specs` only ever had a `ModelSpec` to build from.
Once `Model` carries its own prep stages, `Search::run` can retrain a full workflow path per
trial instead, closing that gap for free as part of the rename, not as separate work.

### 3.3 Builder + SpotCheck — inventory first, comparison second

```cpp
lymph::Builder built = lymph::builder::build({
    {"weighted_only__logistic", lymph::models::logistic_regression({...})},
    {"weighted_only__tree", lymph::models::decision_tree_classifier({...})},
    {"weighted_only__forest", lymph::models::random_forest_classifier({...})},
});

lymph::Builder grown = lymph::builder::build();
grown.inject("blend_neighbors__logistic", lymph::models::logistic_regression({...}))
     .inject("blend_neighbors__tree", lymph::models::decision_tree_classifier({...}))
     .eject("blend_neighbors__tree");

lymph::ValidationPlan cv = lymph::validate::stratified({
    .folds = 5,
    .repeat = 2,
    .shuffle = true,
    .random_state = 7,
});

lymph::SpotCheck check = lymph::spotcheck::models(built);
lymph::SpotCheckResult compared = check.run(
    train_batch, data_options, cv, train_options, assess_options,
    lymph::MetricKind::RocAuc, lymph::RankGoal::High);

compared.board().title("Cross-Validated Candidate Ranking").show();
lymph::Model champion = compared.best();
std::vector<std::pair<std::string, lymph::Model>> shortlist = compared.top(5);
```

`Builder` is the coordinator/inventory family, taking its cue from `pluss::table::Nexus`.
It must support both construction-time population and later `.inject()` / `.eject()`.
The comparison work itself belongs to `spotcheck::`, `validate::`, and `search::`, which
consume the Builder instead of being absorbed by it.

**Decision on names:** require explicit names for all Builder entries for now. Do not
support unnamed injection in the initial redesign and do not silently base important output
on `model_0`, `model_1`, etc. Names are used in boards, diagnostics, duplicate handling,
result selection, persistence manifests, and reproducible reports; index names become
unstable as soon as the user reorders or removes a candidate. If unnamed scratch entries
later become necessary, revisit the design deliberately and take the cue from Nexus rather
than adding an ad hoc auto-counter.

### 3.4 Workflow — the actual "stack everything, raw data in" object

```cpp
lymph::Workflow deployed = lymph::workflow::make(champion, prep_policy);
deployed.save(bundle_path, raw_train_frame, data_options);

lymph::Workflow reloaded = lymph::workflow::load(bundle_path);
lymph::Series predicted = reloaded.infer(raw_incoming_frame);   // raw schema, zero cleanup at call site
```

`Workflow` is the finalized deployment artifact: raw cleanup policy, routed stages, frozen
categorical maps, invertible feature/target transforms when present, trained model, and
inference/scoring behavior.

### 3.5 ColumnRouter — unchanged, already built

```cpp
lymph::ColumnRouter categorical = lymph::columnrouter::names({"gender", "A2", "A3"});
lymph::ColumnRouter numeric = lymph::columnrouter::remaining_of(categorical);
```

Ships as-is from this session; the redesign only changes what consumes it
(`Model::append` instead of a bare `feature_route` vector).

### 3.6 TargetTransform — folded into Model

Target transforms are optional `Model` state, set via a chainable method, so there is one
model object type:

```cpp
lymph::Model m3 = lymph::models::linear_regression({...});
m3.target_transform(lymph::transform::log1p());   // optional, chainable
m3.learn_(train_batch);
lymph::Series predicted = m3.infer(test_batch);    // already inverse-transformed
```

### 3.7 Invertible transforms — forward mapping must remember how to come back

Scaling and other invertible transforms belong to the preprocessing/transform families,
not to `Builder` or `Workflow` directly. Standalone prep objects expose an inverse path
when the operation is mathematically invertible:

```cpp
lymph::prep::MinMaxScaler scaler = lymph::prep::minmax_scaler({.range = {0.0, 1.0}});

pluss::table::DataFrame scaled = scaler.learn_map(frame);
pluss::table::DataFrame restored = scaler.inverse(scaled);
```

When the same transform is attached as a `Stage`, fitted state is carried by the learned
`Model` and then by the deployed `Workflow`. Feature-space inverse is exposed only for
debugging/inspection paths that ask for it; prediction remains in the user's expected
space. Target transforms are different: `Model::infer(...)` and `Workflow::infer(...)`
return predictions on the original target scale by default, with an explicit transformed-
space accessor only for diagnostics.

## 4. The std::move rule, precisely

- **Gone**: `std::move` needed just to get a value *out* of something you already hold
  (today's `std::move(compared.best model handle)`). `Model` is cheap-copy, so
  `result.best()` and `result.top(k)` are enough for keeping winners around.
- **Stays, and is fine**: `std::move` at a point *you* choose to transfer ownership — into
  a candidate list, into a `Workflow` constructor, into a container. That is normal C++,
  not API friction, and no design should try to hide it.

## 5. Step-by-step migration plan

Each phase keeps the full NodeJ/Lymph test suite green (currently 23 tests) before moving
to the next; nothing lands half-working.

**Progress** (updated as work actually lands, not aspirationally):
- Skeleton for all 8 new per-family files created (comment-only headers + 3 `.cpp` stubs for
  `search`/`select`/`workflow`'s real dispatch logic), `api.h` module map updated to list them
  as "not yet wired in." 23/23 tests green with the skeleton alone.
- **`columnrouter.h` landed for real** (ahead of the numbered phases below, as the lowest-risk
  first cut): `ColumnRouter` moved out of `common.h`; construction moved off static methods
  onto `lymph::columnrouter::names/where/remaining_of` per rule 2. `ColumnRouter` had exactly
  one real call site (`lymph_column_router_test.cpp`, built this session) so this was a clean
  cut, not a staged alongside-the-old-API migration — updated directly, no transitional dual
  surface. `common.h` left a one-line pointer comment where the class used to live. 23/23
  tests still green.
- **`balance.h` landed for real.** One correction against the earlier design-doc sketch: the
  real `BalanceTarget` (nodej/options.h) has four `BalanceTargetKind` values
  (Keep/Equalize/Ratio/Counts) and cleaning fields (`clean`/`clean_neighbors`/`clean_vote`/
  `protect_small_classes`), not just Keep/Equalize -- caught by reading the actual struct
  before implementing rather than the two kinds this session's own chat examples had been
  using. Scoped `lymph::balance::` to `keep()`/`equalize_reuse_rows(n)`/
  `equalize_blend_neighbors(n)` -- the three strategies actually exercised anywhere in this
  session's demos -- and documented Ratio/Counts/cleaning as a disclosed scope cut (raw
  `BalanceTarget{...}` literals still work if needed) rather than either silently dropping
  them or over-building factories for untested combinations. New test
  (`lymph_balance_factory_test.cpp`) covers all three factories plus the default-argument
  case. 24/24 tests green.
- **`stage.h` + `Model::append(Stage, ColumnRouter)` + `model.h` landed for real.** The single
  biggest piece so far, three real findings along the way:
  1. **Naming collision, caught before writing code, not after**: `prep.h` already has a real
     `lymph::prep::` namespace (`standard_scaler()`, `one_hot_encoder()`, ...) returning
     standalone fittable transformer objects with their own `.learn(frame)`/`.map(frame)` --
     completely unrelated to what this document had been calling `Prep`. Same names would have
     collided outright. Renamed the whole append()-target concept to `Stage`
     (`lymph::stage::standard_scaler()`, ...) rather than force the collision or bury one
     system behind the other -- see §2's `Stage`/`prep::`/`stage::` entries for the full
     writeup.
  2. **`.append()` can't resolve its `ColumnRouter` at call time** -- it's always called before
     any `DataFrame` exists, and `ColumnRouter::resolve()` needs a frame + column-name order.
     Resolution is deferred: `Model` gained a `pending_stages_` member (a
     `vector<pair<Stage, ColumnRouter>>`), `.append()` just queues onto it, and the two
     DataFrame-aware `learn_(...)` overloads (`(frame, data_options)`, `(x, y)`) call a new
     `materialize_pending_stages_()` that resolves each router against the real column names
     and folds the result into `workflow_` via `append_transform_from_spec_()` -- an existing,
     already-tested private dispatcher `clone_for_out_of_place_()` already used, reused rather
     than duplicated. The bare `learn_(LabeledBatch)` overload has no column names at all, so
     it throws a clear error if any stage is still pending instead of silently skipping
     routing. `clone_for_out_of_place_()` was also updated to carry `pending_stages_` over, so
     the non-mutating `.learn(...)` sibling (which clones first) still resolves correctly.
  3. **`using SomeClass::static_method;` at namespace scope is not legal C++** ("using
     declaration cannot refer to class member") -- tried it first to consolidate the
     implementation factory structs into `lymph::models::` with zero hand-copied signatures,
     and the compiler rejected every line. Fell back to one-line forwarding wrapper functions
     (`inline Model logistic_regression(LinearFamilyOptions o={}) { return
     detail::linear_factory::logistic_regression(o); }`) for all current entries -- still
     nothing to keep in sync but the function name itself, just more lines than hoped. Where
     several families already share a name (`regressor`/`classifier`/`transformer`, each
     taking a different `*FamilyOptions` type), the wrappers become ordinary overloads
     resolved by argument type. The implementation factory structs are now under
     `lymph::detail`; public code uses `lymph::models`.
  `Model` has since become a real cheap-copy public handle over the internal model core, so
  the alias step is historical only. New test (`lymph_model_append_test.cpp`) covers: stages not materializing until
  `learn_()` sees a frame, a real noiseless dataset only separable if the attached
  scaler/one-hot-encoder actually ran, the bare-`LabeledBatch`-with-pending-stages throw, and
  the non-mutating `.learn(...)` clone path resolving correctly. 25/25 tests green.
- **Old `Workflow` lazy facade retired from `selection.h`** (phase 4, partial -- `selection.h`
  itself is done; the four downstream callers that still build `Workflow{...}` aggregate
  literals directly, `workflow.h`'s actual class removal, and `pipeline.h`'s relocation are
  still open, see the pending-tasks note below this list). `Candidate.workflow: Workflow` ->
  `Candidate.model: Model`; `CompareResult.best_workflow: Workflow` ->
  `CompareResult.best_model`; `.fitted: vector<Workflow>` -> `vector<Model>`. One real
  finding along the way: **`Model` has no default constructor** (only
  `explicit Model(const ModelSpec&)` and `explicit Model(Workflow)`), unlike
  the old `Workflow` facade whose `std::optional<Model> model{}` member made an empty
  `Workflow` trivially default-constructible. `Candidate::model` doesn't need a default value
  at all (its one constructor always sets it via the member-initializer-list, so the field is
  declared with no `{}`), but `CompareResult::best_model` genuinely does -- `compare_batch_`
  default-constructs `CompareResult result{};` well before the actual winner is known partway
  through the function. Wrapped `best_model` in `std::optional<Model>`, kept `.best()`
  as the public accessor that unwraps it (throwing "no best model has been computed" if called
  before it's set) so callers never see the optional themselves.
- **`Builder` landed for real** (Nexus-inspired named model coordinator, requested alongside
  the Comparison -> Benchmark rename -- `builder.h`, new type, not one of the originally
  numbered phases). One real finding, verified directly against clang++ before committing to a
  design: **a braced list of `(name, Model)` pairs cannot populate a
  `std::vector<std::pair<std::string, Model>>`** while `Model` was still move-only, because
  `std::initializer_list`'s backing array is permanently `const` and `std::vector`'s
  initializer-list constructor can only copy out of it. That limitation is now gone at the
  public Lymph level: `Model` is a cheap-copy handle, so `builder::build({{"name", model}, ...})`
  works with lvalues, prvalues, maps, and vectors without caller-written `std::move`.
  Duplicate names throw rather than
  overwrite, matching `Nexus::inject(key, frame)`'s real behavior (`NexusInternals.inc`'s
  `append_named_frame` -- `PLUSS_CHECK`'s a duplicate key is an error), not the "last write
  wins" this document had first sketched. `.list(n)` stayed the plain
  `vector<string>` preview; the rich `DataFrame` sibling went through two names in review --
  `.summary()`, then briefly `.describe(n)` -- before landing on **`.report(n)`**, since
  `DataFrame` already has its own unrelated `.describe()` (summary statistics), and reusing
  that name for a different meaning on `Builder` was worth avoiding. `.benchmark(...)` clones
  every held model into fresh `selection::Candidate`s and forwards
  to `selection::compare(...)` -- non-destructive, and removes the "list your candidates by
  hand" step entirely, as intended; `.candidates()` is exposed separately for callers who want
  to drop down to `selection::compare(...)` directly. New test
  (`lymph_builder_test.cpp`) covers inject/eject/has/names/list/report, the duplicate-name
  throw, and a real end-to-end `.benchmark(...)` spot-check across 3 models. 26/26 tests green.

  Redesign follow-up landed: `Builder` now also supports construction-time population,
  not only empty construction plus `.inject()`. The public path is a Nexus-like brace-list
  factory with cheap-copy `Model` handles:
  `lymph::builder::build({{"forest", model}, {"tree", lymph::models::decision_tree_classifier({...})}})`.
  Explicit names are mandatory; `.inject()` / `.eject()` remain the mutation path after construction.

- **`lymph::models::` is the public consolidated model factory namespace.** The existing
  `linear_factory`/`tree_factory`/... structs moved under `lymph::detail` because they remain
  useful implementation identifiers and already encode family-specific defaults, but they are
  no longer the public Lymph construction spelling. New user code calls
  `lymph::models::random_forest_regressor(...)`, `lymph::models::knn_regressor(...)`,
  `lymph::models::pca(...)`, and the online factories
  `lymph::models::online_linear_classifier(...)` /
  `lymph::models::online_linear_regressor(...)`. The temporary singular
  `lymph::model::` alias has been removed.
- **Phase 3 public-surface cleanup started.** `Model` is no longer exported as the
  user-facing estimator type: the old implementation header moved under
  `detail/model_core.h`, the implementation type is `lymph::detail::ModelCore`, and public
  construction/loading is through `lymph::Model`. Model-backed classification reports and
  threshold helpers moved behind the `lymph::reports::` family so `Model` stays focused on
  model behavior and reporting does not become another set of scattered public free functions.
- **Phase 4 result API direction corrected.** `selection::CompareResult` and `SearchResult`
  already hold public `Model`s directly after the earlier Workflow-facade retirement. The
  public result surface should stay lean: `.best()` for the leading model and `.top(k)` for
  the top-k models practitioners may want to keep exploring before choosing a champion. Do
  not add ownership-drain result methods unless a concrete future use case proves
  `.best()`/`.top(k)` insufficient.
- **Phase 5 Search family advanced.** `Search` is now the public object for model-aware
  hyperparameter search: factories under `lymph::search::` create it, and `.run(...)` executes
  against `LabeledBatch`, `(DataFrame, Series)`, `(DataFrame, Thread)`, or raw-frame plus
  `DataFrameBatchOptions`. Search factories accept either `SplitOptions` or
  `CrossValidationOptions`; when constructed with `CrossValidationOptions`, the object keeps
  that CV intent and resolves task-aware reliability defaults at run time, so classification
  searches still auto-stratify like the Builder/selection path. `search::build(axes,
  TuneOptions)` bridges the old axis/options pair into the new object surface while preserving
  `split`, `trials`, `random`, and `max_parallel` intent. The execution boundary remains
  `Search::run(make_model, data, ...)`, where the trial factory builds a full `Model`
  including stages; `selection::tune_specs` remains the lower-level ModelSpec/NodeJ escape
  hatch, not the recommended Lymph workflow path.

**`Workflow` retirement completed.** The real caller count was 6, not 4 (a repo-wide grep
before deleting anything found `bike_sharing_end_to_end.cpp` and `lymph_metrics_test.cpp` too,
missed in the earlier scoping) -- `nodej_cv_tune_test.cpp`, `nodej_workflow_tuning_demo.cpp`,
`bike_sharing_end_to_end.cpp`, `lymph_housing_ch2_end_to_end_notebook.cpp`,
`lymph_end_to_end_hour_workflow_showcase.cpp` (~43 `Workflow{.model=..., .stages={...}}`
aggregate-literal blocks alone, migrated via a small Python brace-matching script rather than
by hand, then verified by a full rebuild), and `lymph_metrics_test.cpp`. Every real caller only
ever used methods `Model` already had directly (`.trained()`, `.infer()`,
`.infer_scores()`, `.assess()`, `.learn_()`, `.save()`, `coeff`/`intercept` fields, plus the
free-function `confusion_table`/`classification_summary`/`classification_metric_series`/
`cross_validate*` overloads that already accept `const Model&` directly) -- so retiring
the facade turned out to be pure mechanical substitution, not new engineering:
`Workflow{.model = X, .stages = {workflow::stage::foo(a,b), ...}}` became `auto m = X; m.foo(a,
b); ...` (each `workflow::stage::` DSL factory already takes the exact same `(options,
feature_route)` signature as the matching `Model` builder method, confirmed before
scripting the transform), and `Workflow::load(path)` became `Model::load(path)`
(already a real static method Workflow's own `.load()` just forwarded to).

The old `class Workflow`, `make_pipeline(...)`/`pipeline(...)`, `TrainedHandle`/
`AnyTrainedModel`, and the `cross_validate*(const Workflow&, ...)` overloads were deleted from
`workflow.h` once the repo-wide grep confirmed zero remaining references; the matching
`Workflow`-specific `pluss::io::save`/`load` overloads in `io_bridge.h` were deleted too (a
real, separate dependency this file's own grep caught -- a generic model-persistence bridge,
not itself listed in the original 4/6-caller scoping).

**NodeJ now owns Flow clone semantics.** `nodej::Flow::clone()` rebuilds non-opaque flows from
their `ModelSpec` and serializable `TransformSpec`s, reloads fitted transform/model state when
the source is trained, preserves stage metadata, and rejects opaque callable transforms or
model-backed flows with no `ModelSpec`. `Model::clone()` now delegates to that NodeJ
method instead of carrying a private Lymph-side clone recipe. This is the backend foundation
for the phase-3 physical `Model` redesign: `Model` can become a cheap-copy handle with
copy-on-write detach powered by `Flow::clone()`, instead of faking copyability in Lymph.

`Workflow`/`Policy` landed as real public families. The old pipeline skeleton and frame-prep
compatibility wrappers are gone; `workflow.h` owns the deployable raw-frame artifact,
`policy.h` owns cleanup semantics, and `workflow::make(...)` / `workflow::load(...)` are the
construction entry points.

Full regression: 26/26 AI tests pass, plus a full rebuild of every touched example target
(`plussai_nodej_workflow_tuning_demo`, `plussai_bike_sharing_end_to_end`,
`plussai_lymph_housing_ch2_end_to_end_notebook`, `plussai_lymph_end_to_end_hour_workflow_showcase`).

1. **Baseline freeze. Completed.** The Lymph-focused tests and touched examples have been
   used as the regression bar during the redesign.
2. **`Stage` value type + `Model::append(Stage, ColumnRouter)`. Completed.** Stage values are
   the attachable preprocessing descriptors for `Model`.
3. **Hide the old workflow-model implementation behind `Model`. Completed.** The implementation
   lives under `lymph::detail::ModelCore`; public consumers use `lymph::Model`.
4. **Retire the old lazy Workflow facade and normalize result APIs. Completed.** Results hold
   public `Model` handles and expose `.best()` plus `.top(k)`.
5. **`Search` object wrapping `GridAxis`/`TuneOptions`. Completed.** `Search::run` takes a
   `(GridPoint) -> Model` factory and supports task-aware split/CV resolution.
6. **Consolidate factory structs into `lymph::models::`. Completed.** Batch model construction
   now goes through `lymph::models::`, online model construction uses
   `models::online_linear_*`, old factory structs are internal `detail` tags, and the singular
   `lymph::model` alias is gone.
7. **Fold `TargetTransform` into `Model`. Completed.** `Model::target_transform(...)` owns the
   target-space transform; the wrapper class is gone.
8. **Make `Workflow`/`Policy` the final deployment surface. Completed.** `Policy` owns raw-frame
   cleanup, `Workflow` owns raw-frame learn/infer/assess/save/load, and the old frame-prep
   compatibility names have been removed.

## 6. Decisions on the three open questions

### 6.1 `.append()` called twice for the same logical stage — stack, always. No implicit replace.

Default to **stack**, matching `Flow::add_column_stage`'s existing (already-tested,
already-relied-on) behavior of just appending. The normal case *is* two different stages
on two different routes (`.append(standard_scaler, numeric)` +
`.append(one_hot_encoder, categorical)` — the exact shape every candidate in this session's
own demos already builds) — the "duplicate stage" case is the rare, degenerate one, and
guessing intent from a kind+route match is a new *implicit* mutation this codebase doesn't
do anywhere else. `Model` already exposes explicit, named, targeted mutation
(`set_stage_key_`, `set_stage_route_`, `set_stage_remainder_`) rather than pattern-matching
side effects — if a caller genuinely wants to replace a stage, that stays an explicit,
separate operation (a `.replace(index, Stage, ColumnRouter)` or `.remove(index)` +
`.append(...)` pair), never something `.append()` infers on its own. A cheap, optional
addition: a debug-mode diagnostic when `.append()` sees an exact kind+route duplicate,
surfacing a likely mistake without changing behavior — worth having, not worth blocking on.

### 6.2 Search retrain-per-trial cost — build the simple version first; the real fix is a loop reorder, not a warning label

Two real options, not one:

- **Option A (simple)**: each trial independently rebuilds and refits the *entire* `Model`
  (prep + terminal model) from scratch. Correct, and consistent with how broad comparison
  dispatch already works today (each candidate already independently refits its own full
  workflow path per fold) — so this is not a new kind of cost, just the same cost `Search`
  didn't previously pay because it never had prep stages to run.
- **Option B (optimized)**: hoist prep-fitting *out* of the trial loop and into the fold
  loop — for a given fold's train partition, fit prep once, transform once, then vary only
  the terminal model/hyperparameter across all of that fold's trials. Strictly more work to
  build (the current `probe_specs`/(trial × fold) dispatch shape doesn't distinguish
  "prep-invariant" from "hyperparameter-varying" pieces at all), and it still has to respect
  per-fold CV hygiene (each fold's prep must only ever see that fold's own train partition,
  never global statistics) — so it is a real restructuring, not a small patch.

Recommendation: **build Option A for the redesign pass.** It's correct, it's consistent
with `compare()`'s existing cost model, and — per this session's own standing instruction
not to chase hypothetical optimizations — Option B should stay a documented, deliberate
follow-up, built only if profiling on a real workload actually shows prep cost dominating
trial cost. Premature loop-reordering here risks the same mistake this project has
avoided everywhere else (see LYMPH_BLUEPRINT.md's own performance notes, which measure
before optimizing, never the other way around).

### 6.3 Builder entries holding a `Search` — out of scope entirely, not deferred

Removed from the roadmap rather than kept as a future phase. Two reasons:

1. **It doesn't need a new type.** The "search each family's hyperparameters, then compare
   the winners" workflow is already fully reachable by composing Phases 1-6 as they stand:
   run `Search` once per family *outside* the comparison (five small searches, each picking
   that family's own best hyperparameter), then hand the five resulting tuned `Model`s into
   a `Builder` and let `spotcheck::models(builder)` compare them as ordinary fixed
   candidates. No nested Builder-entry/Search type is required — this is the same "compose
   small, single-purpose objects" principle already chosen over one mega-object for the
   `Model`/`Stage`/`Search`/`Workflow` split itself.
2. **The real cost is combinatorial, and the data model doesn't fit today's shape.** A
   comparison where even a few of 15 candidates are themselves full searches multiplies out
   to candidates x folds x trials, which is exactly the "heavy, takes time" concern flagged
   before any of this GridAxis work started. `.board()`/`SpotCheckResult` are shaped around
   one score per (candidate, fold) today; scoring nested searches would need a third axis,
   a bigger change than anything else in this plan. Not worth taking on speculatively.

If the two-step composed pattern turns out to be genuinely painful in practice once the
redesign ships, that's real evidence to revisit this — not something to build ahead of it.

## 7. Folder structure, NodeJ's role, and Stage-housed feature engineering (built as lymph::stage:: entries)

### 7.1 Folder structure

`PlussAI/Lymph/` (the `.cpp` side) is genuinely shallow today — `prep.cpp` and
`workflow_dsl.cpp` only, because almost everything is header-only. The headers themselves are
already split per-concern (15 files under `PlussAI/include/pluss/ai/lymph/`); the redesign
should extend that same split to the new names and let real logic move into matching `.cpp`
files as it earns a place there:

```
PlussAI/include/pluss/ai/lymph/
  model.h, prep.h, columnrouter.h, policy.h, transform.h,
  balance.h, search.h, builder.h, spotcheck.h,
  validate.h, workflow.h                             <- one header per family
  common.h, metrics.h, api.h                          <- shared/umbrella, as today

PlussAI/Lymph/
  search.cpp     <- Search::run's (trial x fold) dispatch (see 7.2)
  spotcheck.cpp  <- broad model/strategy comparison dispatch
  validate.cpp   <- validation plan execution helpers
  workflow.cpp   <- deployable artifact save/load
  prep.cpp       <- existing, grows with pca/rfe/select_k_best (see 7.3)
```

### 7.2 What NodeJ already supports vs. what's new lymph-level work

Checked rather than assumed. Most of the redesign is pure resurfacing of engines already
built and tested this session — `Model`/`Stage`/`SpotCheck`/`BalanceTarget`/`TargetTransform`
wrap `Flow`/`ModelSpec`/`TransformSpec`/`selection::compare`/`BalanceTarget`/
`nodej::regression_metrics` with no NodeJ changes needed. `ColumnRouter` and `Policy` never
touched NodeJ in the first place.

**`Search` is the one real exception.** NodeJ's `probe_specs`/`ProbeSpecSampler` searches over
bare `ModelSpec`s — no notion of an attached prep workflow path. Making `Search::run()` retrain a
full `Model` (prep included) per trial means the (trial × fold) dispatch has to live at the
lymph level, mirroring `SpotCheck`'s own candidate×fold dispatch rather than delegating to
NodeJ's spec-only `tune()`. Real, scoped lymph-level work — not a NodeJ change, but not "just a
wrapper" either.

**Current NodeJ non-supervised inventory, checked in source before documenting this plan:**

- `families.h` exposes `TaskKind::Transform`, `ModelFamily::Cluster`,
  `ModelFamily::Decompose`, `ClusterMethod::KMeans`, `DecomposeMethod::Pca`, and
  `KernelForm::OneClass`.
- `builders.h` exposes `make_transformer(const ModelSpec&)`, and
  `family_transformer.cpp` wires transform-task construction for Cluster, Decompose, and
  Kernel(one-class).
- `prep.h` / `prep.cpp` expose `OutlierIsolator`, `FunctionTransformer`,
  `FeatureTransformer`, scalers, imputers, encoders, quantile/power maps, threshold
  binarization, vector normalization, and polynomial lifting.
- `nodej_unsupervised_demo.cpp` demonstrates KMeans and PCA through the transform path, and
  `nodej_reliability_io_unsupervised_test.cpp` covers KMeans, PCA, and one-class kernel
  persistence/reliability paths.

So Lymph can honestly expose Pluss-native front doors for KMeans/PCA/one-class/outlier
work once the naming and family boundaries are settled. Broader unsupervised and
semi-supervised catalogs are not automatically present just because they are in the target
vision. If Lymph wants a new clustering, manifold, density, covariance, graph-label, or
semi-supervised family, the real algorithm should land in NodeJ first, with Series-aware
execution, then Lymph should coordinate it.

### 7.3 Is NodeJ staying public?

Yes, deliberately not privatized. Precedent already exists in the codebase for this exact
shape: `pluss::codem::` sits underneath `pluss::Series` the same way NodeJ sits underneath
lymph, and CodeM is not hidden — it's a real, documented, directly-usable layer that `Series`
is the *convenient* front door for, not the *only* door. Two reasons to keep the same shape
here:

- NodeJ is already a large, tested, actively-used public surface (many files this session call
  it directly) — privatizing it is a much bigger and riskier undertaking than "redesign lymph,"
  for no functional gain.
- `Search` is deliberately scoped to skip `Bayesian`/`SuccessiveHalving`, with the explicit
  advice "call `probe_specs` directly with a custom sampler" (§6.2/§3.2). That escape hatch
  only exists if NodeJ stays public — privatizing it now would silently contradict a decision
  already on the books.

Lymph becomes the documented, recommended, 95%-of-the-time surface. NodeJ stays available
underneath for what lymph deliberately doesn't cover yet.

### 7.4 New stage:: entries for feature engineering

Three sklearn-adjacent capabilities the user asked to add, checked against what's already
available rather than assumed buildable:

- **`lymph::stage::pca({.n_components = k})`** (dimensionality reduction). `Series::svd()`/
  `svd_u`/`svd_s`/`svd_vh` already exist at the CodeM level — this is a wrapper over existing
  linear algebra, no new math. Same shape as every other unsupervised, X-only `Stage` stage
  today (scalers, encoders) — the easy case.
- **`lymph::stage::rfe(...)`** (recursive feature elimination). `Model::
  linear_parameters()`/`.coefficients()` and `permutation_impact()` already exist to rank
  importance; RFE is "fit, rank, drop the weakest, repeat" on top of those. The real
  complication: RFE needs the *label*, and a probe model, inside what's currently an X-only
  prep stage — every existing `Stage` only ever sees a `FeatureBatch` during `.learn_()`, never
  `y`. Making RFE a real Stage means teaching that one stage type to see labels, which
  today's `TransformKind`/`Flow` dispatch doesn't support for *any* stage yet. Real, scoped
  engineering — not hard, but not "just call the existing function" the way `pca` is.
- **`lymph::stage::select_k_best(...)`** (hypothesis-test-based feature selection). Partially
  already built and worth knowing: `f_score_rank`/`f_regression_rank` (`feature_select.h`) are
  already F-test hypothesis testing (ANOVA F-test / F-regression) — wrapping one as a `Stage`
  stage closes the sklearn `SelectKBest(f_classif)`/`SelectKBest(f_regression)` gap directly.
  Same supervised-stage caveat as `rfe` applies. Broader hypothesis testing (chi-square, mutual
  information) isn't built — real gap, not urgent unless a specific need for it shows up.

## 8. `Lymph/src/` given a real role; GPU/Series-direct entry points on compare()/Builder

Two small, independent landings, both user-requested follow-ups after the `Workflow`
retirement + `Builder` work above.

**`Lymph/src/` was dead** -- present in the tree, not globbed by `Lymph/CMakeLists.txt` (which
only picked up `Lymph/*.cpp`), and not referenced anywhere in this document. Checked the actual
precedent this whole redesign already leans on for structure -- `PlussMic/src/table/*.cpp`
backs `PlussMic/include/pluss/table/*.h` -- and found `Lymph/*.cpp` (`prep.cpp`,
`workflow_dsl.cpp`, `search.cpp`, `select.cpp`, `workflow.cpp`) was *already* playing that exact
role, just one directory level too shallow, with an empty, unwired `src/` sitting uselessly
alongside it. Moved all five into `Lymph/src/`, pointed the CMake glob at `Lymph/src/*.cpp`
instead of `Lymph/*.cpp`, and fixed every `// See Lymph/foo.cpp` pointer comment in the
matching headers. No new convention invented -- just made the existing one apply at the right
depth. Full rebuild + 26/26 tests confirmed the move didn't disturb anything.

**GPU/Series-direct entry points.** `selection::compare(...)` and `Builder::benchmark(...)`
only had DataFrame-taking public overloads, and `DataFrame` is deliberately CPU-only -- so
there was no public path to spot-check models against GPU-resident data. Checked before
assuming a gap existed: `LabeledBatch` (`nodej/dataset.h`) is nothing but
`{FeatureBatch{Series x}, Series y, Series sample_weights}` -- no DataFrame anywhere in its
definition -- and `compare_batch_`, the shared body every existing `compare()` overload already
forwards to, already takes a `const LabeledBatch&` directly. `Model::learn_(LabeledBatch
...)` already exists too (exercised by `lymph_model_append_test.cpp`'s bare-batch throw case).
So the actual gap was narrow: `compare()`/`benchmark()` just never *exposed* the LabeledBatch
entry point publicly. Added one `selection::compare(candidates, const LabeledBatch&, ...)`
overload (a one-line forward to `compare_batch_`, mirroring how the DataFrame overloads
already forward to it) and the matching `Builder::benchmark(const LabeledBatch&, ...)`. No
"target platform" argument needed or added -- NodeJ's model cores dispatch on the tensor's own
device, so a `Series` built with `Config{}.with_platform(pluss::pGPU)` trains/evaluates on GPU
straight through this path; the platform is a property of the data handed in, not a policy
knob to thread through every signature. New coverage in `lymph_builder_test.cpp`: builds a
`LabeledBatch` via the same `cross_validate_labeled_from_xy_(...)` helper the DataFrame
overloads already use internally (not a hand-rolled one), runs both
`builder.benchmark(batch, ...)` and `selection::compare(builder.candidates(), batch, ...)`
directly, and checks they produce the same trial count as the DataFrame path. 26/26 tests
green.
