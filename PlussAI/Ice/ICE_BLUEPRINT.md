# Ice Blueprint (PyTorch-Lightning + PyTorch-Forecasting-shaped space)

## 0. Elevator pitch

Ice's entire value proposition is one word: **convenience**.

> What you want to do has already been done for you — just customize it to your spec. On top of
> that, we also ship niche forecasting, NLP, LLM, and transformer-family models (the TFT-shaped
> stuff) ready to use instead of you implementing them from scratch — and you can still customize
> those to your spec too.

Ice is deliberately the "easy but escapable" layer on `NodeK`, the same relationship jQuery/React
has to vanilla JavaScript: if you want full control, you write NodeK directly; if you want the
common case handled for you with rich options to tune it, you use ice. Neither is wrong — they're
different jobs. **Ice's law**: anything that takes 5+ lines of NodeK plumbing (parameter wiring,
optimizer setup, the train loop, gradient clipping, non-finite guards, state-dict persistence)
should be a single line in ice, with deep, well-documented options for the cases where the default
isn't enough.

## 1. Positioning

**Ice is a generic model/training layer over `NodeK` — not a NodeB feature.** It is scoped and
justified entirely on its own, the same way NodeK, NodeJ, and Lymph are each generic and
domain-agnostic. NodeB is one *consumer* of ice (its Sequence-backend training loops are the
first, most convenient migration target, per §7.2) but is not the reason ice exists and does not
shape ice's core abstractions. Ice's `core`/`train`/`io` layers must stay just as usable for a
tabular forecasting model, an image model, or anything else built on NodeK as they are for text.

Ice sits over `NodeK` the same way Lymph already sits over `NodeJ`:

- `NodeJ`: classical ML engine (raw estimators, optimizers, splitters)
- `Lymph`: convenience layer over NodeJ — workflow-oriented `learn`/`infer`, DataFrame-oriented
  preprocessing/evaluation, one-call factory helpers (`kernel_factory::`, `bayes_factory::`,
  `neighbor_factory::`), cross-validation/tuning helpers, online wrappers.
- `NodeK`: deep learning engine (raw tensors, `Module`, optimizers, losses — the PyTorch-equivalent
  base layer)
- `Ice`: convenience/model layer over NodeK — same shape as Lymph, aimed at deep learning.
  It plays two roles at once, matching two different PyTorch-ecosystem packages:
  1. **PyTorch-Lightning role** — a trainer that removes the hand-written `optimizer.clear();
     loss.reverse(); optimizer.step();` loop, gradient clipping, non-finite-loss/gradient
     diagnostics, and generic model-state persistence, so a model author defines *what* to
     compute, not the training mechanics.
  2. **PyTorch-Forecasting role** — a growing catalog of ready-made "nice, niche" architectures
     that NodeK itself intentionally does not provide (Temporal-Fusion-Transformer-shaped
     forecasting models, causal language models, seq2seq/translation encoder-decoders, retrieval
     components, ...), the way pytorch-forecasting layers TFT and friends on top of plain PyTorch.

Once ice exists, it also happens to be where the deep-NLP capability gap identified in
`NodeB/NODEB_BLUEPRINT.md` — machine translation, summarization, Q&A, retrieval, text generation,
general language modeling — belongs, because those six capabilities are generative/seq2seq-shaped,
not pipeline-shaped, and a NodeK-backed model layer is their natural home regardless of NodeB. That
NodeB benefits is a consequence of ice existing, not ice's design brief.

## 2. Why this is buildable now: the evidence from this session

Four separate NodeB training paths were hand-written directly against NodeK this session
(`embed::Embedder`, `embed::ContextualEmbedder`, `TextCatStage`'s Sequence backend, and the shared
`detail::train_token_classifier` used by `TaggerStage`/`NerStage`). All four repeat the same shape:

1. Build/own a mix of raw trainable `Series` (an embedding table, a linear head) and NodeK `Module`
   parameters (a `TransformerEncoder`), merged into one `std::vector<Series*>` for the optimizer.
2. Construct `pluss::nn::optim::Adam` with hand-picked `GradientClipOptions` and
   `StepDiagnosticsOptions` (fail on non-finite loss/gradient/parameter) — configured identically,
   copy-pasted, in all four places.
3. Run `optimizer.clear(); loss.reverse(); optimizer.step();` in a loop, sometimes with an
   early-stopping delta check, sometimes without, inconsistently.
4. Hand-serialize model state: walk `Module::parameters()` in construction order, dump shape +
   flat values per tensor, reconstruct an identically-configured module on load and overwrite each
   parameter in place. Written from scratch in `token_classifier.cpp`; `Embedder`/
   `ContextualEmbedder` each solved persistence differently (or skipped it).

None of this is NodeB-specific — it is exactly the boilerplate a trainer abstraction exists to
delete. Ice's first job is to delete it, not to add new modeling capability.

## 3. Design Principles

1. **Convenience is the entire selling point.** Ice exists to make the common case a one-liner
   with rich, discoverable options behind it — not to add capability NodeK lacks. If a change to
   ice doesn't make some real task shorter, safer, or harder to get wrong, it's out of scope for
   ice and belongs in NodeK instead.
2. **Easy but escapable — the jQuery/React relationship to vanilla JS.** Ice is deliberately not
   the only way to use NodeK; it's the *convenient* way. A user who needs full control drops to
   NodeK directly, same object model, same `Series`/`Module` underneath — there is no cliff where
   ice-built models become opaque or unportable.
3. **Consistent verbs with the existing stack.** `learn`, `infer`, `assess`, `save`, `load` —
   matching `Workflow`/`WorkflowModel`/Lymph exactly, not PyTorch's `fit`/`predict` or Lightning's
   `training_step`. Cross-stack familiarity (NodeJ → Lymph → NodeB → Ice) matters more than
   resemblance to any one upstream framework.
