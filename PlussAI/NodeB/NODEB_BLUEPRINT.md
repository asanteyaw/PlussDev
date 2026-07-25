# NodeB Blueprint (spaCy-like NLP Space)

## 1. Positioning

NodeB is the NLP product space in `PlussAI`, designed for **pipeline-first NLP** in the spirit of spaCy, with pluss-native naming and contracts.

- `NodeJ`: classical ML engine
- `NodeK`: deep learning engine
- `NodeB`: NLP orchestration/runtime + language components + task heads

NodeB stays opt-in via `<pluss/nlp.h>`.

## 2. Design Principles

1. Pipeline-first user surface
2. Consistent verbs with current AI stack: `learn`, `infer`, `assess`, `save`, `load`
3. Strong typing (enums/struct options, no stringly core config)
4. Production readiness: deterministic seeds, artifact versioning, robust diagnostics
5. Doctrine compliance: concise API, maintainable modules, correctness-first

## 3. Target User Surface (Draft)

```cpp
#include <pluss/nlp.h>

using namespace pluss::ai::nodeb;

Workflow nlp {
  .lang = Language::English,
  .stages = {
    stage::tokenize(),
    stage::sentencize(),
    stage::lemmatize(),
    stage::tagger(),
    stage::ner(),
    stage::textcat(),
  },
};

nlp.learn(train_docs, TrainOptions{.random = {.seed = 42, .deterministic = true}});
auto pred = nlp.infer(test_docs);
auto report = nlp.assess(test_docs, gold_labels);
nlp.save("models/en_core_nodeb.bin");
```

## 4. Planned Folder/Module Layout

```text
PlussAI/NodeB/
  core/            # Doc, Span, Token, Lexicon, TextPool, hashes, registry base
  tokenize/        # Rule tokenizer, regex tokenizer, language exceptions
  linguistics/     # Morphology, POS schema, lemmatization, sentence boundaries
  vocab/           # Lexeme store, vectors table, term frequencies
  vectorize/       # BoW, ngram, hashing, tf-idf, static embeddings
  pipeline/        # Workflow runtime, stage graph, execution state
  tasks/
    tagger/        # POS/morph tagger
    syntax/        # dependency-style syntax linker
    ner/           # named-entity recognizer
    textcat/       # single/multi-label text classification
    matcher/       # rule matcher, phrase matcher
    lemmatizer/    # rule/lookup lemmatizer
    sentencizer/   # sentence boundary detector
  train/           # trainers, batching, curriculum, callbacks
  eval/            # scoring, confusion matrices, entity/span metrics
  io/              # serialization, model packs, version metadata
  runtime/         # inference runtime, memory arenas, caches
  registry/        # components/factories/plugin registration
  resources/       # language assets, lookup tables, default configs
  bindings/        # optional python/json adapters later
  docs/            # NodeB docs sources
  test/            # unit/integration/parity tests
```

## 5. Include Surface Plan

- Public umbrella: `pluss/nlp.h`
- AI namespace umbrella: `pluss/ai/nlp.h`
- Core namespace anchor: `pluss/ai/nodeb/nodeb.h`
- Stage-specific headers (future):
  - `pluss/ai/nodeb/workflow.h`
  - `pluss/ai/nodeb/tokenize.h`
  - `pluss/ai/nodeb/tasks.h`
  - `pluss/ai/nodeb/io.h`
  - `pluss/ai/nodeb/options.h`

## 6. spaCy Parity Matrix (Execution-Oriented)

## 6.1 Core objects

- spaCy `Doc/Token/Span/Vocab` inspired the need for shared lexical state, but NodeB exposes Pluss-native `Doc/Token/Span/Lexicon`
- spaCy string interning informed NodeB's `TextPool` design in `core/`

## 6.2 Pipeline runtime

- spaCy `nlp.add_pipe(...)` -> NodeB `Workflow{ .stages = {...} }` and `append_stage(...)`
- spaCy `nlp(text)` -> NodeB `infer(text|docs)`

## 6.3 Components

- tokenizer, tagger, syntax, ner, textcat, sentencizer, lemmatizer, matcher
- first tranche prioritizes: tokenizer + sentencizer + lemmatizer + textcat

## 6.4 Training/evaluation

- spaCy train loop parity with NodeB `learn`/`assess`
- NodeJ/NodeK backends selectable by options (classical vs deep)

## 6.5 Persistence

- spaCy model package concepts -> NodeB binary pack + metadata schema version

## 7. Integration Strategy

1. Delegate heavily to CodeN primitives for text-native operations (token buffers, string/lexeme transforms, compact text storage and slicing).
2. NodeB text vectorizers emit `Series` directly for NodeJ models.
3. NodeB deep components consume/emit NodeK tensors.
4. DataFrame bridge via PlussMic (text column -> NodeB docs).
5. OutputEngine-compatible summaries for diagnostics.

