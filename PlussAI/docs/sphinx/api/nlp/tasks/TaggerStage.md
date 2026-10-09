# `pluss::ai::nlp::TaggerStage`

`TaggerStage` assigns a part-of-speech label to every token and writes it to
`Token::pos`. The tag set is open: it is whatever strings the rules or the
training data use. The conventional tags are `NOUN`, `PROPN`, `VERB`, `AUX`,
`ADJ`, `ADV`, `ADP`, `DET`, `PRON`, `CCONJ`, `SCONJ`, `PART`, `NUM`, `PUNCT`,
and `X`, and the heuristic rules below produce exactly that set.

A token can receive its tag from three sources. The learned tables give the
tag most often seen for a word, its suffix, or its word shape in training. The
heuristic rules are fixed, need no training, and always produce a tag. The
sequence model is a trainable classifier that looks at each token in its
context. The options choose which sources run and in what order, and they are
the main thing to understand before using this stage.

The stage tags tokens, so it runs after a tokenizer. Its most useful input is a
lemmatizer's output, because the lemma is what the rules and tables compare by
default. The [`SyntaxStage`](SyntaxStage.md) reads these tags, so it runs after
this stage.

## Signature

```cpp
pluss::ai::nlp::TaggerStage::TaggerStage(
    pluss::ai::nlp::TaggerOptions options = {});
void pluss::ai::nlp::TaggerStage::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    const pluss::ai::nlp::TrainOptions& options);
pluss::ai::nlp::Stage pluss::ai::nlp::stage::tagger(
    pluss::ai::nlp::TaggerOptions options = {});
```

## Construction

Use `stage::tagger` to place the stage in a workflow. Construct the concrete
class when you need to call `learn` on it, because `Workflow::learn` reaches
the stage through the same method. Keep the typed handle for training and wrap
it in a `Stage` for the workflow.

A new stage is untrained. It applies only its heuristics, or nothing at all if
heuristics are off, until `learn` has been called on training documents.

## Options

The options struct is `pluss::ai::nlp::TaggerOptions`. Its defaults give
heuristic tagging with the learned tables enabled, the classic backend, and
lemmas used when they are available.

### `lowercase`

Controls whether the word-form tables and the heuristics compare lowercase
forms. When `true`, `Ada`, `ada`, and `ADA` share one learned form entry and one
heuristic base. When `false`, the case of the form is part of the key, so a
word learned as `Ada` does not apply to `ada`.

Keep it on for most text, because tagging depends more on what a word is than
on how it was capitalized at the start of a sentence. The capitalization still
matters to the heuristics through the proper-noun rule, which reads the token's
own case flags, not the lowercased base.

### `use_lemmas`

Controls whether the lemma stands in for the surface form in every comparison.
When `true` and the token has a non-empty lemma, the learned form key and the
heuristic base both come from the lemma. A learned tag for `write` then applies
to `wrote`, `writes`, and `write`, and the heuristic verb list sees `write`
instead of `writes`.

When `false`, or when a token has no lemma, the surface form is used. Turn it
off when the lemmatizer is not in the pipeline, or when the form itself carries
the distinction the tags need.

Running without a lemmatizer is the most common way to get a surprising tag,
and the Practical Examples show one. The heuristic rules treat a word ending in
`s` as a verb form, so an uninflected plural such as `APIs` is tagged `VERB`
unless a lemmatizer has first removed the `s`.

### `prefer_learned`

Controls whether the learned tables are consulted before anything else. When
`true`, a token whose learned form, suffix, or word shape appears in the
training tables takes the tag from the table, and the later sources are skipped
for that token. The three tables are checked in that order, and the first hit
wins.

When `false`, the learned tables are ignored entirely. The token then gets its
tag from the sequence model if that backend is active and has a prediction,
and from the heuristics otherwise. Turn it off to see what the rules alone
would say, or to let a rule override a table that was trained on a small
corpus.

### `use_heuristics`

Controls whether the built-in rules run. They run last, so they tag only the
tokens that no earlier source handled. The rules are described under Methods,
where `apply` explains their order.