4. **No hidden magic beyond what's necessary.** Users who want raw control keep using NodeK
   directly — ice is an on-ramp, not a wall. Every `ice::Model` should be inspectable down to its
   underlying `Series`/`Module` parameters.
5. **Numerical safety is a default, not an opt-in.** Gradient clipping and
   `fail_if_non_finite_*` diagnostics ship on by default (the four hand-written training loops this
   session all had to remember to configure this identically; a real user shouldn't have to).
6. **Pluss-native naming.** No verbatim `LightningModule`/`Trainer.fit`/`DataModule` naming lifted
   from PyTorch Lightning, and no verbatim pytorch-forecasting model-class names. A name doesn't
   need to be a literal English description of what it does (R's `tibble()` isn't a dictionary
   word either) — it needs to be distinctive, collision-free against both this codebase and the
   wider ecosystem, and become a household name through use. `ice::Outline` (decided; §4, §6) is
   the first example: short, checked against this codebase and known frameworks/languages before
   being picked, not descriptive by itself but unambiguous once learned.
7. **Prove the trainer on real, already-correct code before adding new modeling scope.** Migrate
   existing, tested NodeB training loops onto ice first — as the first, most convenient consumer,
   not the design driver — before building net-new "nice/niche" models. Do not start with the full
   generative NLP stack (§8).

## 4. Target User Surface (Draft)

**`ice::Outline`** is ice's answer to `pluss::nn::Schedule` (NodeK's own `nn.Sequential`
equivalent) — the common-case, one-liner way to compose existing layers into something trainable.
It owns a real `pluss::nn::Schedule` internally (composition, not inheritance) and adds
`learn`/`infer`/`assess`/`save` on top; nothing about the layers themselves is reimplemented, so
dropping to raw `pluss::nn::Schedule` at any point costs nothing (§3.2):

```cpp
#include <pluss/ice.h>

namespace ice = pluss::ai::ice;

auto model = ice::Outline(
    pluss::nn::Fusion(in_features, 24), pluss::nn::ReLU(), pluss::nn::Fusion(24, num_classes));

model.learn(x_train, y_train, ice::TrainOptions{
    .max_epochs = 60,
    .learning_rate = 5e-3,
    .loss = ice::LossKind::CrossEntropy,
    // .grad_clip / .fail_on_non_finite are on by default; shown here only for clarity.
    .grad_clip = {.mode = pluss::nn::optim::GradientClipMode::ByNorm, .threshold = 1.0},
});
auto predictions = model.infer(x_test);
auto report = model.assess(x_test, y_test, ice::LossKind::CrossEntropy);  // explicit, not remembered —
                                                                          // a remembered value wouldn't
                                                                          // survive save()/load()
model.save("model.bin");   // generic state-dict save/load, written once in ice::io
```

Implemented and tested as of this session — `PlussAI/Ice/{core,io}/*.cpp`,
`PlussAI/include/pluss/ai/ice/*.h`, `PlussAI/test/ice_model_test.cpp` — matching this section
exactly (the signatures above are real, not aspirational).

For architectures that aren't a straight layer chain, `ice::Model` is the escape hatch: define
parameters + `forward`/`loss` yourself and still get `learn`/`infer`/`assess`/`save`/diagnostics
for free.

```cpp
class MyClassifier : public ice::Model {
 public:
  explicit MyClassifier(Options opts)
      : embedding_(opts.vocab_size, opts.width),
        encoder_(TransformerEncoderOptions{...}),
        head_(opts.width, opts.num_classes) {
    register_module(encoder_);      // NodeK Module parameters, discovered automatically
    register_parameter(embedding_.weights());  // raw Series, tracked explicitly
    register_parameter(head_.weight());
  }

  Series forward(const ice::Batch& batch) const override { ... }
  Series loss(const Series& predictions, const ice::Batch& batch) const override { ... }
};

MyClassifier model{opts};
model.learn(train_batches, ice::TrainOptions{.max_epochs = 60, .learning_rate = 5e-3});
```

One-call factories, matching Lymph's `kernel_factory::`/`bayes_factory::` shape:

```cpp
namespace ice = pluss::ai::ice;

auto lm = ice::sequence_factory::causal_lm(ice::CausalLmOptions{.width = 256, .num_layers = 6});
lm.learn(corpus);
auto continuation = lm.generate("Once upon a time", ice::GenerateOptions{.max_new_tokens = 50});
```

## 5. Planned Folder/Module Layout

```text
PlussAI/Ice/
  core/          # Model base class, parameter registration (raw Series + NodeK Module), Batch
  train/         # the loop: optimizer wiring, grad clip, non-finite diagnostics, early stopping
  io/            # generic Module+Series state-dict save/load (generalizes token_classifier.cpp's
                 #  hand-rolled version — write it once, delete the three ad hoc copies)
  callbacks/     # logging/checkpoint/early-stop hooks, opt-in
  models/        # the "nice/niche" catalog: causal LM first (§8 Phase 3), then seq2seq, retrieval
  data/          # batching/collation bridging PlussMic/NodeB Doc into ice::Batch
  eval/          # perplexity, BLEU/ROUGE-style scoring for generative tasks
  factories/     # sequence_factory::, etc. — one-call builders returning ready-to-train models
  examples/
  test/
```

## 6. Parity Matrix (execution-oriented, mirrors NodeB's own spaCy matrix)

