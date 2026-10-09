# PlussAI `nlp`

`pluss::ai::nlp` is PlussAI's document-processing and language-modeling
family. It turns raw text into a `Doc` whose token, sentence, lemma, entity,
phrase, match, syntax, and document-label layers can be inspected together.
The same family also supplies classical vectorizers, trainable static and
contextual embeddings, lower-level sequence components, language resources,
custom-stage registration, and workflow persistence.

The central design is pipeline-first. A [`Workflow`](nlp/workflow/Workflow.md)
owns an ordered list of stages. Each stage receives the same `Doc`, reads the
layers it depends on, and adds or updates one layer for later stages. This
makes ordering part of the contract: a lemmatizer needs tokens, a matcher that
uses lemmas needs both tokenization and lemmatization, and syntax analysis is
more useful after part-of-speech tags exist.

## Include And Namespace

```cpp
#include <pluss/nlp.h>

namespace nlp = pluss::ai::nlp;
```

Use `pluss::ai::nlp` in public code. The implementation layout is deliberately
not part of the public contract.

## Choosing The Right Surface

| Goal | Start here | Result |
| --- | --- | --- |
| Tokenize, segment sentences, lemmatize, tag, match, recognize entities, parse syntax, or classify text | [`Workflow`](nlp/workflow/Workflow.md) plus [`stage`](nlp/tasks.md) factories | One `Doc` per input text |
| Inspect annotations already produced | [`Doc`](nlp/core/Doc.md), [`Token`](nlp/core/Token.md), and [`Span`](nlp/core/Span.md) | Host records and extracted text |
| Build count, TF-IDF, or hashing features | [`vectorize::Mapper`](nlp/vectorize/Mapper.md) or [`vectorize::Hasher`](nlp/vectorize/Hasher.md) | Dense or sparse classical feature batches |
| Learn one context-independent vector per token | [`embed::Embedder`](nlp/embed/Embedder.md) | Static token and mean-pooled document vectors |
| Learn context-sensitive token vectors or continue a prompt | [`embed::ContextualEmbedder`](nlp/embed/ContextualEmbedder.md) | Per-token contextual vectors, document vectors, or generated tokens |
| Assemble a custom sequence model | [`runtime`](nlp/runtime.md) | Indexed, padded batches and neural encoders |
| Add a custom workflow stage | [`IStage`](nlp/core/IStage.md) and [`StageRegistry`](nlp/registry/StageRegistry.md) | A stage that can participate in execution and persistence |
| Inspect built-in language data | [`resources`](nlp/resources.md) | Language packs, abbreviations, titles, lemma rules, and token exceptions |
| Persist to a path | `Workflow::save()` / `Workflow::load()` | A versioned, checksummed workflow artifact |
| Persist to an existing stream | [`io::write_workflow`](nlp/io/write_workflow.md) / [`io::read_workflow`](nlp/io/read_workflow.md) | The same workflow schema under caller-controlled I/O |

`Workflow::assess()` is a structural health report: it counts documents,
tokens, and sentence spans. It is not a task-accuracy metric. Use
`assess::tagger()` or `assess::ner()` when predicted annotations must be
compared with gold annotations.

## The Document Lifecycle

A `Workflow` does not keep a mutable current document. Every `infer()` call
constructs a new `Doc`, copies the input text into it, records the workflow's
language, then invokes each stage in order. Batch overloads repeat that process
and return a `std::vector<Doc>` in input order. The scalar and batch paths
therefore have the same stage semantics; the difference is only result shape.

Raw-text overloads accept one `std::string_view`, a borrowed
`pluss::RangeView<std::string_view>`, an owning `std::vector<std::string>`, or a
text `pluss::Note`. A `Note` is flattened in logical element order, so a
higher-rank text Note still produces one flat document vector. Use the
`std::vector<Doc>` overloads when tokenization or gold annotations have already
been prepared and must not be reconstructed from raw text.

Annotation ownership remains with each returned `Doc`:

- `tokens` stores per-occurrence offsets, surface text, lemma, morphology,
  part-of-speech tag, dependency role, and entity label;
- `sentences`, `phrases`, `matches`, and `entities` are token ranges represented
  by `Span`;
- `doc_label`, `doc_score`, and `doc_labels` carry document-classification
  results;
- the optional lexicon interns repeated text and morphology records within the
  document without changing token order.

`Span::text(doc)` recovers the original character slice, including interior
spacing and punctuation. The convenience methods `sentence_texts()`,
`entity_texts()`, and their siblings apply that extraction to an entire span
layer.

## Stage Ordering And Composition

The built-in stages have narrow responsibilities:

1. `stage::tokenize()` establishes token boundaries and lexical observations.
2. `stage::sentencize()` marks sentence boundaries and fills sentence spans.
3. `stage::lemmatize()` fills canonical token forms.
4. `stage::tagger()` fills part-of-speech tags.
5. `stage::matcher()` adds deterministic pattern spans.
6. `stage::ner()` adds named-entity spans and token entity labels.
7. `stage::syntax()` fills dependency heads and roles, then derives phrases.
8. `stage::textcat()` predicts document-level labels and scores.

The workflow intentionally does not infer missing prerequisites. For example,
putting a lemmatizer before tokenization gives it no tokens to transform. This
keeps execution predictable and allows custom stages to use the same simple
contract. Establish data-producing stages first, then annotation consumers,
then document-level heads.

Rule-driven and learned stages can coexist. Tokenization, sentence boundaries,
lemmatization, matching, and the default syntax baseline are deterministic
transformations. Tagging, entity recognition, and text classification can use
classical or sequence backends according to their option objects. A workflow
may therefore begin with deterministic structure, add learned token-level
annotations, and finish with a learned document label.

## Learning, Inference, And Assessment

`learn()` mutates trainable stages in place and returns the same `Workflow&`,
which permits configuration and training to remain in one expression. Every
stage receives the same training corpus; stages that have no learning phase
use the default no-op implementation. Unlabeled overloads suit embedding-like
or annotation-driven learning. Labeled overloads are required by supervised
document classification.

`TrainOptions` controls corpus validation and reproducibility:

| Field | Meaning | Default |
| --- | --- | --- |
| `random.seed` | Seed forwarded to trainable stages | `0` |
| `random.deterministic` | Request repeatable stage behavior | `true` |
| `allow_empty_docs` | Permit a training item whose text is empty | `true` |

Training always rejects an empty corpus. Labeled training also requires one
label per document. Setting `allow_empty_docs=false` moves accidental empty
records from model behavior into an immediate contract error.

`infer()` never retrains. It applies the stage state as it exists at call time
and returns fresh documents. `assess()` either runs inference over raw text or
summarizes already-produced documents. Its option contract is deliberately
small:

| Field | Meaning | Default |
| --- | --- | --- |
| `require_sentences` | Reject a non-empty document when no sentence spans were produced | `false` |

Turn this on when sentence segmentation is a required pipeline invariant. Do
not turn it on for a deliberately token-only workflow.

## Worked Example: One Workflow, Three Input Paths

This example exercises scalar inference, a `Note` batch, assessment over
already-produced documents, lemma-based matching, and a persistence round
trip. A permanent reference executable produces the displayed output, which is
copied verbatim.

