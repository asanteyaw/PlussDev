# Lymph Redesign Plan — "mini pluss inside pluss"

Status: **design only, not started**. Per explicit sequencing from the user: finish
end-to-end functionality first (done — see LYMPH_BLUEPRINT.md §8/§9), *then* do this
as its own staged pass. Nothing in this document has been implemented yet.

Companion to LYMPH_BLUEPRINT.md, which documents what exists today and why. This
document is the target shape and the plan to get there.

## 1. Design goals

1. Object-based, dot-chainable, the way `pluss::Series`/`pluss::Note`/`pluss::table::DataFrame`
   already work: one namespace of small factory functions constructing a handful of real
   value types, then everything else happens through methods and accessors on the value
   you got back.
2. A caller never writes `std::move` to *unwrap a result*. `std::move` still shows up at
   ownership-transfer points the caller writes on purpose (building a candidate list,
   handing a `Model` into a `Pipeline`) — that is normal C++ and stays. What goes away is
   the awkward two-hop `std::move(x.workflow_model())` dance the current `Workflow` facade
   forces on every caller who wants to hold onto a trained model past the call that
   produced it.
3. Grouping by *role*, the same intuition sklearn gives a reader for free: a scaler must be
   a `Stage`, a regression must be a `Model`, a grid search must be a `Search`, a raw-frame-in/
   prediction-out artifact must be a `Pipeline`. One namespace of factories per role,
   `lymph::model::`, `lymph::stage::`, `lymph::search::`, `lymph::route::` (already exists as
   `ColumnRouter`), `lymph::transform::`, `lymph::select::`.
4. Value types with an internal tagged representation (mirroring `ModelSpec`/`TransformSpec`
   already in nodej), **not** a new inheritance hierarchy. The abstract-base-class layer
   (`TransformerBase`/`ClassifierBase`/`RegressorBase`) already exists one level down in
   NodeJ and stays exactly where it is, invisible to a lymph caller.
5. No sklearn-identical names, no "sklearn" in code/comments — same discipline already
   applied to `GridAxis`/`selection::compare`/`selection::tune` this session.

## 2.0 Construction vs. method rule (settled through chat iteration, binding on everything below)

1. **A free function/namespaced call is only for bringing an object into existence** — even
   one that takes existing objects as ingredients (`lymph::pipeline::from(champion, policy)`
   builds a *new* `Pipeline` consuming an existing `Model`; that's still construction, not an
   operation on `champion`). Once an object exists, anything that's naturally *its own*
   operation is a method, full stop — no free function ever takes an already-existing lymph
   object as its first argument to do something *to* it. (`prep_policy.apply(frame)`, not
   `apply_frame_prep(frame, prep_policy)`; `comparison.run(...)`, not
   `select::compare(candidates, ...)`.)
2. **Construction always goes through a lowercase namespace, never a static method on the
   type.** `Type::factory()` syntax forces the left of `::` to be the exact class name
   (case-sensitive), so it can never be lowercased — the fix is moving construction off the
   class entirely into a real namespace. `ColumnRouter::names(...)` becomes free functions in
   `lymph::columnrouter::names(...)` / `where(...)` / `remaining_of(...)`; `Pipeline::load(...)`
   becomes `lymph::pipeline::load(...)`, sibling to `lymph::pipeline::from(...)`. No
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
6. **`Policy`** (short for what this document originally called `FramePrepPolicy`) is the
   single-shape frame cleanup type: `lymph::Policy prep_policy = lymph::policy({.drop_columns
   = {...}, .filter_nulls = true});`. Renamed for brevity — "any short, one-word descriptive
   name," per direct instruction — everywhere `FramePrepPolicy` appears below should be read
   as `Policy`.

## 2. Target namespace/type map

