I’d do it in **9 phases**. That is enough to make Ice structurally strong, centralized, and extensible without turning it into a rewrite spiral.

The governing principle would be exactly what you said: **centralize shared behavior once, then reuse it everywhere**. In practice that means no more ad hoc train loops, ad hoc checkpoint formats, ad hoc batch movement, or model-specific optimizer plumbing unless a workload genuinely cannot fit the shared substrate.

Two additional cross-cutting requirements need to be treated as first-class parts of this redesign rather than afterthoughts:

1. **Regularization must be centralized by kind.** Architectural regularization (dropout, stochastic masking, train/eval-dependent behavior), objective regularization (weight decay, explicit penalty terms, label smoothing, auxiliary losses), and loop regularization (early stopping, validation monitoring, restore-best, patience/min-delta policy) should each live in one shared place with one shared policy surface, not be reintroduced model by model.
2. **No-grad must move down to NodeK.** The current `ice::NoGradGuard` is only a local workaround that toggles registered parameter leaves. That is not the same thing as a real libtorch-style execution-mode guard. A proper solution belongs in the lower-level autodiff/runtime layer so that `infer()`, `assess()`, checkpoint evaluation, post-training scoring, and any future model family all benefit from the same mechanism.

This redesign therefore treats regularization and no-grad as foundation work, not polish.

**Phase 1: Core batch abstraction**
- Introduce `ice::Batch` as the common transport for training, validation, test, and inference.
- Let it carry named tensors, targets, masks, weights, and lightweight metadata.
- Keep it generic enough for tabular, sequence, forecasting, multimodal, and future NodeK consumers.
- Migrate current single-`Series` assumptions behind adapters rather than breaking everything at once.

**Phase 1.5: NodeK grad-mode foundation**
- Introduce a real lower-level grad-mode surface in NodeK, modeled on the libtorch design of a **thread-local RAII guard that restores the previous state on destruction**.
- The minimum robust substrate is:
  - `GradMode::is_enabled()`
  - `GradMode::set_enabled(bool)`
  - `AutoGradMode(bool enabled)`
  - `NoGradGuard`
- Every graph-building path in NodeK should consult this one shared thread-local state before attaching autodiff metadata or edges. The rule is: if grad mode is off, differentiable ops still run numerically but do not extend the backward graph.
- `ice::Model::infer()` and `ice::Model::assess()` should stop owning a private workaround and instead delegate to this lower-level guard.
- This is deliberately modeled after libtorch's public and source-level shape:
  - `c10::GradMode`
  - `c10::AutoGradMode`
  - `c10::NoGradGuard`
  - and, later if NodeK's runtime grows the necessary machinery, an `InferenceMode`-style stricter/faster mode.
- Important boundary: **do not jump straight to a fake `InferenceMode` clone**. PyTorch's `InferenceMode` also manipulates dispatch/autodiff key state and version/view behavior, not just a boolean flag. Until NodeK has equivalent lower-level execution-state plumbing, the correct first milestone is a real `NoGradGuard`, not a misleading pseudo-inference mode.

**Phase 2: Central trainer engine**
- Introduce `ice::Coach` as the one true training orchestrator.
- Move shared loop mechanics out of `Model::learn()` into `Coach`.
- Centralize epoch loop, batch loop, backward, optimizer step, validation cadence, early stopping checks, chunked inference, and numerical guards here.
- Keep `model.learn(...)` as a convenience wrapper over `Coach`.

**Phase 3: Central training config surface**
- Introduce `ice::Recipe` as the common training configuration object.
- Move optimizer selection, loss selection, metrics, gradient clipping, accumulation, precision policy, seed, platform/device, validation cadence, and checkpoint policy into it.
- This becomes the single config object reused by `Outline`, `Model`, TFT-like models, NodeB deep stages, and future Ice models.

At this phase, `ice::Recipe` should also become the **single regularization policy surface**. That policy should be split by concern, not flattened into a miscellaneous bag of booleans:

- `ArchitectureRegularization`
  - `dropout`
  - optional per-subsystem overrides later (`attention_dropout`, `residual_dropout`, `ff_dropout`, etc.)
  - train/eval behavior and deterministic seeding policy
- `ObjectiveRegularization`
  - `weight_decay` (delegated to NodeK optimizers where already supported)
  - `decoupled_weight_decay` where optimizer family supports it
  - explicit additive penalties such as L1/L2/activity penalties
  - label smoothing / auxiliary losses where applicable
- `LoopRegularization`
  - early stopping enabled/disabled
  - monitor target (`val_loss`, named metric, training loss if intentionally requested)
  - monitor direction (`min`, `max`)
  - `patience`
  - `min_delta`
  - validation cadence
  - restore-best-weights / restore-best-checkpoint policy

The key design rule is:

- **model-intrinsic regularization stays in NodeK model math**
  Example: dropout inside attention/RNN/transformer blocks.
- **objective penalties and stopping logic stay in Ice**
  Example: weight decay policy, auxiliary-loss aggregation, early stopping, restore-best.

That split keeps the layer/kernel behavior where it belongs while still centralizing the training-time behavior once.

**Phase 4: Step lifecycle and override ladder**
- Redesign `Model` around a progressive-disclosure stack:
  - `forward(batch)`
  - `compute_loss(predictions, batch)`
  - `learn_step(batch, ctx)`
  - `valid_step(batch, ctx)`
  - `test_step(batch, ctx)`
  - `infer_step(batch, ctx)`
- Default implementations should delegate downward so users only override what they need.
- This is the key to avoiding bespoke loops everywhere.

Regularization should plug cleanly into this override ladder:

- `forward(batch)` should respect model training/eval state, which is where architectural regularizers like dropout naturally branch.
- `compute_loss(...)` should be the shared point where objective penalties are composed:
  - base supervised loss
  - auxiliary heads
  - explicit regularizer penalties
