# Lymph Blueprint (sklearn-shaped ergonomics layer over NodeJ)

## 0. Elevator pitch

Lymph's entire value proposition is the same one word Ice claims for NodeK: **convenience**.
Lymph exists so that a classical-ML task reads like sklearn — `Pipeline` + `cross_validate` +
`GridSearchCV` + `classification_report`, in that spirit — instead of like hand-rolled NodeJ
plumbing. Today it delivers on that promise for the *middle* of a workflow (build a model, add
prep stages, cross-validate, tune among fully-specified candidates, assess) but not the *edges*
(routing raw DataFrame columns into a model, tuning heterogeneous per-candidate options, picking an
operating threshold, turning results into something you can look at). Those edges are exactly
where `EnterpriseBankIntel/src/modeling/loan_mod_lymp.cpp` — a real downstream consumer — falls
back to hand-written NodeJ-style loops, defeating the reason lymph exists. This blueprint closes
that gap.

## 1. Positioning (unchanged, restated for reference)

- `NodeJ`: classical ML engine — raw estimators, splitters, metrics, optimizers. Low-level,
  `Status`/`StatusOr` everywhere, full control expected, manual work is the *design*, not a bug.
- `Lymph`: convenience layer over NodeJ — workflow-oriented `learn`/`infer`/`assess`, DataFrame-first
  ergonomics, one-call factories, cross-validation/tuning helpers, `resolve()`-unwrapped exceptions
  instead of `Status` plumbing.