```
pluss::ai::lymph::

  Model            -- any trained/trainable model: task + family + options + an ordered
                       list of Stages + optional TargetTransform. Replaces WorkflowModel
                       (renamed) and retires the Workflow lazy facade.
  Stage            -- one preprocessing step attached to a Model: TransformKind + options +
                       (optionally) the ColumnRouter it's routed through. Replaces the
                       option-struct-passed-to-a-builder-method pattern (16 WorkflowModel
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
  Pipeline         -- the raw-frame-in/prediction-out deployable artifact: Policy + Model.
                       Renames today's PreparedModel (frame_prep.h) -- this is the object the
                       earlier "final stage that stacks everything" question was actually
                       asking about.
  SelectionResult  -- returned by Comparison::run(...). .board(), .top(k) -> vector<pair<name,
                       Model>> ranked, .best() -> Model (= top(1).front().second).
  SearchResult     -- returned by Search::run(...). .report, .top(k)/.best() -- same shape as
                       SelectionResult, .top(k)'s name is the winning GridPoint's .label().

  model::          -- factories, replacing linear_factory::/tree_factory::/ensemble_factory::
                       (same underlying calls, one namespace): logistic_regression(...),
                       decision_tree_classifier(...), random_forest_classifier(...),
                       gradient_boost_classifier(...), adaboost_classifier(...),
                       linear_regression(...), decision_tree_regressor(...), ...
  prep::           -- UNCHANGED, not part of this redesign: the real, pre-existing
                       lymph::prep:: namespace of standalone fittable transformer objects
                       (StandardScaler, OneHotEncoder, SimpleImputer, ...), each with its own
                       .learn(frame)/.map(frame)/.learn_map(frame) -- usable directly on a
                       DataFrame with no Model involved at all. This is exactly why Stage
                       (above) needed a different name and a different namespace (stage::,
                       below) instead of reusing these factory names for a second, unrelated
                       return type.
  stage::          -- Model::append()'s actual factory namespace: standard_scaler(),
                       one_hot_encoder(), min_max_scaler(), simple_imputer(), ... returning
                       Stage values. Same option-struct types prep.h's standalone wrappers
                       already use (StandardScalerOptions, OneHotEncoderOptions, ...) -- only
                       the return type and the role differ from lymph::prep::'s same-named
                       functions.
  search::         -- grid(axes), random(axes, trials, random_state)
  transform::      -- log1p(), sqrt(), custom(forward, inverse) -- today's
                       TargetTransform factories (target_transform.h), unchanged in
                       spirit, just grouped under this namespace and attached to Model
                       via a method instead of a separate wrapping class (see §3.6).
  select::         -- candidates(candidate_list) -> Comparison, an object matching Search's
                       own shape (construct once, call .run(...) to get a SelectionResult).
                       Originally scoped as a plain function (a candidate list looked like a
                       one-shot comparison, not reusable config) -- reversed once the
                       construction-vs-method rule (see §2.0) made the asymmetry with Search
                       something a caller would have to remember rather than predict.
  columnrouter::   -- names(...), where(...), remaining_of(...). Originally static methods on
                       the ColumnRouter class; moved to a real namespace per §2.0 rule 2
                       (Type::factory() can never be lowercased -- the type name after `::`
                       has to match the class exactly).
  policy::         -- lymph::policy({...}) -- Policy's one construction shape (renamed from
                       FramePrepPolicy, see §2.0 rule 6).
  balance::        -- keep(), equalize_reuse_rows(n), equalize_blend_neighbors(n) -- guards
                       BalanceTarget's kind/growth/neighbors correlation the way a flat
                       struct literal can't.
  cv::             -- none needed -- CrossValidationOptions is *Options-suffixed and exempt
                       (§2.0 rule 3), plain designated-init throughout.
  feature::        -- rank_features(...), f_score_rank(...), f_regression_rank(...) already
                       exist in nodej::feature; carried into lymph unmodified. Per §7 below,
                       new feature-engineering additions (pca, rfe, select_k_best) land as
                       lymph::stage:: factories instead of a separate family, since they're
                       Stages in shape (narrow/transform the feature matrix) even where
                       they need more than the existing X-only stages do.
```

## 3. What each type looks like in use

### 3.1 Model — build, attach prep, train, predict