- `learn_step(...)` should remain the escape hatch for workloads with unusual optimization flows, but it should still receive the same centralized recipe/callback/checkpoint/monitoring context instead of rebuilding that machinery.

**Phase 5: Data/source substrate**
- Introduce `ice::Source` as the shared data lifecycle abstraction.
- Responsibilities:
  - `prepare()`
  - `learn_batches()`
  - `valid_batches()`
  - `test_batches()`
  - `infer_batches()`
  - `transfer_batch(...)`
  - pre/post transfer hooks when needed
- `TimeSeriesDataSet` becomes one producer into this system, not a parallel universe.

**Phase 6: Callback and event system**
- Add a centralized callback/event pipeline.
- Hooks like:
  - run start/end
  - epoch start/end
  - batch start/end
  - before backward
  - after backward
  - before optimizer step
  - checkpoint save/load
  - exception handling
- This is where progress, logging, early stopping, LR schedules, checkpointing, profiling, and custom user behavior should plug in.

Early stopping should not be a hard-coded if-statement living inside one trainer loop. It should be implemented as a first-class shared callback/policy with state:

- current best monitor value
- epochs/steps without improvement
- stop reason
- best-checkpoint reference
- optional best-weights restore action

That makes it reusable across `Outline`, custom `Model` subclasses, TFT-style models, NodeB deep consumers, and any future Ice catalog model.

**Phase 7: Real checkpointing and state persistence**
- Upgrade from “save parameters” to full training checkpointing.
- Centralize artifact format for:
  - model weights
  - optimizer state
  - scheduler state
  - epoch/global step
  - best monitored metric
  - RNG state
  - preprocessing artifacts
  - user-defined extra state
- Keep weights-only save/load as a simpler path, but make full checkpointing first-class.

This phase should explicitly support the regularization story too:

- if early stopping is active, checkpoint state must preserve the monitor history needed to resume training correctly
- if `restore_best` is enabled, the checkpoint manager should know which artifact is the current best one
- if objective penalties or auxiliary losses carry state, that state must be persisted alongside the model/trainer state
- if preprocessing or validation policies are part of the training contract, those must be serialized too

**Phase 8: Precision, accumulation, and execution policies**
- Add shared execution policies into `Coach` and `Recipe`.
- Centralize:
  - gradient accumulation
  - precision mode
  - inference/no-grad mode
  - anomaly/non-finite behavior
  - deterministic mode
  - validation frequency
  - fast-dev/debug runs
- This should apply uniformly to every Ice model instead of being reimplemented per model family.

This phase should consume the lower-level NodeK grad-mode work from Phase 1.5:

- Ice should never again implement its own parameter-flipping pseudo no-grad guard.
- `infer()`/`assess()`/validation/test/predict loops should all enter the shared lower-level no-grad scope through one path.
- When NodeK eventually has the equivalent lower-level machinery for a stricter inference mode, Ice can expose that as an execution policy here without changing model code.

**Phase 9: Migration and hardening**
- Migrate all current Ice training surfaces and the eligible NodeB deep-learning consumers onto the new substrate.
- Remove duplicated loops and redundant local serializers.
- Add regression tests proving old behavior still works through centralized infrastructure.
- Only after this do we aggressively expand the model catalog.

Hardening must include regularization and no-grad regression coverage:

- dropout behaves differently in training vs evaluation where expected
- no-grad truly prevents graph extension during inference/evaluation
- early stopping reacts to the configured monitor/direction/patience correctly
- restore-best policy actually restores the best monitored state
- resumed training continues monitor/checkpoint state correctly
- objective penalties are applied exactly once and are reflected in reported losses

**Why 9 phases instead of fewer**
- Fewer phases would blur architecture and migration together and create churn.
- More phases would be too granular and burn time on project management instead of code.
- Nine is enough to separate foundation, extension points, persistence, and migration cleanly.

**What “great” looks like at the end**
- One shared trainer core.
- One shared batch abstraction.
- One shared config surface.
- One shared callback system.
- One shared checkpoint format.
- One shared data/source lifecycle.
- One shared regularization policy surface.
- One real lower-level NodeK grad-mode/no-grad substrate.
- Model authors mostly write model math, not plumbing.
- NodeB and future Ice models reuse infrastructure instead of cloning loops.

**Suggested implementation order**
1. `ice::Batch`
2. NodeK `GradMode` / `AutoGradMode` / `NoGradGuard`
3. `ice::Coach`
4. `ice::Recipe`
5. `Model` override ladder
6. `ice::Source`
7. callbacks + centralized early stopping
8. checkpointing
9. precision/accumulation/execution policies
10. migration + hardening

**Source-grounded note on the no-grad design**

The lower-level no-grad recommendation above is not arbitrary. It is modeled on PyTorch's public C++ docs and source:

- PyTorch C++ documents `torch::NoGradGuard` as an RAII guard that disables gradient computation in its scope and restores the previous mode afterwards.
- PyTorch's `c10/core/GradMode.h` implements this as a thread-local grad-mode flag with:
  - `GradMode::is_enabled()`
  - `GradMode::set_enabled(bool)`
  - `AutoGradMode(bool)`
  - `NoGradGuard : AutoGradMode(false)`
- PyTorch's stricter `InferenceMode` goes further and also manipulates autodiff/dispatch execution state, which is why it should **not** be faked in NodeK before the lower-level runtime can support that cleanly.

If you approve, I’d begin with **Phase 1, 1.5, and 2 together**, because `Batch` without `Coach` is just a container, `Coach` without `Batch` still leaves the current shape problem unresolved, and a centralized trainer should not be built on top of a temporary Ice-local no-grad workaround.