When `false`, a token that no earlier source covered is left with an empty
tag. Turn this off when you want a trained-only tagger, so that every tag in the
output came from training. With the Sequence backend, the classifier covers
every token once it is trained, so turning heuristics off then makes the
output depend on training alone.

### `backend`

Selects the trainable component. `TaskBackend::Classic` uses the three learned
tables and nothing else. `TaskBackend::Sequence` also trains a contextual
classifier, which scores each token with its neighbors in view and produces a
tag even for words it has never seen.

With the Sequence backend, the classifier's prediction outranks the heuristics.
Selecting the backend therefore takes effect whether or not heuristics are on.
The learned tables still come first when `prefer_learned` is on. The
classifier is trained only by `learn` and applies only after training.

### `sequence_width`

`std::int64_t`, default `48`. The width of the hidden representation in the
Sequence backend's classifier. A larger width has more capacity to separate
tags, and it costs more memory and training time. The value has no effect on
the Classic backend, and it must be positive when the Sequence backend trains.

### `sequence_seed`

`std::uint64_t`, default `0`. The seed for the classifier's initial weights.
With the same seed, the same training documents, and the same width, training
produces the same predictions, which is what makes a trained tagger
reproducible. Change it to test whether a result depends on the initialization.

## State

A trained stage keeps three learned tables and, with the Sequence backend, the
classifier's weights. An untrained stage has none of these.

`dump_state_lines()` writes the options, a `trained` flag, and the tables, one
entry per learned key. The lines for the classifier follow the tables. The
output is complete, so `load_state_lines()` on a fresh stage restores a tagger
that gives the same tags as the original. Lines that belong to options this
stage no longer has, such as quantization hints from older saves, are ignored
on load.

## Methods

### `name()`

Returns `"tagger"`.

### `learn(docs, options)`

Trains on the tags already present on the documents' tokens. Every token with a
non-empty `pos` contributes one observation. Tokens with an empty tag are
skipped, so a partly tagged corpus trains on its tagged part only.

The learned tables count, for each key, how often each tag appears, and keep
the most frequent one. A tie goes to the lexicographically smaller tag, which
makes training deterministic. The three tables are keyed by the form, the
suffix, and the word shape.

With the Sequence backend, `learn` also trains the classifier. That training
needs at least two distinct tags in the corpus. A corpus with one tag throws,
because a classifier with one output class has nothing to decide. The
`TrainOptions` argument is accepted for interface consistency. The tagger's
randomness is set by `sequence_seed`, not by `TrainOptions::random`.

Calling `learn` replaces the previous training. It does not merge the new
corpus into the old tables. With the Classic backend, an empty corpus clears the
trained state.

### `apply(Doc&)`

Sets `pos` on every token. For each token, the stage tries these sources in
order, and stops at the first that produces a tag:

1. When `prefer_learned` is on and the stage is trained, the form table, then
   the suffix table, then the word-shape table.
2. When the Sequence backend is trained, its prediction for the token.
3. When `use_heuristics` is on, the rules.

The rules check the token in a fixed order, and the order decides ties: punctuation, then URLs and email
addresses (`X`), then numbers (`NUM`), then the determiner list (`DET`), the
pronoun list (`PRON`), the auxiliary list (`AUX`), the coordinating and
subordinating conjunction lists (`CCONJ`, `SCONJ`), `to` (`PART`), the
preposition list (`ADP`), the adverb list and `-ly` words (`ADV`), the
adjective suffixes (`ADJ`), the verb list and verb suffixes (`VERB`), a
capitalized or uppercase word (`PROPN`), and finally everything else (`NOUN`).
The first rule that matches decides the tag. Because the verb suffixes are checked
before the proper-noun rule, a capitalized word ending in `-ed`, such as `Zed`,
is tagged `VERB`. The Practical Examples show this case.

### `dump_state_lines()`

Returns the options, the trained flag, the tables, and the classifier state,
in that order.

### `load_state_lines(lines)`

