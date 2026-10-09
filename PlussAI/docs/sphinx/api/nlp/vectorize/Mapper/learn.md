<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c189d1443b69 -->
# `pluss::ai::nlp::vectorize::Mapper::learn`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::Status pluss::ai::nlp::vectorize::Mapper::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs);
pluss::ai::classic::Status pluss::ai::nlp::vectorize::Mapper::learn(
    pluss::RangeView<std::string_view> texts);
pluss::ai::classic::Status pluss::ai::nlp::vectorize::Mapper::learn(
    const std::vector<std::string>& texts);
pluss::ai::classic::Status pluss::ai::nlp::vectorize::Mapper::learn(
    const pluss::Note& texts);
```

## What It Does

Reads a training corpus and replaces the mapper's vocabulary. Each document is first turned into
feature text under the mapper's [`DocMapOptions`](../DocMapOptions.md), then split into terms the
way the mapper's analyzer splits them, lowercased and expanded to the configured n-gram range. The
mapper counts, for each term, the documents that contain it.

The vocabulary is then filtered and ordered. Terms that appear in fewer documents than
`min_document_frequency` are dropped. The rest are ordered by document frequency, most frequent
first, with ties broken alphabetically, and only the first `max_features` survive. The order is the
column order of every later mapping call, so two runs over the same corpus produce the same columns.

A second call to `learn` replaces the vocabulary entirely; it does not merge the new corpus into the
old one. The overloads accept the corpus in whichever form is at hand: documents that have been run
through a pipeline, plain strings, a view of strings, or a `pluss::Note` of text.
For text without inflection, documents and strings give the same vocabulary, because a string
is not run through a pipeline and so carries no lemmas or punctuation classification.

## Parameters

`docs` is a vector of tokenized documents, the form the pipeline produces. Its lemmas and
punctuation flags are used under the `DocMapOptions` rule.

`texts` is the corpus as text: a vector of strings, a view of strings, or a `Note`. Each string is
analyzed directly, with no pipeline, so the lemma rule does not apply to these overloads.

## Returns

A classic `Status`. It reports success with `ok()`, and it reports failure with a message when no
term survives the document-frequency filter or when the n-gram range is invalid.

## Errors

- The n-gram range is invalid, for example `min_ngram` greater than `max_ngram`, or either bound
  below one.
- No term reaches `min_document_frequency`, so the vocabulary would be empty.

## Practical Examples

Three examples: learning from documents, learning from plain strings, and learning from a `Note`,
with the vocabulary printed in each case so the shared order can be seen.

### Example 1

Learning from tokenized documents. The documents went through a lemmatizer, so `wrote` and `writes`
share one column.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada wrote books.", "Kofi writes books."});

  nlp::vectorize::Mapper mapper{};
  const auto status = mapper.learn(docs);
  std::cout << "learn ok = " << status.ok() << "\n";
  std::cout << "vocabulary:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
learn ok = 1
vocabulary: book write ada kofi
```

### Example 2

Learning from plain strings. No pipeline runs, so the text is analyzed as written, and the result
matches the document route for text with no lemmas to collapse.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::vectorize::Mapper mapper{};
  const std::vector<std::string> corpus = {"cat sat", "dog sat", "cat cat"};
  const auto status = mapper.learn(corpus);
  std::cout << "learn ok = " << status.ok() << "\n";
  std::cout << "vocabulary:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
learn ok = 1
vocabulary: cat sat dog
```

### Example 3

Learning from a `Note` and replacing the vocabulary by learning again. The second corpus does not
share the first corpus's terms, so the old columns are gone.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::vectorize::Mapper mapper{};
  mapper.learn(pluss::make_note({"red apple", "green apple"}));
  std::cout << "first vocabulary:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";

  mapper.learn(pluss::make_note({"blue sky", "blue sea"}));
  std::cout << "second vocabulary:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
first vocabulary: apple green red
second vocabulary: blue sea sky
```

## See Also

- [`count_dense`](count_dense.md) and [`count_sparse`](count_sparse.md) -- map documents over the learned vocabulary.
- [`vocabulary`](vocabulary.md) -- reads the learned terms.
- [`describe`](../describe.md) -- previews the vocabulary without learning.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dump_state.html">
    <span class="label">Previous</span>
    <span class="title">dump_state</span>
  </a>
  <a class="next-link" href="load_state.html">
    <span class="label">Next</span>
    <span class="title">load_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