| Concept | PyTorch (Lightning) / pytorch-forecasting | Ice |
|---|---|---|
| Compose existing layers | `nn.Sequential(...)` | `ice::Outline(...)` — owns a `pluss::nn::Schedule`, adds `learn`/`infer`/`assess`/`save` |
| Define model math, not loop | `LightningModule` | `ice::Model` (`forward`/`loss`, `learn`/`infer`/`assess`) |
| Run training | `Trainer.fit(model, datamodule)` | `model.learn(data, TrainOptions)` |
| Gradient clipping / NaN guards | `Trainer(gradient_clip_val=..., detect_anomaly=...)` | `TrainOptions.grad_clip` / on-by-default diagnostics |
| Checkpointing | `ModelCheckpoint` callback | `ice::io` generic save/load, callback-optional |
| Ready-made niche architectures | `TemporalFusionTransformer`, etc. | `ice::models::` catalog (§8) |
| One-call builder | (less direct analogue) | `ice::sequence_factory::` — mirrors Lymph's `kernel_factory::` |

## 7. Integration Strategy

1. Ice depends on NodeK directly (`Series`, `Module`, `optim`, `losses`) — the same relationship
   Lymph has to NodeJ. Ice does not reimplement attention, losses, or optimizers; it orchestrates
   NodeK's.
2. NodeB's four hand-rolled training loops (`embed::Embedder`, `embed::ContextualEmbedder`,
   `TextCatStage`'s Sequence backend, `detail::train_token_classifier`) are the **first migration
   targets**, not a hypothetical future one — each already has a passing correctness test
   (`nodeb_embed_test`, `nodeb_contextual_embed_test`, `nodeb_textcat_test`,
   `nodeb_tagger_sequence_test`, `nodeb_ner_sequence_test`) that becomes the regression gate: after
   migrating a call site, the exact same test must still pass unchanged. "Migrating" doesn't
   uniformly mean rehoming onto the full `ice::Model` base class, though — §8 Phase 1 found
   `embed::Embedder`'s per-step negative-sampling loop doesn't fit `Model`'s generic forward/loss
   shape, and migrated onto standalone `ice::make_optimizer()` instead. Whether each of the
   remaining three fits `Model` or needs the same lighter-weight treatment is decided per call site
   in Phase 2, not assumed from this list.
3. NodeB becomes a consumer of ice's trainer rather than reimplementing it per task file; NodeB
   keeps owning tokenization, `Doc`, and task-specific decoding (BIO spans, tag application).
4. The deep-NLP gap (translation/summarization/QA/retrieval/generation/LM) becomes ice's
   `models/` catalog, sharing NodeB's `Doc`/tokenizer for text I/O rather than duplicating it.

## 8. Implementation Roadmap

### Phase 0 (Scaffold)
- Folder/include scaffolding; `ice::Model`, `ice::Batch`, `ice::TrainOptions` interfaces.
- No new modeling capability yet.

### Phase 1 (Trainer MVP) — done, with one course correction

`ice::Model` (generic `forward`/`compute_loss`, `learn`/`infer`/`assess`/`save`/`load`, enum-based
`TrainOptions`), `ice::Outline` (the `nn::Schedule`-equivalent one-liner), and `ice::io` (generic
checksummed parameter persistence) are built and tested (`PlussAI/test/ice_model_test.cpp`) on
synthetic, non-NLP tabular tasks (classification, regression, a hand-written custom `Model`
subclass, chunked inference, save/load, and numerical-safety-fails-loudly), per §3.7's "prove it on
something small first."

**`embed::Embedder` was *not* migrated onto `ice::Model` as originally planned** — re-reading its
actual loop closely revealed it doesn't fit `Model`'s generic shape at all: skip-gram negative
sampling redraws random negative samples on *every mini-batch* (no fixed `(x, y)` pair to hand a
`DataLoader`), and its loss is two separate `BCEWithLogits` terms (positive pairs + negative
samples) summed, not one `forward()` → one `compute_loss()`. Forcing that into `Model`'s loop would
have meant contorting the abstraction to fit one case — exactly the premature-abstraction mistake
`ice` exists to avoid, not commit.

The fix, mirroring how PyTorch Lightning handles the same tension (a `training_step` hook for
exactly this): extracted the part of `Model`'s internals that's genuinely reusable *outside* its
loop too — enum-dispatched optimizer construction with gradient clipping and non-finite diagnostics
already wired in — into a standalone **`ice::make_optimizer()`** (`pluss/ai/ice/optim.h`).
`Model::learn()` calls it internally; `embed::Embedder::learn()` now calls it directly, replacing a
hand-rolled `pluss::nn::optim::Adam` construction that — worth noting — had **no gradient clipping
and no non-finite diagnostics at all** before this. Embedder's actual skip-gram loop is unchanged
and stays bespoke NodeB code; the regression gate (`nodeb_embed_test.cpp`) passed unmodified.

Net effect: real, working code that previously lacked ice's safety net now has it, without
inventing scope ice doesn't need. The lesson for Phase 2: don't assume a call site fits `ice::Model`
until its loop has actually been read closely enough to check.

### Phase 1.5 (Device selection) — done

`TrainOptions::platform` (`TrainPlatform{CPU, GPU, Auto}`, `pluss/ai/ice/platform.h`) plus
`ice::resolve_platform()` and `Model::to(Platform)`. `Model::learn()` resolves the requested
platform once per call against a real runtime query (`pluss::cuda::is_ready()` /
`pluss::mps::is_ready()` — confirmed genuine device-availability checks, not compile-time guesses),
moves the model and both the training and validation data there, and wires the resolved platform
into the `DataLoader` (`DataLoaderOptions::target_platform`) so every batch lands on the right
device without the caller doing anything. `infer()`/`assess()` match their input to wherever the
model's parameters currently live, so a model trained with `TrainPlatform::GPU` stays usable without
the caller re-specifying a device. `TrainPlatform::GPU` warns (`pluss::validate::warn`) and falls
back to CPU when no GPU is present; `TrainPlatform::Auto` does the same silently. Default is
`TrainPlatform::CPU`, matching ice's behavior before this option existed — opting into acceleration
is always a deliberate choice, not an implicit one. Covered by
`run_ice_platform_fallback_checks` in `ice_model_test.cpp` (exercises the actual fallback path,
since CI hardware has no GPU).