Replaces the options and all learned state from a saved set of lines. It is
the inverse of `dump_state_lines`. Options are reset to their defaults first,
so a line that is absent from the input leaves its option at the default.

### `training_report()`

Returns the [`TrainingReport`](TrainingReport.md) of the most recent `learn` call: the
number of training documents, the number of learned form, suffix, and word-shape
entries, and, for the Sequence backend, the loss after each optimizer step. An
untrained stage returns an empty record.

## Practical Examples

These examples show the heuristic rules on their own, the priority of the
learned tables and the heuristics, a saved tagger restored into a new stage,
and the Sequence backend with its two-label requirement.

### Example 1

The heuristic rules on a sentence with several word classes, with no training
at all. The lemmatizer runs first, so the rules see base forms. The output
prints each token with its tag and shows what each class looks like.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::tagger()}};
  const nlp::Doc doc = wf.infer("Ada writes the APIs quickly, and Kofi reads 42 books in Tema.");

  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.orth << " / " << token.pos << "\n";
  }
  return 0;
}
```

Output

```text
Ada / PROPN
writes / VERB
the / DET
APIs / NOUN
quickly / ADV
, / PUNCT
and / CCONJ
Kofi / PROPN
reads / VERB
42 / NUM
books / NOUN
in / ADP
Tema / PROPN
. / PUNCT
```

### Example 2

The priority of the sources. The same input is tagged under four settings of
`prefer_learned` and `use_heuristics`, with a trained classic tagger. The learned tables tag the unseen word `Zed` as `PROPN` from the shape of
training words. With the tables off and the rules on, the rules tag `Zed` as
`VERB`, because its `-ed` ending matches the verb suffixes before the
proper-noun rule is reached. With both sources off, nothing tags the tokens.
`writes` is `VERB` whenever any source is active, because training saw it.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  std::vector<nlp::Doc> train;
  for (const char* text : {"Ada writes books.", "Kofi reads notes."}) {
    nlp::Doc doc = base.infer(text);
    const char* tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
    for (std::size_t i = 0; i < doc.tokens.size(); ++i) doc.tokens[i].pos = tags[i];
    train.push_back(doc);
  }

  for (const bool learned : {true, false}) {
    for (const bool heuristics : {true, false}) {
      auto tagger = std::make_shared<nlp::TaggerStage>(
          nlp::TaggerOptions{.prefer_learned = learned, .use_heuristics = heuristics});
      tagger->learn(train, {});
  const nlp::TrainingReport report = tagger->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";
      nlp::Workflow wf{.lang = nlp::Language::English,
                       .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), tagger}};
      const nlp::Doc doc = wf.infer("Zed writes tunes.");

      std::cout << "prefer_learned = " << (learned ? "true" : "false")
                << ", use_heuristics = " << (heuristics ? "true" : "false") << "\n";
      for (const nlp::Token& token : doc.tokens) {
        std::cout << "  " << token.orth << " / " << (token.pos.empty() ? "(empty)" : token.pos) << "\n";
      }
    }
  }
  return 0;
}
```

Output

```text
docs = 2, entries = 16, steps = 0
prefer_learned = true, use_heuristics = true
  Zed / PROPN
  writes / VERB
  tunes / NOUN
  . / PUNCT
docs = 2, entries = 16, steps = 0
prefer_learned = true, use_heuristics = false
  Zed / PROPN
  writes / VERB
  tunes / NOUN
  . / PUNCT
docs = 2, entries = 16, steps = 0
prefer_learned = false, use_heuristics = true
  Zed / VERB
  writes / VERB
  tunes / NOUN
  . / PUNCT
docs = 2, entries = 16, steps = 0
prefer_learned = false, use_heuristics = false
  Zed / (empty)
  writes / (empty)
  tunes / (empty)
  . / (empty)
```

### Example 3