```cpp
#include <pluss/nlp.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow pipeline{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::sentencize(),
          nlp::stage::lemmatize(),
          nlp::stage::matcher({
              nlp::MatcherPattern{
                  .label = "TOPIC",
                  .tokens = {"write", "api"},
                  .use_lemmas = true,
              },
          }),
      },
  };

  const auto doc = pipeline.infer("Ama writes APIs. Kojo tests them!");
  std::cout << "scalar\n";
  std::cout << "tokens=" << doc.tokens.size()
            << " sentences=" << doc.sentences.size()
            << " matches=" << doc.matches.size() << '\n';
  std::cout << "lemmas=";
  const auto lemmas = doc.lemma_texts();
  for (std::size_t i = 0; i < lemmas.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << lemmas[i];
  }
  std::cout << "\nmatch=" << doc.matches.front().label << ':'
            << doc.matches.front().text(doc) << '\n';

  const auto batch = pipeline.infer(
      pluss::make_note({"One sentence.", "Two sentences. Really!"}));
  const auto report = pipeline.assess(batch,
      nlp::AssessOptions{.require_sentences = true});
  std::cout << "batch\n";
  std::cout << "docs=" << report.doc_count
            << " tokens=" << report.token_count
            << " sentences=" << report.sentence_count
            << " mean_tokens=" << report.mean_tokens_per_doc << '\n';

  const auto path = std::filesystem::temp_directory_path() /
                    "pluss_nlp_docs_reference.workflow";
  pipeline.save(path.string());
  const auto restored = nlp::Workflow::load(path.string());
  std::filesystem::remove(path);
  const auto roundtrip = restored.infer("Ama writes APIs.");
  std::cout << "roundtrip\n";
  std::cout << "language=" << nlp::resources::language_code(restored.lang)
            << " stages=" << restored.stages.size()
            << " tokens=" << roundtrip.tokens.size()
            << " matches=" << roundtrip.matches.size() << '\n';
}
```

Output:

```text
scalar
tokens=8 sentences=2 matches=1
lemmas=ama,write,api,.,kojo,test,them,!
match=TOPIC:writes APIs
batch
docs=2 tokens=8 sentences=3 mean_tokens=4
roundtrip
language=en stages=4 tokens=4 matches=1
```

The match uses lemmas, so the pattern `write api` accepts the inflected surface
text `writes APIs`. The extracted span still returns the original text rather
than the normalized pattern.

## Worked Example: Supervised Document Classification

The labeled `learn()` overload is the important distinction here. Text and
label counts must match, and the classifier stage must appear after every stage
that produces features it is configured to consume.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow classifier{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::lemmatize(),
          nlp::stage::textcat(),
      },
  };

  classifier.learn(
      std::vector<std::string>{
          "Compiler errors break builds",
          "Headers need cleanup",
          "Fresh bread and soup",
          "Bake a sweet cake",
      },
      std::vector<std::string>{"tech", "tech", "food", "food"},
      nlp::TrainOptions{
          .random = {.seed = 17, .deterministic = true},
          .allow_empty_docs = false,
      });

  for (const auto& doc : classifier.infer(std::vector<std::string>{
           "Compiler warnings in headers",
           "Bread and soup for dinner",
       })) {
    std::cout << doc.doc_label << '\n';
  }
}
```

Output

```text
tech
food
```

Use a sequence backend when the classification contract requires a learned
sequence representation, multi-label output, or an explicitly selected
sequence head. The default classical backend is usually the smaller and more
direct choice for modest labeled corpora.

## Worked Example: Features Without A Trainable Workflow Head

Vectorization is separate from `Workflow` because its result is a numerical
feature batch rather than another annotation layer on `Doc`. Prepare documents
once, learn a stable vocabulary from the training corpus, and reuse the mapper
for every later batch.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow prep{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()},
  };
  const auto docs = prep.infer(std::vector<std::string>{
      "New York builds APIs.",
      "Accra builds systems.",
      "Tema builds data platforms.",
  });

  nlp::vectorize::Mapper mapper(
      pluss::ai::classic::LexiconMapOptions{
          .lowercase = true,
          .max_features = 32,
          .min_document_frequency = 1,
          .binary = false,
          .min_ngram = 1,
          .max_ngram = 2,
      },
      nlp::vectorize::DocMapOptions{
          .use_lemmas = true,
          .skip_punct = true,
      });

  if (const auto status = mapper.learn(docs); !status.ok()) {
    std::cerr << status.message << '\n';
    return 1;
  }
  const auto features = mapper.tfidf_sparse(docs);
  if (!features.ok()) {
    std::cerr << "feature mapping failed\n";
    return 1;
  }

  std::cout << "documents=" << docs.size() << '\n';
  std::cout << "features=" << mapper.vocabulary().size() << '\n';
  std::cout << "nonzero=" << features.value().values.numel() << '\n';
}
```

