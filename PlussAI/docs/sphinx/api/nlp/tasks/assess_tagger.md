# `pluss::ai::nlp::assess::tagger`

Scores part-of-speech tags against gold tags across a corpus and returns a
[`TagScore`](TagScore.md). The function compares the `pos` field of each
predicted token with the `pos` of the gold token at the same position, counts
the matches, and reports the share of tokens that matched.

The two documents in each pair must describe the same text. Before any tag is
compared, the function checks that the predicted and gold documents have the
same token count, and that each token pair has the same character offsets and
surface text. A mismatch is an error, not a score, because a tag compared
against a different token says nothing about tagging quality.

## Signature

```cpp
pluss::ai::nlp::TagScore pluss::ai::nlp::assess::tagger(
    const std::vector<pluss::ai::nlp::Doc>& predicted,
    const std::vector<pluss::ai::nlp::Doc>& gold);
```

## What It Does

Walks the document pairs in order. For each pair it verifies that the two documents
align token for token, then counts every token whose predicted tag equals its gold
tag. The totals cover the whole corpus, so a long document weighs more than a short
one. The accuracy is the total number of correct tokens divided by the total number
of tokens, and it is `0` when the corpus has no tokens.

Two empty tags are equal, so a token that neither side tagged counts as correct. The
function does not know whether an empty tag means "unknown" or "no tag", so it treats
the two sides alike. Make sure both sides were tagged before reading the accuracy as a
quality measure.

## Parameters

`predicted` is the documents as a tagger produced them, one `Doc` per input text, with
the tags in `Token::pos`. It must have the same length as `gold`, and each document must
have the same tokens, in the same order, as its gold counterpart.

`gold` is the reference documents, tagged by hand or by a trusted process, with the same
token alignment as `predicted`. The function reads only the `pos` field of the gold
tokens and the alignment fields, so the gold documents may carry other annotations.

## Returns

A `TagScore` with `doc_count` set to the number of pairs, `token_count` to the total
number of tokens, `correct_count` to the number of matching tags, and `accuracy` to the
ratio of the last two. The struct is described on its own page.

## Errors

- The two vectors have different lengths: the document counts must match.
- A document pair has different token counts, or a token differs in character span or
  surface text: the token sequences must align exactly. The message names the function
  and the mismatch.

## Practical Examples

These examples score a tagger's output against a reference, show the alignment
requirement, and show that the score counts tokens rather than documents. Each one
builds its own predicted and gold documents from the same text, so the alignment holds
by construction.

### Example 1

A predicted tagging compared with a gold tagging of one sentence. Three of the four
tokens match, so the accuracy is three quarters. The program prints each token's
predicted and gold tag, so the score can be checked by eye.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::Doc gold = base.infer("Ada writes books.");
  const char* gold_tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
  for (std::size_t i = 0; i < gold.tokens.size(); ++i) gold.tokens[i].pos = gold_tags[i];

  nlp::Doc predicted = base.infer("Ada writes books.");
  const char* predicted_tags[] = {"PROPN", "NOUN", "NOUN", "PUNCT"};
  for (std::size_t i = 0; i < predicted.tokens.size(); ++i) predicted.tokens[i].pos = predicted_tags[i];

  for (std::size_t i = 0; i < gold.tokens.size(); ++i) {
    std::cout << gold.tokens[i].orth << ": predicted " << predicted.tokens[i].pos << ", gold "
              << gold.tokens[i].pos << "\n";
  }
  const nlp::TagScore score = nlp::assess::tagger({predicted}, {gold});
  std::cout << "documents = " << score.doc_count << ", tokens = " << score.token_count
            << ", correct = " << score.correct_count << ", accuracy = " << score.accuracy << "\n";
  return 0;
}
```

Output

```text
Ada: predicted PROPN, gold PROPN
writes: predicted NOUN, gold VERB
books: predicted NOUN, gold NOUN
.: predicted PUNCT, gold PUNCT
documents = 1, tokens = 4, correct = 3, accuracy = 0.75
```

### Example 2

The alignment requirement. A prediction made from a different sentence has different
tokens, and the function refuses to score it. The program prints the first line of the
error message, which names the mismatch.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc gold = base.infer("Ada writes books.");
  const nlp::Doc other = base.infer("Ada writes.");

  try {
    nlp::assess::tagger({other}, {gold});
    std::cout << "scored\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
nlp::assess::tagger: predicted/gold token counts must match per document.
```

### Example 3

The accuracy is a token-weighted average, not an average of document accuracies. A
short document that is tagged perfectly and a long one that is tagged poorly give
a corpus accuracy that is closer to the long document's, because the long one
contributes more tokens. The program scores each document alone and then the pair
together, so the difference is visible.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc short_gold = base.infer("Stop now.");
  nlp::Doc long_gold = base.infer("Ada reads the long report carefully every morning.");
  nlp::Doc short_pred = short_gold;
  nlp::Doc long_pred = long_gold;
  for (nlp::Token& token : short_gold.tokens) token.pos = "VERB";
  for (nlp::Token& token : short_pred.tokens) token.pos = "VERB";
  for (nlp::Token& token : long_gold.tokens) token.pos = "NOUN";
  for (nlp::Token& token : long_pred.tokens) token.pos = "VERB";

  const nlp::TagScore short_only = nlp::assess::tagger({short_pred}, {short_gold});
  const nlp::TagScore long_only = nlp::assess::tagger({long_pred}, {long_gold});
  const nlp::TagScore both = nlp::assess::tagger({short_pred, long_pred}, {short_gold, long_gold});

  std::cout << "short accuracy = " << short_only.accuracy << "\n";
  std::cout << "long accuracy = " << long_only.accuracy << "\n";
  std::cout << "corpus accuracy = " << both.accuracy << " over " << both.token_count << " tokens\n";
  return 0;
}
```

Output

```text
short accuracy = 1
long accuracy = 0
corpus accuracy = 0.25 over 12 tokens
```

## See Also

- [`TagScore`](TagScore.md) -- the result this function returns.
- [`TaggerStage`](TaggerStage.md) -- produces the predicted tags this function scores.
- [`assess::ner`](assess_ner.md) -- the entity-level counterpart.

## Changelog

- **0.1** -- initial documented surface.