**Explicitly out of scope: distributed / multi-GPU training.** Confirmed via exhaustive research
that NodeK/CodeM has zero underlying primitives for this — no process groups, no collective
communication (no NCCL/MPI/gloo/all_reduce/DDP anywhere), and, more fundamentally, no way to even
*address* a second local GPU (`Platform::CUDA` is one undifferentiated value, no device-index
concept at all). This isn't a missing convenience ice can wrap into existence — it's new
engine-level capability, squarely outside Design Principle #1 (§3: ice adds convenience over
existing NodeK capability, not new capability NodeK lacks). No stub/placeholder option was added for
it either, since an enum value that always throws "not implemented" adds API surface without adding
value. Revisit if/when NodeK/CodeM grows device-indexing and a communication backend.

**No-grad guard ownership.** `ice::NoGradGuard` (`pluss/ai/ice/no_grad.h`) remains ice-local for now
and is wired into `infer()`/`assess()`, but it's understood to be a stopgap: the user is building a
proper NodeK/CodeM-level equivalent separately, using libtorch as reference/correctness inspiration.
Once that lands, `ice::NoGradGuard` should be deleted in favor of it — ice's own version exists only
because no engine-level one did yet, not because no-grad is conceptually an ice concern (it isn't;
it belongs wherever `tracks_slope_` itself lives).

### Phase 2 (Generalize to Module-based models) — done, none fit `ice::Model`