```cpp
namespace lymph = pluss::ai::lymph;

lymph::Model m1 = lymph::model::logistic_regression(
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
  lymph::Model m = lymph::model::logistic_regression(
      {.regularization_strength = point.at("regularization_strength")});
  m.append(lymph::stage::standard_scaler(), numeric_router)
   .append(lymph::stage::one_hot_encoder({.max_categories = 128}), categorical_router);
  return m;
};

lymph::Search search = lymph::search::grid(axes);
lymph::SearchResult tuned = search.run(make_model, train_batch);
lymph::Model best = tuned.best();               // no std::move at the call site
std::vector<std::pair<std::string, lymph::Model>> top3 = tuned.top(3);   // for tie-breaking
```

Deliberate improvement over today's `selection::tune`: the factory closure now builds a
**full `Model`** (prep stages included), not a bare `ModelSpec`. Today's `GridAxis`-based
search trains on raw, unscaled/un-one-hot-encoded batches (documented as a scope cut in
LYMPH_BLUEPRINT.md §8.2) because `probe_specs` only ever had a `ModelSpec` to build from.
Once `Model` carries its own prep stages, `Search::run` can retrain a full pipeline per
trial instead, closing that gap for free as part of the rename, not as separate work.

### 3.3 select::candidates / Comparison — rank whole models, not hyperparameters

```cpp
// instead of candidates being a vector, why not a std::map
std::unordered_map<std::string_view, lymph::select::Candidate> candidates = std::unordered_map<std::string_view, lymph::select::Candidate>{
   {.name = "weighted_only__logistic", .model = std::move(m1), .options = train_options},
   {.name = "weighted_only__decision_tree", .model = std::move(m_tree), .options = train_options}
}
// or better still a model cordinator, like how pluss::table::Nexus cordinates a collection of dataframes
std::vector<lymph::select::Candidate> candidates;
candidates.push_back({.name = "weighted_only__logistic", .model = std::move(m1), .options = train_options});
candidates.push_back({.name = "weighted_only__decision_tree", .model = std::move(m_tree), .options = train_options});
// ...

lymph::Benchmark comparison = lymph::benchmark::candidates(std::move(candidates));
lymph::BenchmarkResult compared = comparison.run(
    train_batch, data_options, cv_options, {}, assess_options,
    lymph::MetricKind::RocAuc, lymph::TuneObjectivePolicy::Maximize);

compared.board().title("Cross-Validated Candidate Ranking").show();
lymph::Model champion = compared.best();     // no std::move needed
std::vector<std::pair<std::string, lymph::Model>> top3 = compared.top(3);
```

`Comparison` matches `Search`'s exact shape (`lymph::X obj = lymph::x::factory(...); obj.run(...)`)
— this was originally designed as a plain function (`select::compare(candidates, ...)`), on the
reasoning that a candidate list looked like a one-shot thing, not reusable config the way a
`Search` axis list is. Reversed once it became clear that asymmetry is something a user has to
*remember*, not something they can predict from the rest of the API.

The `std::move(m1)` in `emplace_back` is a normal, expected ownership transfer the caller
writes on purpose (same as today) — that is not the pattern being eliminated.
`compared.take_champion()` is: no unwrapping a lazy facade, no reaching through
`.workflow_model()`, no `std::move` at all.

### 3.4 Pipeline — the actual "stack everything, raw data in" object

```cpp
lymph::Pipeline deployed = lymph::pipeline::from(champion, prep_policy);
deployed.save(bundle_path, raw_train_frame, data_options);

lymph::Pipeline reloaded = lymph::pipeline::load(bundle_path);
lymph::Series predicted = reloaded.infer(raw_incoming_frame);   // raw schema, zero cleanup at call site
```

Renamed from today's `PreparedModel` (`frame_prep.h`) — same object, same
`Policy` sidecar persistence, new name matching the role it actually plays. This
is the direct answer to "the final stage for deployment is the pipeline that stacks
everything" from earlier in this conversation.

### 3.5 ColumnRouter — unchanged, already built

```cpp
lymph::ColumnRouter categorical = lymph::columnrouter::names({"gender", "A2", "A3"});
lymph::ColumnRouter numeric = lymph::columnrouter::remaining_of(categorical);
```

Ships as-is from this session; the redesign only changes what consumes it
(`Model::append` instead of a bare `feature_route` vector).

### 3.6 TargetTransform — folded into Model instead of a separate wrapper class