**Rule for this blueprint**: any new capability that is *numerically heavy, vectorizable, or a
reusable algorithm* (threshold-curve sweep, candidate×fold scheduling, neighbor search) is
implemented in NodeJ as a `Series`-vectorized primitive returning `StatusOr<T>`, with Lymph adding
only a thin `resolve()`-wrapped, DataFrame-friendly, exception-throwing façade on top — exactly the
pattern every existing lymph function already follows (`common.h`'s `using namespace
::pluss::ai::nodej;` plus one-line `resolve(nodej::foo(...), "foo")` wrappers). Anything that is
*pure orchestration/ergonomics* with no real computation (name→index resolution, a leaderboard
`DataFrame` view, an options struct with a nicer default) stays in Lymph only — pushing it into
NodeJ would just be relocating boilerplate, not deleting it.

## 2. Evidence

### 2.1 From `loan_mod_lymp.cpp` (real downstream code, this session)

- Manual `is_categorical_route` lambda + two hand-built `std::vector<int64_t>` index lists just to
  tell `standard_scaler`/`one_hot_encoder` which columns are which (lines 228–244).
- Abandoned `selection::tune` for a hand-rolled nested loop over `balance_specs × candidate_names`
  because `TrainOptions` is one shared struct for every candidate and per-candidate calibration
  isn't expressible (lines 424–448, 643–660; the downstream author's own `lymph_api_feedback.md`
  says this explicitly).
- Hand-written 17-step threshold grid search re-running `classification_summary` at each step
  (lines 665–684) instead of a single vectorized sweep.
- Three separate manual `std::vector` + `push_back` + `DataFrame({...})` assemblies to turn results
  into a table (cv ranking, holdout summary, strategy leaders).
- A hand-rolled `to_i64_vec`/`to_f64_vec` lambda pair because `Series → std::vector` has no public
  one-liner.
- `plan_balance(y_train, ...)` silently requires the `LabeledBatch` overload for
  `BlendNeighbors`/cleaning but nothing before runtime tells you that — the actual bug fixed
  earlier in this session.

### 2.2 sklearn comparison — what lymph is missing that sklearn users would expect

Organized by sklearn module. "Must-have" = common-path functionality every non-trivial classical-ML
script needs; "nice-to-have" = real but not blocking for the common case.

| sklearn concept | Lymph today | Verdict |
|---|---|---|
| `ColumnTransformer` / `make_column_selector` | `feature_route` is integer-index-only everywhere in `workflow.h`/`prep.h` | **Must-have** |
| `GridSearchCV(estimator, param_grid)` with per-candidate params | `selection::tune` takes one shared `TrainOptions` for all candidates | **Must-have** |
| `cross_validate(...)` → `pd.DataFrame(cv_results)` | `CrossValidationReport`/`TuneReport`/`selection::Trial` have no DataFrame view | **Must-have** |
| `precision_recall_curve` + manual argmax for threshold | No vectorized threshold sweep; only single-threshold `classification_summary` | **Must-have** |
| `np.asarray(series)` / trivial array conversion | No public `Series → std::vector` helper (not even lymph's problem — see §3) | **Must-have** (wrong layer) |
| `Pipeline.get_feature_names_out()` | `WorkflowModel` tracks `feature_names_` internally but never exposes it | **Nice-to-have** |
| `VotingClassifier` / `StackingClassifier` | No ensemble-of-`WorkflowModel`s type; `selection::tune` picks one winner, doesn't combine | **Nice-to-have** |
| `CalibratedClassifierCV` (wrap *any* fitted estimator) | Calibration exists but only as a per-model `TrainOptions.calibration` fit-time knob (Platt/Isotonic/Temperature — actually broader than sklearn's two methods) | **Nice-to-have** (current design is arguably fine, just not universal) |
| `learning_curve` / `validation_curve` | Not present — no built-in "vary train size" or "vary one hyperparameter and re-CV" helper | **Nice-to-have** |
| `sklearn.base.clone(estimator)` | `WorkflowModel::clone_for_out_of_place_()` exists but is private; `selection::tune`'s own comment (selection.h:260–271) flags a redundant retrain because of this | **Must-have** (already diagnosed, cheap fix) |
| `KFold`/`StratifiedKFold`/`TimeSeriesSplit`/`GroupKFold` | Unified `CVPlanKind` enum (`Holdout`/`CrossVal`/`StratifiedCrossVal`/`TSCrossVal`/`GroupCrossVal`) | **Already better than sklearn** — one options struct instead of five splitter classes |
| `n_jobs=-1` fold parallelism | `CrossValidationOptions.max_parallel` + `std::async` fold dispatch | **Already present**, but see §5.1 for a real scheduling gap |
| `class_weight="balanced"` / `sample_weight` | `TrainOptions.class_weight`, `.sample_weights` | **Already present** |
| imbalanced-learn `Pipeline` + `SMOTE` | `BalanceTarget`/`plan_balance` with `ReuseRows`/`BlendNeighbors`/cleaning | **Already present**, ergonomics gap only (§2.1 last bullet) |
| `permutation_importance`, PDP (`partial_dependence`) | `inspection::permutation_impact`, `dependence_curve`/`dependence_surface` | **Already present** |
| `SelectKBest`/`VarianceThreshold`/`RFE`-ish | `feature::variance_prune`, `rank_features`, `pick_features`, `f_score_rank` | **Already present** |
| `warm_start` / `partial_fit` | `TrainOptions.warm_start`; `lymph::online.h` (`OnlineLinearClassifier/Regressor`) | **Already present** |
| Full estimator catalog breadth | `linear_factory`/`tree_factory`/`ensemble_factory`/`neighbor_factory`/`kernel_factory`/`bayes_factory`/`cluster_factory`/`decompose_factory` | **Already comparable** to sklearn's core zoo |

**Bottom line**: the *modeling* core (estimators, splitters, metrics, feature selection,
inspection) is already sklearn-comparable or better. The gap is entirely in the **glue a user
writes around that core** — routing, per-candidate tuning, thresholding, and results-to-table — the
exact set of things `loan_mod_lymp.cpp` had to hand-roll. That's a narrow, well-defined blueprint,
not a rewrite.

## 3. Design principles for this blueprint

1. **Fix the edges, not the middle.** Do not touch `WorkflowModel`'s fluent builder, `factories.h`,
   or the estimator catalog — they're already good. Every item below is additive: new free
   functions/overloads/struct fields, zero breaking changes to existing call sites.
2. **NodeJ gets the algorithm, Lymph gets the ergonomics** (§1's rule). Concretely for this
   blueprint: the threshold sweep and the candidate×fold parallel scheduler are NodeJ-level
   `Series`-vectorized primitives; column-name routing and the results-table view are Lymph-only
   (they're host-side metadata bookkeeping, not numerical work — putting them in NodeJ would just
   move string-matching code one layer down for no reason).
3. **`Series → std::vector` is not lymph's or NodeJ's problem.** It's a generic core-Series gap
   (PlussQit), out of scope here — flagged so it isn't lost, not silently absorbed into lymph as a
   workaround.
4. **Naming: short, descriptive, pluss-native, matching the existing lymph vocabulary.** This
   codebase's established convention (checked against `cross_validate`, `classification_summary`,
   `confusion_table`, `holdout_split`, `dependence_curve`, `variance_prune`, `rank_features`,
   `f_score_rank`) is two-word `verb_noun`/`noun_verb` snake_case — not single cutesy words, not
   sklearn's verbatim names (`fit`/`transform`/`GridSearchCV`). New names below were checked against
   every existing symbol in `pluss::ai::{nodej,lymph}` for collisions (`tune`, `sweep`, `probe` are
   taken; `route`, `board`, `curve` as bare identifiers are free but two-word forms are used anyway
   for consistency). Long/implementation-detail names (`materialize_fold_batches_`,
   `assign_objective_ranks_`) stay `snake_case_with_trailing_underscore` and private, matching the
   file's own convention for internals.
5. **Perf is a first-class requirement for every "heavy" engine touched here**, per explicit
   instruction — not an afterthought. Each design item in §5 states its complexity today and its
   complexity after the change; a rewrite that doesn't improve asymptotic or thread-utilization
   behavior gets called out as ergonomics-only rather than mislabeled as a perf win.

## 4. Target user surface

### 4.1 Column routing — `lymph::route(...)` (NodeJ: none needed, pure Lymph)

Problem: `feature_route` is an integer `RangeView::Int` everywhere; users hand-build index vectors
by iterating `x.features()` with a name predicate.

Key insight: `DataFrameBatchOptions::feature_names` (already in `nodej/data.h`) *is* the ordered
name list that determines `FeatureBatch` column order — `feature_batch_from_dataframe` builds
columns in exactly that order. So resolving "give me the indices of these names" is a pure,
already-well-defined lookup against a list that already exists; no new batch-construction plumbing
needed.

```cpp
namespace pluss::ai::lymph {

// Indices of `names` within `frame`'s Series-column order (the same order
// feature_batch_from_dataframe uses). Throws if a name isn't found or isn't
// a Series-kind column.
[[nodiscard]] std::vector<std::int64_t> route(const pluss::table::DataFrame& frame,
                                              pluss::msharp::RangeView::Of<std::string_view> names);

// Indices of every Series-kind column whose dtype satisfies `predicate`.
// Typical use: route_where(x, [](std::string_view dtype) { return dtype == "int64"; })
// for the "these are my factor-code columns" case loan_mod_lymp.cpp hand-rolled.
[[nodiscard]] std::vector<std::int64_t> route_where(
    const pluss::table::DataFrame& frame,
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);

// Convenience: everything NOT in `names` (the sklearn ColumnTransformer "remainder").
[[nodiscard]] std::vector<std::int64_t> route_remaining(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<std::string_view> names);

} // namespace pluss::ai::lymph
```

`loan_mod_lymp.cpp`'s 17-line manual loop becomes:

```cpp
const auto categorical_routes = lymph::route(x, {"gender_code", "account_frequency_code",
                                                  "district_name_code", "region_code",
                                                  "loan_year_code", "loan_month_code"});
const auto numeric_routes = lymph::route_remaining(x, {"gender_code", "account_frequency_code",
                                                        "district_name_code", "region_code",
                                                        "loan_year_code", "loan_month_code"});
```

Perf: O(ncols) string comparisons against a list typically in the tens, computed once outside any
CV/tune loop — not a perf-sensitive path, no vectorization needed.

### 4.2 Per-candidate `TrainOptions` in `selection::tune` (NodeJ: none needed, pure Lymph)

Problem: `tune(candidates, x, y, cv, train_options, ...)` applies one `TrainOptions` to every
candidate ([selection.h:176-184](../include/pluss/ai/lymph/selection.h)).

```cpp
struct Candidate {
  std::string name{};
  Workflow workflow{};
  std::optional<TrainOptions> train_options{};  // NEW: nullopt = use tune()'s shared default

  Candidate(std::string in_name, Workflow in_workflow, std::optional<TrainOptions> in_options = {})
      : name(std::move(in_name)), workflow(std::move(in_workflow)), train_options(std::move(in_options)) {}
  // ... existing move-only ctors unchanged
};
```

`tune()`'s per-candidate loop (selection.h:218) changes one line: pass
`candidates[i].train_options.value_or(train_options)` into `cross_validate_batches_` and into the
final/`fit_top` retrain calls. Fully backward compatible — every existing call site that doesn't
set `.train_options` behaves identically.

`loan_mod_lymp.cpp`'s abandoned-`tune()` case becomes expressible again:

```cpp
std::vector<lymph::selection::Candidate> candidates;
candidates.emplace_back("logistic_baseline", make_logistic_workflow(),
                         lymph::TrainOptions{.calibration = lymph::ScoreCalibration::Isotonic, ...});
candidates.emplace_back("decision_tree", make_tree_workflow());  // no override, uses shared default
// ...
auto result = lymph::selection::tune(std::move(candidates), x, y, cv_options);
```
"Deliberately not migrated (documented in the blueprint, not silently skipped): radius-based neighbor search (different query shape) and balance.cpp's data-cleaning neighbor search (same shape as BlendNeighbors, a natural future extension)."
### 4.3 Threshold curve — `lymph::threshold_curve` / `lymph::best_threshold` (NodeJ: real algorithm)

Problem: `loan_mod_lymp.cpp` re-runs `classification_summary` (a full infer + confusion computation)
at each of 17 threshold steps against the same fixed score vector. This is the textbook case for a
single sorted-cumulative-sum sweep (`sklearn.metrics.precision_recall_curve`'s algorithm).

**NodeJ** (`PlussAI/NodeJ/metrics/` — new file `threshold_curve.cpp`, alongside existing
`family_classifier.cpp`/metrics code):

```cpp
namespace pluss::ai::nodej {

struct ThresholdCurve {
  Series thresholds{};  // every distinct score value, descending
  Series precision{};
  Series recall{};
  Series f1{};
  Series support_positive{};  // running TP+FN count, for context
};

// One argsort + one pair of cumulative sums over the sorted (score, label)
// pairs computes precision/recall/F1 at *every* achievable threshold in a
// single pass — O(n log n) total, vs the O(n * thresholds_tried) cost of
// re-scoring the model at each candidate cutoff.
[[nodiscard]] StatusOr<ThresholdCurve> threshold_curve(const Series& scores, const Series& labels);

[[nodiscard]] StatusOr<double> best_threshold(const Series& scores,
                                              const Series& labels,
                                              MetricKind objective = MetricKind::F1);

} // namespace pluss::ai::nodej
```

**Lymph** (`metrics.h`, next to `classification_summary`):

```cpp
[[nodiscard]] inline ThresholdCurve threshold_curve(const WorkflowModel& model, const LabeledBatch& batch,
                                                     const InferOptions& infer_options = {}) {
  const Series scores = model.infer_scores(batch.features, infer_options);
  return resolve(::pluss::ai::nodej::threshold_curve(scores, batch.y), "threshold_curve");
}
[[nodiscard]] inline ThresholdCurve threshold_curve(const WorkflowModel& model, const pluss::table::DataFrame& x,
                                                     const Series& y, const InferOptions& infer_options = {});
// ... + Thread-y overload, matching the existing classification_summary overload set exactly

[[nodiscard]] inline double best_threshold(const WorkflowModel& model, const LabeledBatch& batch,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {});
// ... + DataFrame/Thread overloads
```

`loan_mod_lymp.cpp`'s 20-line manual loop (lines 665-684) becomes:

```cpp
const double best_threshold = lymph::best_threshold(trained_model, x_train, y_train, lymph::MetricKind::F1);
```

Perf: **O(n log n) replacing O(n × thresholds_tried)** — for the loan dataset's 17-step sweep this
is a ~17x reduction in score-classification work, and the gap widens with finer threshold grids
(sklearn users routinely want the full curve, not 17 points).

### 4.4 Results → `DataFrame` — `.board()` (NodeJ: none needed, pure Lymph)

Problem: `CrossValidationReport`, `TuneReport`, and any hand-rolled `std::vector<Result>` all get
manually unpacked into parallel `std::vector`s and rewrapped as a `DataFrame` — three times in
`loan_mod_lymp.cpp` alone.

```cpp
// metrics.h — one fold-mean row per metric.
[[nodiscard]] pluss::table::DataFrame CrossValidationReport::board() const;

// selection.h — one row per trial: name, objective, rank, + every requested metric mean.
[[nodiscard]] pluss::table::DataFrame TuneReport::board() const;
```

`loan_mod_lymp.cpp`'s three manual assemblies collapse to `result.report.board().save_csv(...)`.

Perf: these are ~dozens-of-rows tables built once at the end of a run — no perf target here, this
is purely an ergonomics fix (stated explicitly per §3.5's honesty requirement: not everything needs
to be a perf story).

### 4.5 `plan_balance`/`class_mix` DataFrame overloads (NodeJ: none needed, pure Lymph)

Close the inconsistency directly: add `(DataFrame x, Series y, ...)` overloads to `plan_balance`
and `class_mix` matching the existing `cross_validate`/`classification_summary`/`holdout_split`
pattern exactly (`metrics.h`, next to the existing Series-only overloads):

```cpp
[[nodiscard]] inline BalancePlan plan_balance(const pluss::table::DataFrame& x, const Series& y,
                                              const BalanceTarget& target, RandomState random = {});
[[nodiscard]] inline ClassMix class_mix(const pluss::table::DataFrame& x, const Series& y);
```

This directly removes the discoverability trap that caused this session's bug: a user reaching for
`plan_balance(x, y, target)` gets full `LabeledBatch` behavior (including `BlendNeighbors`) for
free, with no way to accidentally hit the labels-only restriction unless they deliberately pass
`Series` alone.

### 4.6 Expose the clone path — fix the already-diagnosed `tune()` redundant retrain

`selection.h`'s own comment (lines 260-271) already identifies that `fit_options.fit_top > 0`
retrains the best candidate a second time because `WorkflowModel::clone_for_out_of_place_()`
(a real state dump+reload, not a full re-fit) isn't public. Expose it:

```cpp
// workflow_model.h — public, short name; "clone" is unambiguous and unused elsewhere in lymph.
[[nodiscard]] WorkflowModel clone() const { return clone_for_out_of_place_(); }
```

Then `selection::tune`'s `fit_top` path reuses `result.fitted[0].clone()` for `best_workflow`
instead of calling `.learn(batch, train_options)` a second time. This is a genuine perf fix (skips
one full retrain of the winning candidate on every `tune()` call with `fit_top > 0`), not just
ergonomics, and it's the cheapest item in this entire blueprint — the fix the codebase already told
us to make.

## 5. Performance design (heavy engines)

### 5.1 `selection::tune`: candidate×fold scheduling — IMPLEMENTED

**Was**: `tune()`'s outer loop over candidates was strictly sequential; each call into
`cross_validate_batches_` parallelized only *within* that one candidate's folds, capped at
`worker_n = min(hardware_concurrency, fold_n)`. On an 8+ core machine tuning 5 candidates × 3 folds,
that used at most 3 threads at a time, 5 times in a row — the other 5+ cores sat idle the entire run.

**Built**: the (candidate, fold) Cartesian product is now flattened into one flat work list
(`GridCell_{candidate_index, fold_index}`) and dispatched in a single `pluss::aux::transmit(...)`
call, then regrouped back into one `CrossValidationReport` per candidate
([selection.h](../include/pluss/ai/lymph/selection.h), `tune()`'s body). Reused
`PlussTils/include/pluss/tils/aux/transmit.h` rather than hand-rolling a second thread-pool
dispatcher next to `cross_validate_batches_`'s existing one — `PlussAI/CMakeLists.txt:7` already
puts `PlussTils/include` on every PlussAI target's include path, and `transmit`/`transmit_options`
are header-only, so this needed no new link dependency.

**Correction from the original sketch below**: this ended up living in **Lymph** (`selection.h`),
not NodeJ. The original plan called it a "NodeJ-level scheduler," but the thing being parallelized —
`WorkflowModel::learn(...)`+`assess_series(...)` — is a Lymph-only type; NodeJ works with raw
`ModelSpec`, not `WorkflowModel`. `cross_validate_batches_` (the code this generalizes) already
lived in `workflow_model.h` for the same reason. The mean/stdev tail both that function and the new
flattened scheduler need was extracted into a shared `cross_validation_report_from_folds_()` helper
in `workflow_model.h`, used by both, so the two parallel-dispatch call sites don't each re-derive
fold-aggregation math independently.

**Verified**: `nodej_cv_tune_test` (exercises `fit_top=5`, checks the cloned `best_workflow` actually
infers correctly) passes unchanged, and a full end-to-end run of `nodej_workflow_tuning_demo` (4 real
candidates, real cross-validation, save/restore round-trip) produces identical results with
`max_abs_diff: 0` between the original and restored model's predictions.

Expected win: wall-clock time for a tuning run drops from `sum(candidate_i's fold time)` to roughly
`max(candidate_i's fold time)` on a machine with enough cores for the full grid — the shape of
speedup sklearn's `n_jobs=-1` gives via joblib, achieved here with `std::async` instead of a process
pool. (Not yet benchmarked with a wall-clock number — see §7 open questions.)

### 5.2 Threshold curve: O(n log n) vectorized sweep (NodeJ-level primitive) — IMPLEMENTED

Covered in §4.3. The complexity change is the headline: **O(n × thresholds_tried) → O(n log n)**,
and unlike the naive loop it also becomes exact (every achievable threshold, not an arbitrary
step grid) as a byproduct of doing it right, not an extra feature.

Implementation shape (matches `pairwise_distance2`'s existing style of staying inside `Series` ops,
no host loop over samples):
1. `argsort` scores descending — one `Series::argsort` call.
2. Gather labels into that order via `index_select`.
3. `cumsum` labels (running true-positive count) and running total (running predicted-positive
   count) — two `Series::cumsum` calls, fully vectorized.
4. `precision = tp / predicted_positive`, `recall = tp / total_positive` — elementwise `Series` ops
   across the whole curve at once, not per-threshold.

No host-side loop over either samples or thresholds anywhere in the implementation.

### 5.3 Column routing: no perf work needed, but a caution worth stating

`route`/`route_where` are O(ncols) host string comparisons — irrelevant next to model training
cost. The only perf rule that matters here is *don't recompute it inside a CV/tune loop* — routes
are fixed once a `WorkflowModel`'s prep stages are configured (before `.learn()` is ever called), so
`route(...)` belongs at model-construction time, same place `loan_mod_lymp.cpp` already computed
its manual version once, outside the CV grid. No API change needed to enforce this — it falls out
naturally from where `feature_route` parameters are consumed (workflow construction, not per-fold).

### 5.4 `plan_balance` `BlendNeighbors` / KNN classifier / KNN regressor neighbor search — IMPLEMENTED

**Was**: `pairwise_distance2` (`NodeJ/balance/balance.cpp:173-178`) computes a full O(n²) dense
squared-distance matrix per class via `matmul` for `BlendNeighbors` — vectorized (BLAS-backed), but
algorithmically quadratic. `NodeJ/classifiers/family_classifier.cpp` and
`NodeJ/regressors/family_regressor.cpp` each define their own copy of the same
`pairwise_distance2` pattern too.

**Correction found during implementation**: those `pairwise_distance2` copies in the classifier and
regressor are **not** the actual CPU hot path. Both files already have a *separate*,
already-existing CPU/GPU split: `knn_probability_cpu`/`knn_predict_cpu` are brute-force
triple-nested **host loops** (flat `std::vector<double>`, max-heap of size k) used whenever both
query and train data are on `pluss::pCPU`; `pairwise_distance2`+`argsort` (the Series/matmul path)
is used only as the **GPU fallback**. `balance.cpp` had no such split — it always used the
Series/matmul path regardless of platform. This matters because a KD-tree is fundamentally a
host-memory recursive structure — it cannot run on GPU — so the real migration target is the CPU
host-loop paths that already existed for this reason, not the GPU-safe Series paths this blueprint
originally assumed were the "duplicated" ones. The Series/matmul paths are correct GPU
implementations, not dead code, and **were not touched**.

**Built** (`NodeJ/neighbor/spatial_index.{h,cpp}`, new module): `SpatialIndex` — a classic
median-split KD-tree (`nth_element` build, O(n log n)) with a bounded max-heap k-NN query
(hyperplane-distance pruning, O(log n) average per query for low/moderate dimensionality, always
exact regardless of dimensionality — see the header's own doc comment for why a separate
high-dimensionality brute-force fallback was deliberately not added, another scope correction from
this section's original sketch). Two entry points: `query_k(query_rows, k)` for cross-set search
(model inference against training data) and `query_self_k(k)` for self-vs-self search (balance's
neighbor blending) — the latter excludes each point's own index directly rather than assuming
column 0 of a sorted distance row is self, which the original `argsort`-based code implicitly
assumed and which isn't actually guaranteed when duplicate points tie at distance 0. A genuine
correctness improvement on top of the perf win, not just faster-same-behavior.

**Migrated** (CPU-only, gated behind the exact same `platform() == pluss::pCPU` checks the
classifier/regressor already used; `balance.cpp` gained that split as part of this migration):
- `plan_balance`'s `BlendNeighbors` self-query (`balance.cpp`)
- `family_classifier.cpp`'s `knn_probability_cpu`
- `family_regressor.cpp`'s `knn_predict_cpu`

**Explicitly not migrated this pass** (documented, not silently dropped):
- `radius_probability_cpu`/`radius_probability_series` and their regressor equivalents — a genuinely
  different query shape (radius, not top-k; unbounded result size). `SpatialIndex` could support it
  with a similar tree-pruning skeleton, but that's new scope, not this migration.
- `balance.cpp`'s data-cleaning neighbor search (`cleaned_keep_indices`/`self_masked_distance2`,
  `BoundaryPairs`/`NeighborAgreement` cleaning) — same self-query shape as `BlendNeighbors` and a
  clean fit for `query_self_k`, but deferred to keep this pass's blast radius to the three
  originally-planned call sites.
- The *second* duplicated ROC-AUC copy already noted in §5.2 (the inline branch in
  `classification_metrics`) — still untouched, for the same reason noted there.

**Verified**: new `nodej_spatial_index_test.cpp` checks `SpatialIndex` against a brute-force
reference across 6 randomized (n, dim, k) combinations (including a 30-dimensional case and a
`k > n` clamping case), a hand-picked 2D fixture, an `n=2` self-query edge case, and — importantly
— a duplicate-points case that caught a real bug in the *test's own* comparison logic (naive
index-order tie-breaking doesn't hold for two independently-correct k-NN implementations; fixed by
comparing against the full sorted candidate list with a tie-tolerant optimality check instead of a
truncated, arbitrarily-ordered one). All three migrations verified against the full 20-target
NodeJ/Lymph suite (0 failures) plus a real end-to-end re-run of `nodej_workflow_tuning_demo`'s
`knn_distance` candidate, which produced `objective=77.4881` — bit-for-bit identical to the
pre-migration run.

### 5.5 `.board()` and route helpers: explicitly not perf items

Stated plainly per the honesty requirement in §3.5: `.board()` builds a table with row count equal
to candidate/fold count (tens, not millions) — there is no meaningful perf design to do here beyond
"don't do anything O(n²)," which a straightforward implementation won't. Listed for completeness,
not because it needs optimization.

### 5.6 Raw-DataFrame categorical ingestion: `DataNotePolicy::Factorize` — IMPLEMENTED

Surfaced answering a question about DataFrame ergonomics, not from the original sklearn-comparison
pass: `DataFrameBatchOptions.note_policy` (`nodej/data.h`) offered exactly three choices for a raw
text/categorical (`Note`-typed) column — `Reject` (the default; throws), `Drop`, or `Hash`
(irreversible feature hashing, sklearn's `FeatureHasher` equivalent). None of the three gave a
semantic, interpretable encoding. To get real one-hot/frequency/ordinal semantics, a user had to
call `frame.factor("col")` themselves and route the resulting integer-code column into
`one_hot_encoder`/etc. — the exact two-step `loan_mod_lymp.cpp` pattern (§2.1).

**Correction found during implementation (the open question below turned out to have a "neither"
answer)**: the original sketch proposed reusing `DataFrame::factor()` (a stateless, per-call
conversion that reassigns codes 0..k-1 from whatever distinct values that one call happens to see)
and plugging unknown-category handling into the encoder stages' existing `UnknownCategoryPolicy`.
Neither survived contact with the actual requirement. `DataFrame::factor()` cannot give a persistent
model train/inference-consistent codes — the same string could get a different code between the
`learn()` call and a later `infer()` call on different data, silently corrupting predictions with no
error. The fix has to be *stateful*: fit each `Note` column's category→code map once, at `learn()`
time, and reuse (never refit) that exact map on every later `infer()`/`map()`/`assess()`/`save()`
call for that model's lifetime, including after a save/load round trip. This pushed the change beyond
`dataframe_bridge.cpp` into `WorkflowModel` itself and the `FlowBundle` save/load format — larger than
the original sketch, confirmed with the user via `AskUserQuestion` before implementing (full
correct/stateful design chosen over a smaller stateless-and-known-broken version).

**Built**:
- `nodej/data.h`: `DataNotePolicy::Factorize` (4th enumerator); `CategoryMap{categories}` /
  `CategoryMaps = unordered_map<string, CategoryMap>` (column name → fitted map); both
  `feature_batch_from_dataframe` and `labeled_batch_from_dataframe` gained
  `const CategoryMaps* existing_categories = nullptr, CategoryMaps* fitted_categories_out = nullptr`
  trailing params — absent `existing_categories` fits fresh (standalone/original stateless use, still
  the default), present reuses those exact codes; non-null `fitted_categories_out` captures whatever
  got freshly fit. `DataFrameBatchOptions.factorize_unknown_categorical` (reusing the *existing*
  `UnknownCategoryPolicy` enum from `nodej/options.h`, not a new one — the "plug into the existing
  mechanism" half of the original open question *was* right, just at the raw-encode layer rather than
  the encoder-stage layer) governs a reused map seeing a string absent from it: `Error` throws,
  `MissingBucket` (default) maps to a reserved code (`categories.size()`, one past the known set),
  `Ignore` maps to NaN. A fresh fit can never hit "unknown" by construction — everything it sees
  becomes a category.
- `NodeJ/data/dataframe_bridge.cpp`: the `Factorize` branch builds category codes directly (a
  `std::set<std::string_view>` of distinct values → sorted category list on fresh fit, an
  `unordered_map<string_view, int64_t>` keyed into the owned `CategoryMap.categories` strings for the
  O(n) encode pass) rather than going through `DataFrame::factor()` — that function has no fit/reuse
  split, so building the small amount of logic directly (a set collect + one hash-map encode loop) was
  less code than adapting a stateless helper into a stateful one.
- `nodej/io.h` / `NodeJ/io/flow_bundle.cpp`: `FlowBundle` gained a `CategoryMaps category_maps` field;
  `io::save`/`FlowBundle::load` serialize/deserialize it (new `write_category_maps`/
  `read_category_maps` helpers, same `.pod()`/`.string()` primitive pattern as every other collection
  in that file). `k_flow_bundle_schema` bumped `4 → 5` — this hard-invalidates previously-saved
  `.bundle` files (the format's `load()` does an exact schema-version equality check, no
  backward-compat reading; already bumped 3 times before in this project's history, consistent with
  pre-1.0 status).
- `lymph/workflow_model.h`: `WorkflowModel` gained a private `category_maps_` member, fitted (cleared
  + repopulated) inside both `learn_()` overloads and consumed (`existing_categories`, never refit) by
  every DataFrame-taking `map`/`learn_map`/`infer`/`infer_scores`/`assess_series` overload and both
  `save()` overloads; `restore()`/`load()` recover it from the bundle; `clone_for_out_of_place_()`
  copies it. A real bug was found and fixed along the way: `infer(x, options)` and
  `infer_scores(x, options)` (the 2-arg DataFrame convenience overloads) were calling
  `workflow_.infer/infer_scores(x, ...)` directly, bypassing their own local 3-arg sibling that now
  does the category-map-aware conversion — fixing only the 3-arg forms would have left these two
  silently unfixed. `lymph::Workflow`'s own DataFrame-taking methods needed no changes: they delegate
  straight to `WorkflowModel`'s (now-fixed) methods.

**Scope boundary explicitly held**: `nodej::Workflow` (a different type from `lymph::Workflow`) has
its own internal `feature_batch_from_dataframe`/`labeled_batch_from_dataframe` calls with no
category-map awareness. Touching that would have added a fourth layer to an already-substantial
change; instead `lymph::WorkflowModel`'s DataFrame-taking methods bypass `workflow_.learn/infer/map/
assess(frame, data_options, options)` entirely and do the DataFrame→Batch conversion themselves (with
`category_maps_` threaded through), then call the lower-level `FeatureBatch`/`LabeledBatch` overloads
— which needed no changes at all. `nodej::Workflow`'s class interface was not touched.

**Verified**: `nodej_dataframe_bridge_test.cpp` gained direct `feature_batch_from_dataframe` coverage
— fresh fit assigns sorted-lexicographic codes (checked against exact expected values, not just
"succeeds"), reuse encodes with the frozen map rather than refitting, and all three
`UnknownCategoryPolicy` behaviors are exercised against a category absent from the reused map.
`lymph_metrics_test.cpp` gained an end-to-end `WorkflowModel` check: `learn_()` on a DataFrame with a
raw-text column, `infer()` on an unseen category with `UnknownCategoryPolicy::Error` throwing (the
actual proof that infer reuses the frozen train-time map instead of silently refitting — a stateless
refit would just accept any new string as a legitimate category, no error), `infer()` on known
categories producing identical predictions before and after a real `save()`/`WorkflowModel::load()`
round trip through a temp file, and the reloaded model still rejecting the same unseen category
afterward. Full 20-target NodeJ/Lymph suite re-run, 0 failures.

## 6. Phased implementation plan

**Phase 1 — cheap, additive, zero breaking changes (do first): DONE**
1. §4.6 — expose `WorkflowModel::clone()`/`Workflow::clone()`, fix `tune()`'s redundant retrain. ✅
2. §4.5 — `plan_balance`/`class_mix` DataFrame overloads (`class_mix` ended up taking a column
   name, not a `Series` — see §4.5, corrected during implementation since the original sketch's
   `Series` parameter would have gone unused). ✅
3. §4.1 — `lymph::route`/`route_where`/`route_remaining` in `common.h`. ✅
4. §4.2 — per-candidate `TrainOptions` on `Candidate`. ✅

All four verified via the full 19-target NodeJ/Lymph test suite plus three downstream demos
(`nodej_workflow_tuning_demo`, `lymph_end_to_end_hour_workflow_showcase`,
`lymph_housing_ch2_end_to_end_notebook`) compiling and running unchanged.

**Phase 2 — the real algorithm work: DONE**
5. §4.3 + §5.2 — `nodej::threshold_curve`/`best_threshold` (`NodeJ/metrics/threshold_curve.cpp`),
   sharing a new `detail::binary_score_curve` primitive with a refactored `ovr_auc_binary01`
   (previously an independent duplicate of the same sort+cumsum+boundary shape). Verified against a
   hand-computed 5-sample precision/recall/F1 fixture plus a tie-collapsing fixture in
   `nodej_metrics_test.cpp` — caught and fixed a real bug in the process (an `f1_den.clamp_min(1.0)`
   zero-division guard copied from the scalar-metrics code, wrong for a curve where
   precision+recall legitimately ranges over [0,2]; fixed to `clamp_min(1e-12)`). Then the
   `lymph::threshold_curve`/`best_threshold` façade overload set on both `WorkflowModel` and
   `Workflow`. ✅
6. §5.1 — the candidate×fold flat scheduler, built on `pluss::aux::transmit` rather than a new
   hand-rolled dispatcher. **Lives in Lymph (`selection.h`), not NodeJ** — see §5.1's "Correction
   from the original sketch" for why. ✅

**Phase 3 — results ergonomics: DONE**
7. §4.4 — `.board()` on `CrossValidationReport` (`metrics.h`) and `TuneReport` (`selection.h`). One
   row per metric for the former, one row per trial (name, objective, rank, every requested metric's
   fold mean) for the latter — collapsing what was three manual `std::vector` + `push_back` +
   `DataFrame({...})` assemblies in `loan_mod_lymp.cpp` into `report.board()`. Small
   `metric_kind_name_()` helper added (`MetricKind` had no existing string-name mapping to reuse) for
   column naming. Verified against real cross-validation output in `nodej_cv_tune_test.cpp` — the
   board's "objective"/metric columns are checked to match each trial's actual objective value, not
   just checked to exist. ✅

**Phase 4 — shared neighbor-search primitive (§5.4): DONE**
8. Built `nodej::SpatialIndex` (`NodeJ/neighbor/spatial_index.{h,cpp}`, new module) and migrated
   `plan_balance`'s `BlendNeighbors` self-query (`balance.cpp`) onto it first. ✅
9. Migrated the classifier/regressor CPU-path k-NN (`knn_probability_cpu`/`knn_predict_cpu`) onto
   the same index — **not** the `pairwise_distance2` Series/matmul call sites this item originally
   named, which turned out to be the GPU fallback, not duplicated CPU brute force; see §5.4's
   "Correction found during implementation." ✅
10. ~~Delete the now-dead duplicated `pairwise_distance2` copies~~ — **withdrawn**. They're the
    correct GPU implementation, still in active use, not dead. Nothing to delete.

Verified via a new `nodej_spatial_index_test.cpp` (brute-force cross-check across randomized
dimensions/k/n plus duplicate-point and edge cases), the full 20-target NodeJ/Lymph suite, and a
real end-to-end demo re-run producing bit-identical output to the pre-migration baseline. Explicitly
NOT migrated this pass (§5.4 has the full list): radius-based neighbor search (different query
shape) and `balance.cpp`'s data-cleaning neighbor search (same shape as `BlendNeighbors`, a natural
future extension, deliberately deferred to keep this pass's blast radius to the three
originally-planned call sites).

**Phase 5 — raw-DataFrame categorical ingestion (§5.6): DONE**
11. Resolved the open unknown-category question — and found it needed a bigger answer than "which
    existing mechanism does this plug into": stateless per-call factorization (the original sketch's
    `DataFrame::factor()` reuse) cannot give a persistent model consistent train/inference codes at
    all, regardless of which unknown-category enum backs it. Confirmed the larger, stateful design
    (`WorkflowModel`-level `CategoryMaps`, threaded through the save/load bundle format) with the user
    via `AskUserQuestion` before implementing, rather than building the smaller known-broken version.
    ✅
12. Added `DataNotePolicy::Factorize` to `nodej/data.h` and implemented it directly in
    `NodeJ/data/dataframe_bridge.cpp` (not via `DataFrame::factor()` — that helper has no fit/reuse
    split to build on; see §5.6). ✅
13. Wired `CategoryMaps` through `WorkflowModel::learn_/map/learn_map/infer/infer_scores/assess_series/
    save/restore/load/clone_for_out_of_place_` and through `FlowBundle`'s serialize/deserialize
    (`io.h`/`flow_bundle.cpp`, schema bumped 4→5). ✅
14. Verified with real correctness tests, not just "does it run": `nodej_dataframe_bridge_test.cpp`
    checks fresh-fit codes, reuse-not-refit codes, and all three `UnknownCategoryPolicy` behaviors
    against exact expected values; `lymph_metrics_test.cpp` checks a full `WorkflowModel`
    learn→infer→save→load→infer cycle, including that an unseen category is rejected identically
    before and after the round trip (the actual proof the frozen map — not a silent per-call refit —
    is what's in effect). Full 20-target NodeJ/Lymph suite re-run, 0 failures. ✅

Scheduled after Phase 4 for the same reason Phase 4 was scheduled after Phases 1-3: it was a
NodeJ-level change with an open design question, not a quick additive Lymph-only fix — and, as it
turned out, the open question's real answer widened the change's blast radius further than
originally scoped (§5.6).

**Verification discipline for every phase** (matching this session's established practice): build +
run the full NodeB/NodeJ/Lymph test suite after each phase, plus real end-to-end demo runs, not just
unit-test exit codes. Phases 1-2 were verified this way: the full 19-target NodeJ/Lymph suite, a
hand-computed correctness fixture for `threshold_curve` (§5.2 — this is what caught the F1
zero-division bug), and a real run of `nodej_workflow_tuning_demo` (4 candidates, real
cross-validation, save/restore round-trip `max_abs_diff: 0`).

Note on `loan_mod_lymp.cpp`: it was deliberately **not** used to drive or validate these two phases.
Per explicit direction, this blueprint's API surface must stay generic — shaped by what sklearn
users and the codebase's own established patterns expect, not by any one downstream project's
specific column names or workflow shape. `loan_mod_lymp.cpp` remains useful as *one* real-world
example of the pain points motivating this work (§2.1), but is not the target its fixes are
optimized for, and re-running it is not part of this plan's verification loop.

**Phase 6 — close the Factorize gap across the rest of the (x, y) surface, then refactor
`loan_mod_lymp.cpp` onto all six phases: DONE**

Unlike Phases 1-5, this one *was* driven by `loan_mod_lymp.cpp` directly — by design this time: the
user asked for that file to be refactored onto everything above, with an explicit rule ("only when
the API does not truly offer a way of doing something should it then be done at the user level") that
turns a real refactor into a gap-finding exercise. It surfaced two real, disclosed gaps beyond what
Phase 5 covered:

15. **The (x, y)-Series calling convention can't carry a `note_policy` at all.** `cross_validate`,
    `classification_summary`, `confusion_table`, `threshold_curve`, `best_threshold`, `plan_balance`,
    and `selection::tune` all had `(x, y)` overloads that hardcode `DataNotePolicy::Reject` internally
    (`cross_validate_default_data_options_`/`default_data_options_`) — there's no `DataFrameBatchOptions`
    parameter for a caller to override. The `(frame, DataFrameBatchOptions, ...)` overloads that *do*
    thread the caller's options through already existed for `cross_validate`, but not for the other
    five. Added `(model, frame, data_options, ...)` siblings for `confusion_table`, `classification_summary`,
    `threshold_curve`, and `best_threshold` (`workflow_model.h`) and for `plan_balance`
    (`metrics.h`), all built the same way: derive `y` from `frame.features(*data_options.label_feature)`
    and route through `model.infer`/`infer_scores` (which already reuse the model's frozen
    `category_maps_` correctly per Phase 5) instead of rebuilding a Reject-only batch. `selection::tune`
    needed more surgery: its body was split into a private `tune_batch_(candidates, LabeledBatch, ...)`
    doing all the actual work, with the existing `(x, y)`/`(x, Thread)` overloads and a new
    `(candidates, frame, data_options, ...)` overload each just building their batch differently and
    delegating to it — the flattened candidate×fold scheduler (§5.1) didn't need to change at all. ✅
16. **`route`/`route_remaining` can't index a Note column at all.** Both walk
    `frame.feataxis().series().features()` — Series-kind columns only — so a raw `gender`/`account_frequency`-
    style Note column is invisible to them, and worse, every numeric column *after* one gets the wrong
    index once `DataNotePolicy::Factorize` splices that Note column into the `FeatureBatch` at its real
    position. Added a DataFrame-free overload pair, `route(const std::vector<std::string>& ordered_names,
    names)` / `route_remaining(ordered_names, names)`, that indexes against whatever ordered name list the
    caller already has (their own `DataFrameBatchOptions::feature_names`) instead of re-deriving one from
    the frame — correct regardless of column dtype, and it needed no DataFrame argument at all. ✅
17. **Found and fixed a real crash, not just a gap, while running the refactored file**:
    `EnsembleClassifier::infer_scores()` returns an unbounded raw logit-margin for
    `GradientBoost`/`AdaBoost` (confirmed in `family_classifier.cpp` — `EnsembleClassifier::assess()`
    already applies `.sigmoid()` to it before treating it as a probability), but `threshold_curve`/
    `best_threshold` fed that raw margin straight into `InferOptions::positive_threshold`, which every
    classifier family's `infer()` validates to `[0, 1]` — so the "best" threshold they picked for those
    two families could be, and was, outside that range, throwing on the very next `infer()` call.
    Phase 2's own test fixtures never caught this because they only exercised logistic regression
    (whose `infer_scores()` is already a real probability). Fixed with a small `as_probability_like_()`
    helper in `workflow_model.h`, applied at `threshold_curve`/`best_threshold`'s two root call sites:
    scores already inside `[0, 1]` pass through untouched; anything that escapes gets sigmoid-mapped
    first, mirroring the exact escape hatch `EnsembleClassifier::assess()` already uses. ✅
18. Refactored `loan_mod_lymp.cpp` end to end onto all six phases: `DataNotePolicy::Factorize` replaced
    the `.factor()`/`.assign()` pre-processing block; `route`/`route_remaining` replaced the manual
    categorical/numeric index loop; `selection::tune` (with per-candidate `TrainOptions`) plus
    `.board()` replaced the entire hand-rolled cross-validated-ranking loop and its manual
    struct→vectors→`DataFrame` assembly; `best_threshold` replaced the 17-step manual threshold grid
    search; `plan_balance(frame, data_options, ...)` replaced the manual `LabeledBatch`/matrix
    construction `BlendNeighbors` needed. What's left at the user level is genuinely bespoke: the
    holdout-evaluation grid's per-candidate trained-model/KS-curve/confusion-text retention (no API
    concept covers "run a custom multi-criterion ranking over trained models with side artifacts"),
    and the column-vectors-then-`DataFrame` construction pattern for small result tables (the API is
    column-oriented, there's no row-append builder). ✅

**A discovered analysis-quality finding, not a code defect, worth recording**: with the search now
covering the *full* achievable threshold range (matching sklearn's `precision_recall_curve` +
`argmax` convention) instead of the original file's hardcoded `[0.10, 0.90]` grid, `best_threshold`
picked `threshold≈0.9999` for the uncalibrated `adaboost` candidate under `blend_neighbors_equalize` —
technically valid (`as_probability_like_` sigmoid-mapped it into range correctly) but operationally
useless (recall collapses to ~0.21). This is real information the old restricted grid was silently
hiding by construction: `configured_train_options` only applies `ScoreCalibration::Isotonic` to
`logistic_baseline`, so AdaBoost's raw margin-derived "probability" is poorly calibrated, and an
honest full-range threshold search surfaces exactly that instead of papering over it. Not fixed here
— it's a modeling decision (apply calibration more broadly, or pick the champion by a metric less
sensitive to threshold pathology), not an API defect.

**Verified**: full 20-target NodeJ/Lymph suite, 0 failures, after both the five new `(frame,
data_options)` overloads and the `tune()`/`as_probability_like_` changes. Real end-to-end verification
via the actual refactored `loan_mod_lymp.cpp`: builds clean, runs to completion, produces sane
15-candidate CV rankings, holdout comparisons, and a saved champion bundle (`blend_neighbors_equalize
+ adaboost`).

## 8. Genuine hyperparameter tuning, and the tune → compare rename

Prompted by direct user feedback after Phase 7's downstream refactor exercise: `selection::tune`
(Phase 1-6's model-*comparison* utility — rank pre-built candidates via CV) was misnamed. A user
reading `lymph::selection::tune` reasonably expects hyperparameter *search* (sweep a model's own
settings) and gets model *selection* (rank whole pipelines the caller already built) instead — a
real surprise, not a preference. Fixing the name required first building the thing the name should
actually refer to, since simply renaming `tune`→`compare` and stopping would leave "genuine
hyperparameter tuning" as an unfilled promise.

### 8.1 sklearn parity research (Pipeline / ColumnTransformer / TransformedTargetRegressor /
GridSearchCV / RandomizedSearchCV) — evidence

Grounded against sklearn's actual docs and source (`_search.py`'s `ParameterGrid`/`ParameterSampler`/
`BaseSearchCV`), not memory:

| sklearn concept | Verified sklearn behavior | Lymph/NodeJ before this phase | Verdict |
|---|---|---|---|
| `Pipeline` | chain steps, `.fit/.predict/.score`, `named_steps`, `set_params(step__param=...)`, swap/skip/remove a step, `memory` caching | `WorkflowModel`/`Workflow` already are this in spirit (chain prep stages + model, one `.learn_`/`.infer`/`.assess`); `.map()` mirrors `.transform()`; `function_transformer`/`feature_transformer` (with a remainder policy) already give a raw-callable escape hatch | **Mostly present.** Missing: `named_steps`-style introspection/post-hoc `step__param` mutation, and a column-drop/null-filter prep stage (§5.6/§6 Phase 6 already flagged this) |
| `ColumnTransformer`/`make_column_transformer` | name/dtype/mask/callable column selection, `remainder=drop/passthrough/estimator`, standalone reusable object | `route`/`route_where`/`route_remaining` resolve names/dtypes to `feature_route` indices, fed per-stage into `.standard_scaler(...)`/etc.; `route_where`'s dtype selector is still Series-only (can't see a Note column) | **Partial** — pieces exist, fragmented, one real dtype-blindness gap |
| `TransformedTargetRegressor` | wrap a regressor + a y-transform, auto-inverse on `.predict` | nothing — grepped the whole `pluss::ai::{nodej,lymph}` tree | **Missing entirely** |
| `GridSearchCV` | full param-grid cross product, `cv_results_`, `best_estimator_`/`best_params_`, nested `step__param` naming | `nodej::sweep`/`nodej::tune`/`nodej::probe` (`NodeJ/tune/tune.cpp`) *already* implement `SearchKind::{Grid,Random,Bayesian,SuccessiveHalving}` with full (candidate/trial × fold) parallel dispatch — more search strategies than sklearn's own base install ships. What was missing was purely the front end: nothing turned a parameter *grid* into the `ModelSpec` list these consume. | **Engine already existed; front end missing** — the actual gap was narrower than it first looked |
| `RandomizedSearchCV` | sample `n_iter` param settings, without-replacement from a discrete grid | Same as above — `sample_candidates_without_replacement` (shuffle-and-take) already implements exactly `ParameterSampler`'s all-lists-case algorithm, just for hand-built `ModelSpec` lists, not an expanded grid | **Engine already existed; front end missing** |

The headline finding: this codebase's hyperparameter-*search* engine (NodeJ's `tune`/`probe`, with
Bayesian and successive-halving strategies sklearn doesn't even ship by default) was already more
capable than sklearn's base `GridSearchCV`/`RandomizedSearchCV` — it just had no Lymph-level way to
describe a search space, and its Lymph facade (`tune_specs`/`probe_specs`) was already correctly
named (both genuinely support `TuneOptions.search` beyond plain comparison), while the *other*
Lymph-level `tune` (Workflow-`Candidate` comparison, no search-strategy support at all) was the one
actually mismatched to its name.

### 8.2 What was built

15. **Renamed** the Workflow-`Candidate` comparison utility `selection::tune` → `selection::compare`
    (`TuneReport`→`CompareReport`, `TuneResult`→`CompareResult`, the private `tune_batch_`→
    `compare_batch_`) across `selection.h`, every in-repo test/demo caller, and the downstream
    `loan_mod_lymp.cpp`/`stripped_down_lymph.cpp`. Scoped deliberately narrow: `nodej::sweep`/
    `nodej::tune`/`nodej::probe` and Lymph's `tune_specs`/`probe_specs` keep their names unchanged —
    they already support real search strategies via `TuneOptions.search`, so "tune" was never wrong
    for them, only for the Workflow-`Candidate` form. ✅
16. **Built `GridAxis`/`GridPoint`/`grid_size`/`grid_point_at`/`grid_points`** (`selection.h`): a
    named-value-list axis type, a positional-and-name-keyed point accessor (`point.at("learning_rate")`
    in a factory closure, not a bare index against a parallel axis list), and a mixed-radix decoder
    (`grid_point_at`) computing any one point directly without materializing the others — the same
    divmod cascade `ParameterGrid.__getitem__` uses, chosen deliberately so a randomized search never
    has to build a space it's only going to sample a handful of points from. `grid_points` (full
    materialization) is the exhaustive case's own convenience on top of the same decoder. ✅
17. **Built `selection::tune`** (`selection.h`, reclaiming the name for its intended meaning): expands
    a `std::vector<GridAxis>` into `ModelSpec`s via a caller-supplied `(GridPoint) -> ModelSpec` factory
    and hands the result to `probe_specs` — **zero new parallel-dispatch code**, reusing the exact
    (trial × fold) engine `sweep`/`probe`/`compare` already have. A single public entry point takes a
    `TuneOptions` (the same struct `tune_specs`/`probe_specs` already dispatch on) and branches on
    `.search`: `None`/`Grid` evaluates every point (index range `0..grid_size-1`); `Random` draws
    `tune_options.trials` points via shuffle-and-take without replacement (mirroring
    `sample_candidates_without_replacement`'s own algorithm, applied to raw indices instead of
    pre-built specs so the full space is never materialized for a small sample), matching
    `ParameterSampler`'s exact `n_iter = min(n_iter, grid_size)` clamp when the trial count covers
    everything; any other `SearchKind` throws a clear "not supported here, use `probe_specs` directly"
    error rather than silently misbehaving. This started as two public functions (`tune` exhaustive,
    `tune_sample` randomized) and was consolidated into the one-function/`TuneOptions.search` shape on
    direct feedback that a second, separately-named entry point was one API surface too many once the
    engine already had an enum for exactly this choice — the without-replacement sampling logic that
    used to be the public `tune_sample` body is now reached only through the `SearchKind::Random`
    branch inside `tune` itself, with no separate name of its own. Has
    `(batch)`/`(x, y)`/`(x, Thread)`/`(frame, DataFrameBatchOptions)` overloads, matching every other
    multi-shape entry point in this file. ✅
18. **Naming discipline**: no identifier or comment in the new code says "sklearn" (Phase 6-7's own
    comments that had crept in — `route_remaining`'s and `threshold_curve`'s doc comments — were also
    cleaned up as part of this pass, per explicit instruction not to let the API read as a port).
    `GridAxis`/`GridPoint`/`tune`/`tune_sample`/`grid_size`/`grid_point_at`/`grid_points` are not
    sklearn's names (`ParameterGrid`/`ParameterSampler`/`GridSearchCV`/`RandomizedSearchCV`) even
    though the underlying algorithms are deliberately the same ones, for the same reason those
    algorithms are already correct: no reason to reinvent index-into-cross-product math sklearn's own
    maintainers already got right, every reason to give it pluss-native names. ✅

**Performance note, since this was explicitly flagged as heavy**: the actual expensive part of any
search (training + cross-validating each candidate) was never new work here — `probe`'s existing
parallel (trial × fold) dispatch (chunked `std::async` batches, same shape `sweep`/`compare` use)
handles that, unchanged. The only genuinely new computation is grid bookkeeping (index arithmetic,
`ModelSpec` construction), which is `O(grid\_size)` host-side work with no vectorization opportunity
worth chasing — building a `ModelSpec` is a few field assignments, not a numeric kernel — and the
`SearchKind::Random` branch's whole reason to exist is to avoid *ever* paying that `O(grid\_size)` cost
when the caller only wants `trials ≪ grid\_size` points, which `grid_point_at`'s no-materialization
decode makes possible in the first place.

**Deliberately not built this phase** (documented, not silently dropped, matching every other scope
cut in this blueprint):
- Continuous-distribution axes (sklearn's `scipy.stats` `.rvs()` sampling in `ParameterSampler`) —
  `GridAxis` only takes discrete value lists. A continuous axis would need a small sampler
  abstraction (uniform/log-uniform/choice) layered on top of `tune`'s existing `SearchKind::Random`
  branch; the discrete case covers the overwhelming majority of real hyperparameter grids on its own.
- `TransformedTargetRegressor`-equivalent and a pipeline-native column-drop/null-filter stage (§8.1's
  table) — both real, both still open, neither touched this phase; §9 below tracks them as the next
  two essentials.
- Exposing Bayesian/`SuccessiveHalving` search through the new `GridAxis` front end — `tune_specs`/
  `probe_specs` already reach them via `TuneOptions.search` for a hand-built candidate/spec list, and
  a power user can already get there today by calling `probe_specs` directly with a `grid_point_at`-based
  sampler (exactly what `tune_indices_` does internally) — just without a dedicated ergonomic wrapper
  yet.

**Verified**: full 20-target NodeJ/Lymph suite, 0 failures. New correctness coverage in
`nodej_cv_tune_test.cpp`: `grid_size`/`grid_points` checked for exact point count and full
axis-combination coverage on a tiny 2×3 space; `tune(..., TuneOptions{.search=SearchKind::Grid})`
checked to prefer the least-biased (smallest-regularization) point on noiseless linear data across an
exhaustive 4-point sweep; `tune(..., TuneOptions{.search=SearchKind::Random, .trials=2})` checked to
return exactly `trials` trials with distinct grid points. Both downstream demos
(`loan_mod_lymp.cpp`, `stripped_down_lymph.cpp`) updated for the rename and rebuilt clean against the
re-staged SDK.

## 9. Next essentials (not yet built)

Two real, disclosed gaps carried forward from §8.1's comparison table, in priority order:

1. **A pipeline-native column-drop/null-filter prep stage.** Closes both the `Pipeline`-completeness
   gap and the "no single raw-frame-in/prediction-out production object" gap flagged when comparing
   `loan_mod_lymp.cpp`'s pipeline to a deployable sklearn `Pipeline`: today, dropping ID/leakage
   columns and filtering incomplete rows happens on the raw `DataFrame` *before* any Lymph object sees
   it, so a saved bundle can't replay that step for a caller who only has the original raw schema.
2. **A `TransformedTargetRegressor`-equivalent.** A thin wrapper transforming `y` before `learn_` and
   inverse-transforming predictions on `infer`/`assess` — self-contained, no dependency on anything in
   §8.

Also still open from Phase 6: **no way to persist a model's chosen operating threshold** alongside
its saved bundle (`FlowBundle` has no such field) — a caller who only has the bundle and calls
`.infer()` with default `InferOptions` silently gets a different decision boundary than the one that
was actually tuned for it.

## 10. Open questions

- `route_where`'s dtype predicate: expose the raw dtype string (`"int64"`, `"float64"`, ...) as
  lymph currently does via `DataFrame::dtypes()`, or a typed `enum class ColumnDType` for
  discoverability? Leaning typed enum for a public API, but needs to match whatever `dtypes()`
  already reports without a lossy translation.
- `.board()`'s exact column set for `CompareReport` — every requested metric mean, or mean+stdev both
  (doubling column count)? Leaning mean-only by default with a `bool include_stdev = false` param,
  matching `CrossValidationOptions`-style opt-in defaults elsewhere in lymph.
- Should `evaluate_grid` (§5.1) also subsume plain `cross_validate`'s single-candidate fold
  parallelism (i.e., `cross_validate` becomes `evaluate_grid` with one candidate), or stay a
  tune-only addition with `cross_validate_batches_` left as-is? Leaning toward unifying them —
  two parallel-dispatch implementations of the same idea is exactly the kind of duplication this
  codebase avoids — but that's a larger refactor than Phase 2 needs and should be scoped
  separately once `evaluate_grid` exists and is proven.