Read `detail::train_token_classifier` (tagger/NER), both of `TextCatStage`'s Sequence-backend
branches (multi-label `BCEWithLogits`, multiclass `CrossEntropy`), and `embed::ContextualEmbedder`
closely (per the Phase 1 lesson — confirm, don't assume). All three turned out to share the *exact*
shape mismatch already found for `embed::Embedder`, for the same underlying reasons:

1. **Architecture discovered from data, not fixed beforehand.** Each one calls `indexer.learn(docs)`
   inside its own `learn()` to build a vocabulary, then sizes its embedding table (and, for
   `ContextualEmbedder`, its output projection) off that vocab. `ice::Model` assumes the caller
   already constructed a fixed architecture before calling `learn()`.
2. **Input isn't a flat `Series`.** `embedding->forward(batch)` takes a structured
   `runtime::SequenceBatch` (token ids + position ids + padding mask + valid mask), not the single
   `Series x` that `Model::forward(const Series&)` assumes — there's no way to slice that structure
   row-wise through `ice`'s `DataLoader`.
3. **Full-batch, not mini-batch.** All three build one `SequenceBatch` covering the *entire* training
   set and run one forward/backward per epoch over all of it — not `ice::TrainOptions::batch_size`-
   sized mini-batches.
4. **Different convergence rule.** All three stop early on *train*-loss-delta
   (`|previous_loss - loss| < tolerance`), not `ice`'s val-loss-plateau `early_stopping`.

None of this is a defect in `ice::Model` — it's simply a different, legitimate training shape
("build-vocab-then-full-batch-train-a-small-Transformer-from-raw-Docs") that every NodeB
deep-learning call site so far happens to share, and it is a shape `ice::Model` was never meant to
cover (§3.7: don't force an abstraction to fit a case it wasn't designed for).

**What did apply, and got migrated:** `ice::make_optimizer()`, same as Embedder in Phase 1. All
three previously hand-rolled `pluss::nn::optim::Adam` construction directly; `token_classifier.cpp`
already duplicated ice's own default clip/diagnostics settings (ByNorm/1.0, fail-loud), so its
migration is pure de-duplication. `TextCatStage`'s two Sequence branches and
`ContextualEmbedder` had **no gradient clipping and no non-finite diagnostics at all** before this —
migrating them onto `ice::make_optimizer` is a real new safety net, not just cleanup, and both loops
were also wrapped in the same try/catch-and-fail-loudly pattern `token_classifier.cpp` already used
(they previously had no try/catch around the training loop at all). All four existing regression
tests (`nodeb_tagger_sequence_test`, `nodeb_ner_sequence_test`, `nodeb_textcat_test`,
`nodeb_contextual_embed_test`) pass unchanged.

### Phase 3 (Persistence) — done, `ice::io` doesn't fit; found and fixed real duplication instead

The draft plan above (point the three hand-rolled serializers at `ice::io::save_parameters`/
`load_parameters`) turned out not to hold up once read closely. `ice::io`'s functions are a
standalone **file** format (`std::ofstream out{filepath}`, its own magic line + schema + FNV-1a
checksum) for a flat `vector<Series*>` — nothing else. What `token_classifier.cpp`/`textcat.cpp`
actually need is a set of `"<key>=<value>"` **lines to embed inline** inside a larger
`dump_state_lines()`/`load_state_lines()` blueprint text that also carries non-numeric state
(vocabulary strings, hyperparameters, an `Indexer`'s tokenizer state) that isn't a `Series` at all —
a different problem shape `ice::io` was never meant to solve. Forcing it through `ice::io`'s
file-based API would have meant a bigger, riskier restructuring for no real gain.

Also corrected an inventory error: there are not three divergent serializers, there are two.
`embed::ContextualEmbedder` has **no persistence at all** — no `dump_state`/`load_state`/`save`/
`load` on the class (`PlussAI/include/pluss/ai/nodeb/embed.h`) — so it was never a duplication
target; it's a capability gap, worth flagging but out of scope for a "generalize what exists" pass.

What *was* real, and fixed: `textcat.cpp` already `#include`s `tasks/deep/sequence_heads.h`, which
already exposes shared `hex_encode_string`/`hex_decode_string`/`join_doubles_line`/
`parse_doubles_line` helpers (used by `token_classifier.cpp` and the tagger/NER Sequence rewrite) —
but `textcat.cpp` had its own byte-for-byte duplicate `hex_encode`/`hex_decode`/`parse_doubles` plus
a `double`-only `join_numbers` sitting in its own anonymous namespace instead of using them. Deleted
the duplicates and repointed all 13 call sites at the shared, already-public versions (qualified as
`detail::` — `textcat.cpp` lives in `pluss::ai::nodeb`, one namespace level up from
`pluss::ai::nodeb::detail` where `sequence_heads.h`'s helpers live, unlike `token_classifier.cpp`
which is declared directly inside `detail`). `join_numbers` stays as a local template — genuinely
needed for its one non-`double` caller (`LexiconMapperState::document_frequency` is
`vector<int64_t>`), not a duplicate. `nodeb_textcat_test` plus the full 20-binary NodeB/ice
regression suite pass unchanged.

### Phase 4 (First "nice/niche" model — closes part of the NodeB gap list)
- A causal/autoregressive language model with a real decoding loop (greedy, then sampling) is the
  natural first entry: `embed::ContextualEmbedder` already built the embedding+Transformer+causal
  masking half of this during Phase 2's migration; "generation" is that plus a decode loop. This
  directly closes gap items **#10 (text generation)** and **#11 (language modeling)** from
  `NodeB/NODEB_BLUEPRINT.md`.

### Phase 5+ (Expand incrementally)
- Seq2seq (encoder-decoder) architectures unlock translation (#6) and summarization (#7).
- A retrieval component (nearest-neighbor search over `embed::ContextualEmbedder`/ice embeddings)
  unlocks information retrieval (#9) and, combined with the causal LM, a basic Q&A pattern (#8).
- Each addition is its own `ice::models::` entry built on the Phase 1–3 foundation, not a
  redesign of it.

#### `ice::models::TemporalFusionTransformer` — done

The first non-NLP `ice::models::` entry (`PlussAI/Ice/models/tft/`, public header
`pluss/ai/ice/models/tft.h`): multi-horizon quantile forecasting per Lim et al.'s TFT architecture
(variable selection networks, gated residual networks throughout, an LSTM encoder/decoder seeded
from a static-covariate context, interpretable causal self-attention, quantile/pinball output). An
original implementation — every building block (`elu`, `LayerNorm`, `GatedResidualNetwork`,
`GatedAddNorm`, `VariableSelectionNetwork`, `quantile_loss`) was written from the published,
well-known formulas, not ported from any reference codebase; `Ice/models/tft/tft_blocks.h` documents
each formula and which engine primitives it's built from.

**Remaining deliberate scope cut** (documented in `tft.h`/`tft_blocks.h`, not silently assumed): one
shared LSTM context broadcast across every layer rather than a distinct per-layer projection.
`TFTTrainOptions` is its own struct,
not `ice::TrainOptions` reused — TFT's loss is always pinball (no `LossKind` choice) and it trains
on a multi-tensor `TFTBatch`, not a single `(x, y)` Series pair, so it can't reuse `ice::Model`'s
`train_loop_` — but `ice::make_optimizer()` and the `TrainPlatform`/`resolve_platform()` device
convenience both carry over unchanged.

**Categorical variables (Depth pass 1).** `TFTOptions` gained
`static_categoricals`/`encoder_categoricals`/`decoder_categoricals`
(`std::vector<TFTOptions::CategoricalVariable{variable_index, cardinality}>`) — any variable slot
not listed stays continuous, matching the original numeric-only behavior exactly. Categorical
values still live in the same `TFTBatch` tensors as continuous ones, as the integer category code
stored as a float; no new tensor field. `VariableSelectionNetwork` gained a new `Embedding`
building block (`Ice/models/tft/tft_blocks.h` — a plain learned `[cardinality, hidden_size]` table,
looked up via `index_select`, built with the same allocate-then-register-then-`copy_()` pattern as
the engine's own `nn::` modules) and now dispatches each variable slot to either its small
per-variable GRN (continuous) or an `Embedding` (categorical), both producing a `[...,
hidden_size]` representation.

This surfaced a real correctness issue in the *existing* continuous-only VSN, not just a gap in
what it accepted: `flatten_grn_` (the sub-network that computes the softmax selection weights) was
built to take the *raw* `[..., num_vars]` packed values as its input. For continuous variables
that's merely a design choice; for a categorical slot it's actively wrong — an arbitrary category
code fed straight into a GRN gets treated as if it were an ordinal/continuous quantity, when
category 3 has no numeric relationship to category 1. Fixed by changing `flatten_grn_` to operate
on the *concatenation* of every variable's already-processed `[..., hidden_size]` representation
(`[..., num_vars * hidden_size]`) instead — the standard formulation, and correct uniformly for
both variable kinds. Verified the redesign didn't regress the continuous-only path (`ice_tft_test`'s
original numeric-only check still passes with the same thresholds) and added a dedicated
categorical check with a *deliberately non-monotonic* category→level mapping (category 0 → level
1.2, category 1 → −0.8, category 2 → 0.3, category 3 → −1.5) — a model that silently treated the
raw code as ordinal could not recover this particular mapping, so passing is real evidence the
embedding path works, not just that training completed.

**Two real engine-level bugs surfaced while getting this to actually train — both fixed upstream in
NodeK/CodeM, not worked around in ice:**

1. **A plain (non-`Module`) struct holding top-level `pluss::nn::` submodules never got its
   parameters' `tracks_slope_` flag fixed up.** `Module::module_record()` is what normally flips a
   freshly-constructed child's parameters to `tracks_slope_(true)` — `FusionImpl` (and eight other
   modules: `ConvImpl`, `Conv1dImpl`, `Conv3dImpl`, `MultiheadAttentionImpl`,
   `TransformerEncoderLayerImpl`, `TransformerDecoderLayerImpl`, `TransformerEncoderImpl`,
   `TransformerDecoderImpl`) built their weight/bias as *undefined* Series at the point
   `parameter_record()` first saw them, then reassigned them to real tensors afterwards in
   `reset_parameters()`, so `parameter_record()`'s own "if defined, track" check missed them; the
   fix-up only happened when some ancestor `Module` registered the child via `module_record()`
   (which re-walks the child's now-real `parameters()`). `TemporalFusionTransformer::Impl` is a
   plain struct (deliberately, for the pImpl pattern every other `embed::`/`ice::` class uses) that
   directly owns several top-level submodules (`lstm`, `attention`, `output_head`,
   `static_enrichment_grn`, the three gated skip connections) with nothing above them to run that
   fix-up — this is what first surfaced it. **Fixed at the source, twice** — the first pass
   (register-before-assign reordered, plus a `Module::retrack_parameters()` safety net) closed the
   immediate bug but left a *convention* (reset-before-register) that nothing enforced; a second
   pass replaced it with a structural fix modeled directly on libtorch's
   `torch::nn::Module::register_parameter`/`LinearImpl::reset()`: `parameter_record()` now
   `PLUSS_CHECK`s that a Series is already `defined()` before it will register it, allocation
   (`Series::empty(shape, dtype, platform)`) and registration happen together in one method per
   class, and `reset_parameters()` fills in values with the in-place `Series::copy_()` instead of
   reassigning the member — so registration and tracking can never be out of sync with the value,
   no matter how many times `reset_parameters()` runs, and a future module that gets the order
   backwards fails loudly at construction instead of silently training with dead gradients.
   `Module::retrack_parameters()` was removed once nothing called it anymore. Verified two ways:
   two standalone probes confirmed `Fusion`/`MultiheadAttention` construction now produces
   bit-identical values to the pre-fix code path, and the full NodeK `nn::` module test suite plus
   ice/lymph/NodeB pass. `TemporalFusionTransformer::Impl`'s own explicit fix-up loop became
   redundant once the first pass landed and was removed then (rebuilt/retested — `ice_tft_test`
   still passes; its training budget was separately widened from 150 to 400 epochs after this
   change exposed that the original budget left quantile-interval calibration marginal — median
   accuracy and quantile ordering were unaffected).
2. **`Series::narrow_copy()` cloned and silently detached from the autograd graph — confirmed
   empirically** (a parameter narrowed-and-summed reported zero gradient after `reverse()`; the same
   parameter through `index_select()` reported the correct one). Root cause: `select_range()` (what
   `narrow`/`slice`/`narrow_copy` are all built on) already registers a correct backward via
   `gather()`'s scatter-add rule, and already returns an independent, freshly-materialized buffer —
   but `narrow_copy`'s dispatch in `PlussQit/src/CodeM/compat/Invoke.cpp` appended a redundant
   `.clone()` on top, and `Series::clone()` (unlike its sibling `Series::contiguous()`) never
   registers a backward at all. **Fixed at the source**: dropped the redundant `.clone()` from
   `narrow_copy`'s dispatch — `select_range()` alone already satisfies `narrow_copy`'s
   storage-independence contract, so nothing else changes. Once fixed, `Ice/models/tft/tft_blocks.h`'s
   `index_range()` workaround (an `index_select()`-based reimplementation of narrowing) became pure
   redundancy and was removed — every call site reverted to plain `.narrow_copy(...)`, rebuilt and
   retested (`ice_tft_test` still passes). Verified against the same 31-binary suite.
3. **Investigated and ruled out as a third bug**: `Series::where()` was suspected of the same class of
   silent-detach issue (the ELU/quantile-loss formulas in `tft_blocks.h` were originally written to
   avoid it, on the belief it didn't propagate gradients). Read `Series::where()`'s implementation in
   `OpsInternal.cpp` directly — it correctly forwards to the differentiable `ops_internal::where()`
   free function on every branch — then confirmed with a standalone empirical probe (parameter →
   `where()` → `sum()` → `reverse()`, correct gradient on both branches). `where()` was never broken;
   the original suspicion was very likely an artifact of the `narrow_copy` bug being active in the
   same pipeline at the time. The relu-based ELU/quantile-loss formulas were kept as-is (already
   validated, no simpler than a `where()`-based version) but their comments were corrected to state
   this rather than blame `where()`.

Both were found the hard way: the model trained (no exceptions, no NaNs) but never learned anything
beyond predicting a constant, which is a much harder failure mode to root-cause than a crash — worth
remembering as a category of bug to check for early (verify gradients are actually non-zero and
reaching every parameter) rather than only checking "does it run" on any future from-scratch model.

Tested end-to-end in `PlussAI/test/ice_tft_test.cpp` on a synthetic multi-horizon task with a known
exact answer (`target = static_level + 0.5 * known_future_position`) — checks the median forecast
actually recovers the relationship (not just "ran without crashing"), quantile interval coverage,
prediction determinism, attention weights sum to 1, and `to(pCPU)` round-tripping.

## 8.5 ice::data::TimeSeriesDataSet — generic windowing/covariate dataset (Depth pass 2)

`TFTBatch` started as a raw, pre-shaped tensor struct: a caller had to do their own group-by-series
windowing, categorical vocabulary encoding, and static/known/observed variable packing by hand, and
separately keep `TFTOptions::static_size/encoder_size/decoder_size/*_categoricals` in sync with
whatever that produced. `pluss::ai::ice::data::TimeSeriesDataSet`
(`include/pluss/ai/ice/data/time_series_dataset.h`, `Ice/data/time_series_dataset.cpp`) replaces
that hand-rolled step — and, deliberately, is **not TFT-specific**: it lives outside
`Ice/models/tft/`, in its own `ice::data` module, so a second future forecasting model can reuse it
without depending on TFT at all. `TFTBatch` now has an `explicit` constructor from
`data::TimeSeriesBatch`, and `TemporalFusionTransformer::learn()`/`predict()`/`assess()` gained
overloads taking a `data::TimeSeriesBatch` directly; `TFTOptions::CategoricalVariable` is a type
alias for `data::CategoricalVariable`, not a duplicate — the same categorical bookkeeping flows
through both layers without a translation step.

**Design grounding.** Checked against pytorch-forecasting's `TimeSeriesDataSet` (the actively
maintained, most authoritative reference for this exact abstraction) and against a downloaded
third-party TFT port (`github.com/mattsherar/Temporal_Fusion_Transform`) for a second, independent
data point — the latter turned out to be a rough, not-fully-integrated port (TensorFlow imports
alongside PyTorch model code, a `trainer.ipynb` that calls its own dataset class with the wrong
argument list, 12 open GitHub issues including a duplicate-column-definition bug) rather than an
authoritative source, so it informed the taxonomy (confirmed, not copied: static/known/observed ×
categorical/continuous, matching the published TFT paper) while its own bugs became a checklist of
things to deliberately do differently:

- **Gap detection.** The reference assumes row-adjacency implies time-adjacency in its sliding
  window and never validates it; `TimeSeriesDataSet::from_dataframe()` requires `time_idx` to be
  *strictly contiguous* (step 1) within each group — a gap starts a new run for windowing purposes
  instead of silently producing a window that splices together non-adjacent timestamps.
- **Categorical out-of-vocabulary handling.** The reference's `sklearn.LabelEncoder`-based encoding
  has no fallback and errors on an unseen category at inference time. Every fitted
  `CategoricalVocabulary` reserves a trailing "unknown" code; `from_dataframe(df, options,
  vocabularies_from)` (the second overload, for encoding a validation/test split against a
  training split's already-fitted vocabulary) maps unseen values there instead of throwing.
- **No fragile positional convention between encoder and decoder.** The reference packs known and
  observed variables into one tensor and has the model skip a caller-configured `num_masked_series`
  prefix to hide observed-only columns from the decoder — correct only if column order and that
  count are kept in exact sync by hand, with nothing enforcing it. `TimeSeriesBatch` keeps
  `encoder_inputs`/`decoder_inputs` as separate tensors from the start (encoder gets known +
  observed variables, decoder gets known only) — there's no shared packing convention to get wrong.

**Windowing implementation**: sorts by `(group_ids..., time_idx)` via `DataFrame::sort_values`,
computes every valid window's starting row index on the host (inherently control-flow, not a
batched numeric op — the same category of exception already established for `pack_padded_sequence`
and `ice::Model::dataframe_matrix_`'s column-name enumeration), then materializes every window's
tensor data via **one batched `Series::index_select()` per variable group** (static/encoder/decoder)
against a flat gather-index tensor — not a per-window or per-row loop.

**Deliberate scope cut, documented rather than silently assumed**: no continuous normalization or
target scaling (pytorch-forecasting's `TimeSeriesDataSet` supports per-group or global scaling with
automatic prediction denormalization; both reference implementations checked also normalize). A
caller who wants it can standardize their own continuous columns before calling
`from_dataframe()`/`from_series()`, the same way `ice::Model::TrainOptions::standardize` works
today. Left for a future pass.

Tested in `PlussAI/test/ice_time_series_dataset_test.cpp` (windowing shape/count/content
correctness, gap detection, short-group dropping, categorical vocabulary fit/reuse/OOV mapping,
`from_series()` auto-deriving tensor widths) and end-to-end in `ice_tft_test.cpp`
(`run_ice_tft_dataset_integration_checks`: a long-format DataFrame through
`TimeSeriesDataSet::from_dataframe()` → `TFTOptions::from_dataset()` →
`TemporalFusionTransformer::learn()`/`predict()`, checking the model actually recovers the known
relationship, not just that the pipeline runs) and in the full-capability demo's §14.

## 8.5.1 Full-capability demo (documentation seed)

`PlussAI/examples/Ice/ice_full_capability_demo.cpp` exercises the entire raw public API surface —
deliberately with no new convenience wrappers around `ice::` itself, so any awkwardness shows up
directly rather than getting smoothed over. Builds and runs as `plussai_ice_full_capability_demo`.
Covers: a custom `ice::Model` subclass built from raw `register_parameter()` (not composed
layers), `ice::Outline`, all three `LossKind` values, all three `Model::learn()` overloads,
`infer()` with and without `chunk_size`, `assess()`, `save()`/`load()`, the standalone
`ice::make_optimizer()` escape hatch, `TrainPlatform`/`resolve_platform()`, `ice::io::save_parameters`/
`load_parameters` used directly (not through `Model`), `ice::NoGradGuard` used directly, and
`ice::models::TemporalFusionTransformer` end to end (both `learn()` overloads, `predict()`,
`assess()`, `to()`), a `pluss::nn::TransformerEncoder` composed directly into an `ice::Outline`
(its `forward(src, mask={}, padding={}, is_causal=false)` is Series-in/Series-out, so it — and a
plain lambda mean-pooling stage right alongside it — fit `Schedule`'s stage contract with no
wrapper needed), and a `pluss::nn::LSTM` trained via a raw `ice::make_optimizer()` loop (its
`forward()` returns `std::tuple<Series, std::tuple<Series, Series>>`, so unlike Transformer it
does *not* fit `Outline`'s Series-in/Series-out contract — the same "doesn't fit `Model`'s generic
shape" scenario `ice::make_optimizer()` exists for, this time with a real multi-module parameter
list spanning two separate `nn::` modules rather than one hand-rolled linear layer), and §14:
`ice::data::TimeSeriesDataSet::from_dataframe()` turning a long-format panel into a windowed batch,
`TFTOptions::from_dataset()` deriving the model's sizing from it, and
`TemporalFusionTransformer::learn()`/`assess()` taking that `data::TimeSeriesBatch` directly.

One real gap surfaced by writing it against the raw API rather than through a wrapper, since fixed:

- `Model::learn(DataFrame, target, options)` with `TrainOptions::standardize = true` used to
  standardize features internally without persisting or exposing the per-column mean/std, and
  `assess()`/`infer()` only accepted `Series` — a caller who trained with `standardize = true` had
  no ice-provided way to reproduce the exact same transform downstream. **Fixed**: `Model` now
  fits and keeps `standardize_mean_`/`standardize_std_` as members (private, applied via
  `apply_standardize_()`), and gained `DataFrame`-taking overloads of both `assess(df, target,
  loss)` and `infer(df, target = {}, chunk_size = 0)` that reapply them automatically (a no-op for
  models that never used `standardize = true`). `infer()`'s `target` parameter defaults to empty
  (genuinely new data to predict on typically has no target column at all) but can be set to the
  same name used in `learn()` when a caller reuses that same `DataFrame` — reusing it without
  passing `target` is exactly the footgun `ice_model_test.cpp`'s new
  `run_ice_dataframe_standardize_checks` caught while writing this fix, where the label column got
  silently treated as a third feature until `infer()` gained the same `target` parameter `assess()`
  already had. `save()`/`load()` persist the fitted stats through a small sidecar artifact
  (`<path>.standardize`, written only when `standardize = true` was used) rather than forcing them
  through `ice::io::save_parameters`'s parameter-list format, whose size check requires the
  destination Series to already be correctly shaped before loading — not knowable here until the
  file itself is read. Covered by `ice_model_test.cpp` and demonstrated end-to-end (including the
  save/load round trip) in the full-capability demo's section 3.
- Nothing else rose to the level of a real gap — the rest of the surface (three `learn()` overloads,
  chunked `infer()`, `save()`/`load()`, `make_optimizer()`, `resolve_platform()`, `NoGradGuard`, and
  the full `TemporalFusionTransformer` walkthrough) worked exactly as documented on the first real
  run once dtype mismatches in the demo's own synthetic-data setup (host `double` literals feeding
  a `Float32`-configured `Series`) were fixed — not an API gap, a demo bug.

## 9. Open Questions

1. **Naming.** `ice::Outline` (the `nn::Schedule`/`nn.Sequential`-equivalent composition type,
   §4/§6) is decided. Still open: `Model`/`TrainOptions`/`learn`/`infer` are working names for
   everything else, deliberately avoiding `LightningModule`/`Trainer.fit`/`DataModule` — in
   particular whether ice mirrors `WorkflowModel` closely enough to reuse that exact type shape
   instead of inventing a parallel `ice::Model`.
2. **Where does `ice::io`'s generic state-dict format live relative to NodeB's `io::` module?**
   They solve the same *kind* of problem (versioned, checksummed artifact persistence) for
   different payloads (NodeK model parameters vs. NodeB stage state). Worth deciding whether
   `ice::io` reuses NodeB's `io::detail` checksum helpers or is fully independent, given ice must
   not depend on NodeB (NodeB will depend on ice, not the reverse).
3. **Batch/data bridging scope.** How much of `ice::data` is generic (any NodeK training loop) vs.
   text-specific (bridging NodeB `Doc`s in)? Keeping ice's core generic and putting the NodeB
   bridge in a thin adapter (rather than baking `Doc` into `ice::core`) keeps ice usable for
   non-text NodeK models too.

## 10. Quality Gates

1. Every NodeB call site migrated onto ice must keep its existing test green, unchanged, as the
   regression gate (§7.2) — this is a stricter bar than "new tests pass," since it proves the
   abstraction didn't change behavior, only removed duplication.
2. Numerical safety (gradient clipping + non-finite diagnostics) verified on by default, not
   opt-in, with a test that deliberately induces instability and confirms a clean, loud failure.
3. Deterministic training reproducibility given a fixed seed, matching NodeB's own principle.
4. No `ice::models::` entry ships without at least one test proving it learned the *target*
   capability (e.g. a generation model's test must show it actually completes a learned pattern,
   not just that `.generate()` returns a string) — the same bar applied to every NodeB Sequence
   backend rewrite this session.

## 11. Risks and Mitigations

1. **Over-scoping into "the LLM stack" before the trainer foundation is solid.**
   Mitigation: Phase 1 targets the simplest existing, already-correct code, not new capability.
2. **Migrating existing NodeB call sites regresses working behavior.**
   Mitigation: each call site already has a passing test from this session; that test is the
   acceptance criterion for its migration, unchanged.
3. **Ice duplicates NodeB's `io::` versioning/checksum work instead of reusing what's proven.**
   Mitigation: explicit open question (§9.2) to resolve before Phase 3, not an assumption either way.