A trained tagger saved with `dump_state_lines` and restored into a fresh stage.
The original and the restored stage tag the same input the same way, so a tagger
can be trained once and reused.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::Doc doc = base.infer("Ada writes books.");
  const char* tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
  for (std::size_t i = 0; i < doc.tokens.size(); ++i) doc.tokens[i].pos = tags[i];

  auto trained = std::make_shared<nlp::TaggerStage>(nlp::TaggerOptions{.use_heuristics = false});
  trained->learn({doc}, {});
  const nlp::TrainingReport report = trained->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";
  const std::vector<std::string> saved = trained->dump_state_lines();
  const std::vector<std::string_view> views(saved.begin(), saved.end());

  auto restored = std::make_shared<nlp::TaggerStage>();
  restored->load_state_lines(views);

  nlp::Workflow original_wf{.lang = nlp::Language::English,
                            .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), trained}};
  nlp::Workflow restored_wf{.lang = nlp::Language::English,
                            .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), restored}};
  const nlp::Doc before = original_wf.infer("Ada writes books.");
  const nlp::Doc after = restored_wf.infer("Ada writes books.");

  std::cout << "saved lines = " << saved.size() << "\n";
  for (std::size_t i = 0; i < before.tokens.size(); ++i) {
    std::cout << before.tokens[i].orth << " / " << before.tokens[i].pos << " | restored "
              << after.tokens[i].pos << "\n";
  }
  return 0;
}
```

Output

```text
docs = 1, entries = 11, steps = 0
saved lines = 33
Ada / PROPN | restored PROPN
writes / VERB | restored VERB
books / NOUN | restored NOUN
. / PUNCT | restored PUNCT
```

### Example 4

The Sequence backend trained on two sentences with four tags, then applied to
an unseen sentence. The classifier decides every token it covers, and the
output comes from the trained model rather than the rules. Training is
seeded, so the output is the same on every run with the same seed. The word
`Zed` is unseen in training. The heuristic rules would tag it `VERB` from its
`-ed` ending, as the previous example showed, so the `PROPN` here comes from
the classifier.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  std::vector<nlp::Doc> train;
  for (const char* text : {"Ada writes books.", "Kofi reads notes."}) {
    nlp::Doc doc = base.infer(text);
    const char* tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
    for (std::size_t i = 0; i < doc.tokens.size(); ++i) doc.tokens[i].pos = tags[i];
    train.push_back(doc);
  }

  auto tagger = std::make_shared<nlp::TaggerStage>(nlp::TaggerOptions{
      .backend = nlp::TaskBackend::Sequence,
      .sequence_width = 16,
      .sequence_seed = 5,
  });
  tagger->learn(train, {});
  const nlp::TrainingReport report = tagger->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), tagger}};
  const nlp::Doc doc = wf.infer("Zed writes papers.");
  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.orth << " / " << token.pos << "\n";
  }
  return 0;
}
```

Output

```text
docs = 2, entries = 16, steps = 60
final loss = 0.0201613
Zed / PROPN
writes / VERB
papers / NOUN
. / PUNCT
```

### Example 5

The Sequence backend needs two distinct tags to train. A corpus where every
token has the same tag throws, and the message says what to change.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc doc = base.infer("Ada writes books.");
  for (nlp::Token& token : doc.tokens) token.pos = "NOUN";

  nlp::TaggerStage tagger{nlp::TaggerOptions{.backend = nlp::TaskBackend::Sequence, .sequence_width = 16}};
  try {
    tagger.learn({doc}, {});
    std::cout << "trained\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
nlp::detail::train_token_classifier: expected at least two distinct labels.
```

## See Also

- `stage::tagger` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`SyntaxStage`](SyntaxStage.md) -- reads the tags this stage writes.
- [`LemmatizerStage`](LemmatizerStage.md) -- supplies the lemmas that `use_lemmas` compares.
- [`NerStage`](NerStage.md) -- entity recognition, which uses the same trainable classifier.
- [`assess::tagger`](assess_tagger.md) -- scores tags against gold labels.

## Changelog

- **0.1** -- initial documented surface.
- **0.1 (fix)** -- the Sequence backend's predictions take precedence over the heuristics, so selecting the backend takes effect with heuristics on.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