## 8. Implementation Roadmap

## Phase 0 (Scaffold)

- Create folder + public include scaffolding
- Define core interfaces (`IStage`, `ITrainer`, `IArtifact`)
- Define workflow lifecycle (`learn`, `infer`, `assess`, `save`, `load`)

## Phase 1 (High ROI MVP)

- Tokenizer + sentencizer + lemmatizer (rule/lookup)
- Text classification baseline via NodeJ (`tf-idf + linear`)
- End-to-end examples and model persistence

## Phase 2 (Production NLP essentials)

- NER and POS tagger baseline
- Rule matcher + phrase matcher
- Cross-validation/tuning utilities for textcat

## Phase 3 (Deep NLP path)

- NodeK-backed embeddings + sequence encoders
- Transformer-style text heads
- Distillation/quantization-ready inference hooks

## Phase 4 (Ecosystem hardening)

- Plugin registry for custom stages
- Multilingual assets and locale packs
- Rigorous parity/perf tests and docs completion

## 8.1 Near-Parity Ten-Tranche Plan

### Phase 1/10 (Core lexical substrate)

- `Lexicon`, deterministic `TextPool`, and `Lexeme` ownership in `core/`
- token morphology/shape cached once at tokenization time
- lower-level `CodeN` token-analysis helpers so NodeB delegates downward

### Phase 2/10 (Morphology and linguistic attributes)

- explicit morphology tables and normalized feature schema
- richer lemma/POS/normalized-surface attribute surfaces on `Token` and `Lexeme`
- document-level attribute inspection and diagnostics

### Phase 3/10 (Syntax baseline)

- dependency-style syntax baseline
- dependency labels, heads, children, subtree helpers
- syntax-aware sentence and noun-chunk behaviors

### Phase 4/10 (Trainable POS and NER)

- move tagger/ner from heuristics to trainable stages
- proper span/entity metrics and artifact persistence
- backend contracts that can target NodeJ first, then NodeK
  - current NodeJ-first baseline: compact learned lexical/phrase memory with save/load

### Phase 5/10 (Vectorizers and text statistics)

- ngrams, hashing, tf-idf, vocabulary stats, document vectors
- direct `Series`/NodeJ bridge for classical NLP features
- batch-oriented text feature extraction

### Phase 6/10 (Embeddings and sequence runtime)

- NodeK sequence batching, masks, embeddings, and encoder contracts
- reusable sequence runtime for text heads
- deterministic inference/training paths

### Phase 7/10 (Deep task heads)

- transformer/BiLSTM-style heads for textcat, tagger, ner
- distillation/quantization-aware interfaces
- model-selection hooks across NodeJ and NodeK

### Phase 8/10 (Multilingual assets)

- language packs, tokenizer exceptions, morphology resources
- locale-aware sentencizer/tokenizer behavior
- workflow-level language/model selection

### Phase 9/10 (Extensibility and packaging)

- stage registry/plugin system
- model packaging, metadata schema, compatibility policy
- deployment-oriented artifact boundaries

### Phase 10/10 (Parity hardening)

- rigorous parity suites against reference NLP behaviors
- performance harnesses and regression tests
- production docs, cookbook, migration map, and full examples

## 9. Quality Gates

1. Correctness parity tests against spaCy reference behaviors
2. Deterministic training reproducibility
3. Catch2 edge/stress/numerical tests for all core components
4. Save/load parity (`infer` before vs after restore)
5. End-to-end demos with minimal user code

## 10. Documentation Deliverables

- `docs/sphinx/nodeb_user_guide.md`
- `docs/sphinx/nodeb_cookbook.md`
- `docs/sphinx/nodeb_api_reference.md`
- migration map: spaCy terms -> NodeB terms

## 11. Risks and Mitigations

1. Scope blowup from full spaCy parity
   - Mitigation: strict phased delivery + parity matrix per tranche
2. Backend fragmentation between NodeJ and NodeK
   - Mitigation: stable NodeB stage contracts, backend hidden behind options
3. Asset/version drift
   - Mitigation: explicit versioned metadata + compatibility policy

## 12. Immediate Next Buildable Tranche

1. Define `core::Doc/Token/Span` minimal structs
2. Add `pipeline::Workflow` skeleton with stage execution order
3. Add `tokenize::RuleTokenizer` and `tasks::sentencizer`
4. Add one end-to-end demo (`examples/NodeB/nodeb_textcat_mvp.cpp`)
5. Add Catch2 tests for tokenizer and pipeline persistence