Output

```text
documents=3
features=17
nonzero=19
```

`Mapper` owns a learned vocabulary and document-frequency state. `Hasher`
does not learn a vocabulary and can transform immediately, trading named,
inspectable features for a fixed-width hash space. Do not fit a new mapper on
each inference batch: that changes column meaning and invalidates any model
trained on the earlier mapping.

## Static And Contextual Embeddings

The two embedding classes solve different representation problems.
`embed::Embedder` learns one vector per vocabulary item with an unsupervised
context objective. `vector_of()` retrieves that stable token vector,
`nearest()` searches the learned vocabulary, and `embed(doc)` mean-pools the
in-vocabulary token vectors.

`embed::ContextualEmbedder` learns a causal sequence encoder. `encode(doc)`
returns one vector per surviving token, so the same token may have different
vectors in different contexts. `embed(doc)` pools those contextual rows, while
`generate(prompt, options)` repeatedly predicts and appends new tokens. Its
sampling path is controlled by `greedy`, `temperature`, `top_k`, and `seed`;
greedy decoding ignores the sampling-only fields.

Both models require tokenized documents for `learn()`. If `use_lemmas=true`,
include lemmatization in the preparation workflow. If `skip_punct=true`,
punctuation contributes neither training examples nor output rows. Querying a
model before successful training returns a failed status rather than a
plausible empty vector.

## Persistence And Extension Boundaries

`Workflow::save()` writes the current schema with language, stage count,
per-stage state, and an integrity checksum. `Workflow::load()` accepts the
supported historical schema range and restores stages through the registry.
Truncated data, checksum mismatch, unsupported schema versions, unknown stage
names, null stages, and unreadable paths fail explicitly.

Custom stages implement `IStage`, return a stable name, and provide `apply()`.
Override the relevant `learn()` overload only when the stage has trainable
state. Override both state-line methods when that state or configuration must
survive persistence, then register a zero-argument factory under the same
stable name before loading. A custom stage that neither emits nor accepts
state can rely on the empty default dump, but a non-empty state payload is
rejected by the default loader.

## Performance And Ownership

- Batch overloads reserve their output and preserve input order, but stage
  execution remains document-oriented; use a batch call for API clarity, not
  as a promise that every custom stage is internally vectorized.
- `Doc` owns its text and annotation vectors. Returned spans are offsets into
  that document and must be interpreted against the same `Doc`.
- `TextPool` and `MorphBank` deduplicate repeated lexical data, reducing
  duplicate string and morphology storage without changing token records.
- Reuse trained workflows, mappers, hashers, and embedders. Reconstructing or
  retraining them per request discards learned state and dominates inference
  cost.
- The runtime family exposes lower-level sequence batching and encoders when
  a custom model needs them. Prefer workflow stages for ordinary annotation
  tasks because they keep preprocessing, state, and persistence together.

## Reference Families

- [`core`](nlp/core.md): `Doc`, annotation records, lexical storage, and the
  custom-stage interface.
- [`workflow`](nlp/workflow.md): pipeline lifecycle, overloads, reports, and
  persistence entry points.
- [`tasks`](nlp/tasks.md): built-in deterministic and trainable stages.
- [`vectorize`](nlp/vectorize.md): vocabulary and hashing feature maps.
- [`embed`](nlp/embed.md): static and contextual representation learning.
- [`runtime`](nlp/runtime.md): indexing, sequence batches, embedding tables,
  pooling, and text encoders.
- [`resources`](nlp/resources.md): built-in multilingual language assets.
- [`registry`](nlp/registry.md): thread-safe stage factory registration.
- [`io`](nlp/io.md): stream-level workflow serialization.

```{toctree}
:maxdepth: 3
:hidden:

nlp/core
nlp/workflow
nlp/tasks
nlp/vectorize
nlp/embed
nlp/runtime
nlp/resources
nlp/registry
nlp/io
```
