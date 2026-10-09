<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: badeefee8457 -->

# `pluss::ai::nlp::Workflow::infer`

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## Signature

```cpp
pluss::ai::nlp::Doc pluss::ai::nlp::Workflow::infer(std::string_view text) const;
std::vector<pluss::ai::nlp::Doc> pluss::ai::nlp::Workflow::infer(pluss::RangeView<std::string_view> texts) const;
std::vector<pluss::ai::nlp::Doc> pluss::ai::nlp::Workflow::infer(const std::vector<std::string>& texts) const;
std::vector<pluss::ai::nlp::Doc> pluss::ai::nlp::Workflow::infer(const pluss::Note& texts) const;
```

## What It Does

Runs every stage over the input and returns the annotated documents. A single string gives one `Doc`; a list of strings, a `Note` of text, or a range of views gives one `Doc` per input text, in the same order. Each document is independent: it starts from its own text and its own lexicon, and no stage carries information from one document to the next. `infer` never changes the workflow, so it can be called concurrently on a trained workflow.

## Parameters

`text` / `texts`
: One string, or several in a list or `pluss::Note`. Texts are processed independently.

## Returns

A `Doc` for a single string, or a `std::vector<Doc>` for a collection, one per input text.

## Errors

Propagates any error a stage throws while annotating a document.

## Practical Examples

### Example 1

A single string returns one annotated document.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const nlp::Doc doc = wf.infer(std::string_view("A single text. Two sentences."));
  std::cout << "sentences=" << doc.sentence_texts().size() << "\n";
  return 0;
}
```

Output

```text
sentences=2
```

### Example 2

A list of strings returns one document per string, in input order.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const std::vector<nlp::Doc> docs = wf.infer(std::vector<std::string>{"Laurel writes APIs. Second sentence!", "Only one here."});
  std::cout << "docs=" << docs.size() << " first_tokens=" << docs[0].tokens.size() << " second_tokens=" << docs[1].tokens.size() << "\n";
  return 0;
}
```

Output

```text
docs=2 first_tokens=7 second_tokens=4
```

### Example 3

A `pluss::Note` of texts is processed the same way as a list of strings.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const std::vector<nlp::Doc> docs = wf.infer(pluss::make_note({"Laurel writes APIs.", "Second text here."}));
  std::cout << "docs=" << docs.size() << " first=" << docs[0].token_texts()[0] << "\n";
  return 0;
}
```

Output

```text
docs=2 first=Laurel
```

## See Also

- [`learn`](learn.md) -- training before inference.
- [`assess`](assess.md) -- summarizing the same kind of run.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
