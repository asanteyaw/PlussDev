# `pluss::ai::nlp::runtime::SequenceOptions`

The configuration that turns a document into a sequence of vocabulary ids for the neural runtime.
It decides which tokens become sequence elements, how their text is normalized, and which special
tokens frame the sequence. [`Indexer`](Indexer.md) reads it at construction, and the same options must
be used for the training and the inference sequences, or the ids will not line up.

## Fields at a glance

| Field | Type | Default |
|---|---|---|
| `use_lemmas` | `bool` | `true` |
| `skip_punct` | `bool` | `true` |
| `lowercase` | `bool` | `true` |
| `add_bos` | `bool` | `false` |
| `add_eos` | `bool` | `false` |
| `batch_first` | `bool` | `true` |
| `pad_token` | `std::string` | `"<pad>"` |
| `unk_token` | `std::string` | `"<unk>"` |
| `bos_token` | `std::string` | `"<bos>"` |
| `eos_token` | `std::string` | `"<eos>"` |

## Fields

### `use_lemmas`

`bool`, default `true`. A token contributes its lemma when the lemma is non-empty, and its surface form
otherwise. Without a lemmatizer in the pipeline every lemma is empty, so the option has no visible effect.
Turn it off when the inflected form itself must be a separate id.

### `skip_punct`

`bool`, default `true`. Tokens marked as punctuation are left out of the sequence. A sequence then holds
words only, which is what most word-level models expect. Turn it off when punctuation carries signal, such
as a question mark deciding the meaning of a sentence.

### `lowercase`

`bool`, default `true`. Every element is lowercased before it is looked up or added to the vocabulary, so
`Ada` and `ada` share one id. Turn it off when case distinguishes words in the corpus, such as acronyms
that collide with ordinary words when folded.

### `add_bos`

`bool`, default `false`. When `true`, the bos token is placed before every sequence, and it gets its own
vocabulary id. Sequence models that need a start marker, or that read the first position as a summary,
use it.

### `add_eos`

`bool`, default `false`. The same as `add_bos` at the end of each sequence: the eos token is appended and
given its own id. Sequences that are scored by where they stop need it.

### `batch_first`

`bool`, default `true`. Chooses the layout of every batch the indexer returns: `[batch, sequence]` when
true, `[sequence, batch]` when false. The encoders check that this matches their own `batch_first` setting,
so a mismatch is reported when a sequence reaches the model rather than silently transposing it.

### `pad_token`

`std::string`, default `"<pad>"`. The token used to fill short sequences in a batch. It is always the
first vocabulary entry, id `0`, and it is masked out of every computation that follows.

### `unk_token`

`std::string`, default `"<unk>"`. The token that stands for any term the vocabulary does not hold. It is
always the second vocabulary entry, id `1`, so an unseen word at inference time becomes a known id instead
of an error.

### `bos_token`

`std::string`, default `"<bos>"`. The token added when `add_bos` is on. It is only added to the vocabulary
when that option is on.

### `eos_token`

`std::string`, default `"<eos>"`. The token added when `add_eos` is on, with the same rule as `bos_token`.

## Guidance

Fix the options before learning and keep them fixed. Every field changes the vocabulary or the layout, and
a saved [`Indexer`](Indexer.md) carries its options with it, so a restored indexer keeps the choices it was
trained with.

The special tokens are ordinary vocabulary entries once they are added. If a corpus happens to contain the
literal text `<pad>`, it maps to the pad id, so choose tokens that cannot appear in the input.

## See Also

- [`Indexer`](Indexer.md) -- reads these options and produces batches.
- [`SequenceBatch`](SequenceBatch.md) -- the batch the indexer returns.
- [`EmbeddingTable`](EmbeddingTable.md) -- turns the ids into vectors.

## Practical Examples

These examples show how each option changes the sequence a document produces, and what happens to the
vocabulary when the special tokens are on.

### Example 1

The sequence a sentence produces under the default options, with each element's text printed in order.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books, Kofi reads."});

  nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{}};
  indexer.learn(docs);
  std::cout << "vocabulary size = " << indexer.vocabulary().size() << "\n";
  std::cout << "sequence:";
  for (const std::string& term : indexer.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
vocabulary size = 7
sequence: <pad> <unk> ada writes books kofi reads
```

### Example 2

Case and punctuation. With `lowercase` and `skip_punct` off, the same sentence yields a vocabulary with
capitalized entries and punctuation marks as elements.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books, Kofi reads."});

  nlp::runtime::SequenceOptions options{.skip_punct = false, .lowercase = false};
  nlp::runtime::Indexer indexer{options};
  indexer.learn(docs);
  std::cout << "vocabulary:";
  for (const std::string& term : indexer.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
vocabulary: <pad> <unk> Ada writes books , Kofi reads .
```

### Example 3

The boundary tokens. With `add_bos` and `add_eos` on, both markers appear in the vocabulary with their
own ids, and the program prints those ids.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books."});

  nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{.add_bos = true, .add_eos = true}};
  indexer.learn(docs);
  std::cout << "pad_id = " << indexer.pad_id() << ", unk_id = " << indexer.unk_id() << "\n";
  std::cout << "bos_id = " << indexer.bos_id() << ", eos_id = " << indexer.eos_id() << "\n";
  return 0;
}
```

Output

```text
pad_id = 0, unk_id = 1
bos_id = 2, eos_id = 3
```

## Changelog

- **0.1** -- initial documented surface.