Today's `TargetTransformedModel` (`target_transform.h`) is a second class wrapping
`WorkflowModel`. In the redesign it becomes an optional field on `Model` itself, set via a
chainable method, so there is one object type instead of two:

```cpp
lymph::Model m3 = lymph::model::linear_regression({...});
m3.target_transform(lymph::transform::log1p());   // optional, chainable
m3.learn_(train_batch);
lymph::Series predicted = m3.infer(test_batch);    // already inverse-transformed
```

## 4. The std::move rule, precisely

- **Gone**: `std::move` needed just to get a value *out* of something you already hold
  (today's `std::move(compared.best_workflow.workflow_model())`). Every result type gets a
  named `.take_*()` consuming accessor instead.
- **Stays, and is fine**: `std::move` at a point *you* choose to transfer ownership — into
  a candidate list, into a `Pipeline` constructor, into a container. That is normal C++,
  not API friction, and no design should try to hide it.

## 5. Step-by-step migration plan

Each phase keeps the full NodeJ/Lymph test suite green (currently 23 tests) before moving
to the next; nothing lands half-working.

**Progress** (updated as work actually lands, not aspirationally):
- Skeleton for all 8 new per-family files created (comment-only headers + 3 `.cpp` stubs for
  `search`/`select`/`pipeline`'s real dispatch logic), `api.h` module map updated to list them
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
     Resolution is deferred: `WorkflowModel` gained a `pending_stages_` member (a
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
     declaration cannot refer to class member") -- tried it first to consolidate
     `linear_factory::`/`tree_factory::`/.../`decompose_factory::` (all `struct`s with
     `static` methods) into `lymph::model::` with zero hand-copied signatures, and the compiler
     rejected every line. Fell back to one-line forwarding wrapper functions
     (`inline Model logistic_regression(LinearFamilyOptions o={}) { return
     linear_factory::logistic_regression(o); }`) for all ~40 entries -- still nothing to keep
     in sync but the function name itself, just more lines than hoped. Where several families
     already share a name (`regressor`/`classifier`/`transformer`, each taking a different
     `*FamilyOptions` type), the wrappers become ordinary overloads resolved by argument type.
  `Model` itself is `using Model = WorkflowModel;` (phase 3's alias step, not a physical rename
  yet). New test (`lymph_model_append_test.cpp`) covers: stages not materializing until
  `learn_()` sees a frame, a real noiseless dataset only separable if the attached
  scaler/one-hot-encoder actually ran, the bare-`LabeledBatch`-with-pending-stages throw, and
  the non-mutating `.learn(...)` clone path resolving correctly. 25/25 tests green.
- **Old `Workflow` lazy facade retired from `selection.h`** (phase 4, partial -- `selection.h`
  itself is done; the four downstream callers that still build `Workflow{...}` aggregate
  literals directly, `workflow.h`'s actual class removal, and `pipeline.h`'s relocation are
  still open, see the pending-tasks note below this list). `Candidate.workflow: Workflow` ->
  `Candidate.model: WorkflowModel`; `CompareResult.best_workflow: Workflow` ->
  `CompareResult.best_model`; `.fitted: vector<Workflow>` -> `vector<WorkflowModel>`. One real
  finding along the way: **`WorkflowModel` has no default constructor** (only
  `explicit WorkflowModel(const ModelSpec&)` and `explicit WorkflowModel(Workflow)`), unlike
  the old `Workflow` facade whose `std::optional<WorkflowModel> model{}` member made an empty
  `Workflow` trivially default-constructible. `Candidate::model` doesn't need a default value
  at all (its one constructor always sets it via the member-initializer-list, so the field is
  declared with no `{}`), but `CompareResult::best_model` genuinely does -- `compare_batch_`
  default-constructs `CompareResult result{};` well before the actual winner is known partway
  through the function. Wrapped `best_model` in `std::optional<WorkflowModel>`, kept `.best()`
  as the public accessor that unwraps it (throwing "no best model has been computed" if called
  before it's set) so callers never see the optional themselves.
- **`Builder` landed for real** (Nexus-inspired named model coordinator, requested alongside
  the Comparison -> Benchmark rename -- `builder.h`, new type, not one of the originally
  numbered phases). One real finding, verified directly against clang++ before committing to a
  design: **a braced list of `(name, Model)` pairs cannot populate a
  `std::vector<std::pair<std::string, Model>>`**, the same way `Nexus`'s own constructor takes
  a braced list of `(name, DataFrame)` pairs -- `std::initializer_list`'s backing array is
  permanently `const`, so `std::vector`'s `initializer_list` constructor can only *copy* out of
  it, and `Model` (== `WorkflowModel`) has no copy constructor. Confirmed with a standalone
  move-only-type repro (`std::vector<std::pair<std::string, MoveOnly>> v{ {"a", make(1)} };`)
  before writing any real code: fails with "call to implicitly-deleted copy constructor",
  regardless of whether the elements are prvalues built inline or already-named lvalues -- a
  hard language limitation, not a design choice to route around. So `Builder` is constructed
  empty via `builder::build()` and grown one model at a time via a chainable, mutating
  `.inject(name, model)` -- which also happens to be exactly the fluent chain already wanted
  for verbosity reasons, independent of this limitation. Duplicate names throw rather than
  overwrite, matching `Nexus::inject(key, frame)`'s real behavior (`NexusInternals.inc`'s
  `append_named_frame` -- `PLUSS_CHECK`'s a duplicate key is an error), not the "last write
  wins" this document had first sketched. `.list(n)` stayed the plain
  `vector<string>` preview; the rich `DataFrame` sibling went through two names in review --
  `.summary()`, then briefly `.describe(n)` -- before landing on **`.report(n)`**, since
  `DataFrame` already has its own unrelated `.describe()` (summary statistics), and reusing
  that name for a different meaning on `Builder` was worth avoiding. `.benchmark(...)` clones
  every held model (`WorkflowModel::clone()`) into fresh `selection::Candidate`s and forwards
  to `selection::compare(...)` -- non-destructive, and removes the "list your candidates by
  hand" step entirely, as intended; `.candidates()` is exposed separately for callers who want
  to drop down to `selection::compare(...)`/`tune(...)` directly. New test
  (`lymph_builder_test.cpp`) covers inject/eject/has/names/list/report, the duplicate-name
  throw, and a real end-to-end `.benchmark(...)` spot-check across 3 models. 26/26 tests green.

**`Workflow` retirement completed.** The real caller count was 6, not 4 (a repo-wide grep
before deleting anything found `bike_sharing_end_to_end.cpp` and `lymph_metrics_test.cpp` too,
missed in the earlier scoping) -- `nodej_cv_tune_test.cpp`, `nodej_workflow_tuning_demo.cpp`,
`bike_sharing_end_to_end.cpp`, `lymph_housing_ch2_end_to_end_notebook.cpp`,
`lymph_end_to_end_hour_workflow_showcase.cpp` (~43 `Workflow{.model=..., .stages={...}}`
aggregate-literal blocks alone, migrated via a small Python brace-matching script rather than
by hand, then verified by a full rebuild), and `lymph_metrics_test.cpp`. Every real caller only
ever used methods `WorkflowModel` already had directly (`.trained()`, `.infer()`,
`.infer_scores()`, `.assess()`, `.learn_()`, `.save()`, `coeff`/`intercept` fields, plus the
free-function `confusion_table`/`classification_summary`/`classification_metric_series`/
`cross_validate*` overloads that already accept `const WorkflowModel&` directly) -- so retiring
the facade turned out to be pure mechanical substitution, not new engineering:
`Workflow{.model = X, .stages = {workflow::stage::foo(a,b), ...}}` became `auto m = X; m.foo(a,
b); ...` (each `workflow::stage::` DSL factory already takes the exact same `(options,
feature_route)` signature as the matching `WorkflowModel` builder method, confirmed before
scripting the transform), and `Workflow::load(path)` became `WorkflowModel::load(path)`
(already a real static method Workflow's own `.load()` just forwarded to).

The old `class Workflow`, `make_pipeline(...)`/`pipeline(...)`, `TrainedHandle`/
`AnyTrainedModel`, and the `cross_validate*(const Workflow&, ...)` overloads were deleted from
`workflow.h` once the repo-wide grep confirmed zero remaining references; the matching
`Workflow`-specific `pluss::io::save`/`load` overloads in `io_bridge.h` were deleted too (a
real, separate dependency this file's own grep caught -- a generic model-persistence bridge,
not itself listed in the original 4/6-caller scoping).

`pipeline.h`'s skeleton (never implemented -- `PreparedModel`/`FramePrepPolicy` -> `Pipeline`/
`Policy`, phase 8, genuinely new engineering) was merged into `workflow.h`'s header comment
with `Pipeline` renamed to `Workflow` throughout (reusing the name just freed) and `pipeline.h`
deleted; `Lymph/pipeline.cpp` renamed to `Lymph/src/workflow.cpp` (see the later `Lymph/src/`
note further down for why it moved under `src/`). This is a *plan* relocation only
-- no `Pipeline`/`Policy` logic exists yet; phase 8 is still fully unstarted and deserves its
own explicit go-ahead before landing, not something to infer from this cleanup. One namespace
note for whoever picks phase 8 up: `lymph::workflow::` already holds `workflow::stage::`/
`workflow::feature::` (workflow_dsl.h's branch DSL) -- adding `workflow::from(...)`/
`workflow::load(...)` factory functions at the same namespace's top level is fine (nested
namespaces and free functions coexist without collision), just worth knowing going in.

Full regression: 26/26 AI tests pass, plus a full rebuild of every touched example target
(`plussai_nodej_workflow_tuning_demo`, `plussai_bike_sharing_end_to_end`,
`plussai_lymph_housing_ch2_end_to_end_notebook`, `plussai_lymph_end_to_end_hour_workflow_showcase`).

1. **Baseline freeze.** No behavior changes yet — confirm the 23-test suite is the
   regression bar for every phase below.
2. **`Stage` value type + `Model::append(Stage, ColumnRouter)`**, added *alongside* the
   existing 16 builder methods (which start delegating to `.append` internally rather than
   being removed yet). Lets every existing caller keep compiling while `Stage` gets proven
   out.
3. **Rename `WorkflowModel` -> `Model`** via a type alias first (`using Model =
   WorkflowModel;`), so nothing downstream breaks immediately; physically rename the class
   once call sites have migrated.
4. **Retire the `Workflow` lazy facade.** Change `CompareResult`/the tune result types to
   hold `Model` directly (not `Workflow`), add `.take_champion()`/`.take_best()`. This is
   the change that actually removes the `std::move(x.workflow_model())` pattern.
5. **`Search` object wrapping `GridAxis`/`TuneOptions`**, `Search::run` upgraded to take a
   `(GridPoint) -> Model` factory (closing the prep-stage-free search gap from §3.2 as part
   of the move, not as separate follow-up work).
6. **Consolidate `linear_factory::`/`tree_factory::`/`ensemble_factory::`** into
   `lymph::model::`. Same functions, one namespace; update every in-repo call site.
7. **Fold `TargetTransform` into `Model`** as an optional field + chainable method; retire
   `TargetTransformedModel` as a separate class.
8. **Rename `PreparedModel` -> `Pipeline`, `FramePrepPolicy` -> `Policy`.** Sidecar persistence
   are unchanged.
9. **Sweep every example, test, and downstream demo** (`loan_mod_lymp.cpp`,
   `stripped_down_lymph.cpp`, `nodej_workflow_tuning_demo.cpp`,
   `lymph_housing_ch2_end_to_end_notebook.cpp`,
   `lymph_end_to_end_hour_workflow_showcase.cpp`) onto the new surface. No backward-compat
   aliases kept afterward — this is a single-consumer codebase (this project and its one
   downstream thesis project), so carrying two parallel surfaces indefinitely is pure
   maintenance cost with no one left depending on the old one.

## 6. Decisions on the three open questions

### 6.1 `.append()` called twice for the same logical stage — stack, always. No implicit replace.

Default to **stack**, matching `Flow::add_column_stage`'s existing (already-tested,
already-relied-on) behavior of just appending. The normal case *is* two different stages
on two different routes (`.append(standard_scaler, numeric)` +
`.append(one_hot_encoder, categorical)` — the exact shape every candidate in this session's
own demos already builds) — the "duplicate stage" case is the rare, degenerate one, and
guessing intent from a kind+route match is a new *implicit* mutation this codebase doesn't
do anywhere else. `WorkflowModel` already exposes explicit, named, targeted mutation
(`set_stage_key_`, `set_stage_route_`, `set_stage_remainder_`) rather than pattern-matching
side effects — if a caller genuinely wants to replace a stage, that stays an explicit,
separate operation (a `.replace(index, Stage, ColumnRouter)` or `.remove(index)` +
`.append(...)` pair), never something `.append()` infers on its own. A cheap, optional
addition: a debug-mode diagnostic when `.append()` sees an exact kind+route duplicate,
surfacing a likely mistake without changing behavior — worth having, not worth blocking on.

### 6.2 Search retrain-per-trial cost — build the simple version first; the real fix is a loop reorder, not a warning label

Two real options, not one:

- **Option A (simple)**: each trial independently rebuilds and refits the *entire* `Model`
  (prep + terminal model) from scratch. Correct, and consistent with how `select::compare`'s
  own candidate dispatch already works today (each candidate already independently refits
  its own full pipeline per fold) — so this is not a new kind of cost, just the same cost
  `Search` didn't previously pay because it never had prep stages to run.
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

### 6.3 `select::Candidate` holding a `Search` — out of scope entirely, not deferred

Removed from the roadmap rather than kept as a future phase. Two reasons:

1. **It doesn't need a new type.** The "search each family's hyperparameters, then compare
   the winners" workflow is already fully reachable by composing Phases 1-6 as they stand:
   run `Search` once per family *outside* the comparison (five small searches, each picking
   that family's own best hyperparameter), then hand the five resulting tuned `Model`s into
   `select::compare(...)` as ordinary fixed candidates. No nested `Candidate`/`Search` type
   required — this is the same "compose small, single-purpose objects" principle already
   chosen over one mega-object for the `Model`/`Stage`/`Search`/`Pipeline` split itself.
2. **The real cost is combinatorial, and the data model doesn't fit today's shape.** A
   comparison where even a few of 15 candidates are themselves full searches multiplies out
   to candidates x folds x trials, which is exactly the "heavy, takes time" concern flagged
   before any of this GridAxis work started. `.board()`/`SelectionResult` are shaped around
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
  balance.h, search.h, select.h, pipeline.h          <- one header per family
  common.h, metrics.h, api.h                          <- shared/umbrella, as today

PlussAI/Lymph/
  search.cpp     <- Search::run's (trial x fold) dispatch (see 7.2)
  select.cpp     <- Comparison::run's dispatch
  pipeline.cpp   <- save/load
  prep.cpp       <- existing, grows with pca/rfe/select_k_best (see 7.3)
```

### 7.2 What NodeJ already supports vs. what's new lymph-level work

Checked rather than assumed. Most of the redesign is pure resurfacing of engines already
built and tested this session — `Model`/`Stage`/`Comparison`/`BalanceTarget`/`TargetTransform`
wrap `Flow`/`ModelSpec`/`TransformSpec`/`selection::compare`/`BalanceTarget`/
`nodej::regression_metrics` with no NodeJ changes needed. `ColumnRouter` and `Policy` never
touched NodeJ in the first place.

**`Search` is the one real exception.** NodeJ's `probe_specs`/`ProbeSpecSampler` searches over
bare `ModelSpec`s — no notion of an attached prep pipeline. Making `Search::run()` retrain a
full `Model` (prep included) per trial means the (trial × fold) dispatch has to live at the
lymph level, mirroring `Comparison`'s own candidate×fold dispatch rather than delegating to
NodeJ's spec-only `tune()`. Real, scoped lymph-level work — not a NodeJ change, but not "just a
wrapper" either.

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
- **`lymph::stage::rfe(...)`** (recursive feature elimination). `WorkflowModel::
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
forwards to, already takes a `const LabeledBatch&` directly. `WorkflowModel::learn_(LabeledBatch
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
