<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6874620647cf -->
# `pluss::ai::nlp::Doc::lemma_texts`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::string> lemma_texts() const;
```

## What It Does

Returns each token's lemma (`Token::lemma`), in token order. `Token::lemma`
is only filled in by [`LemmatizerStage`](../../tasks/LemmatizerStage.md)
-- without it in the pipeline, every entry is an empty string, not the
token's own surface form; this never silently falls back to
[`token_texts`](token_texts.md).

## Parameters

None.

## Returns

A `std::vector<std::string>`, one entry per token (possibly empty
strings), empty overall if `tokens` is empty.

## Errors

None.

## Practical Examples

### Example 1

A lemmatized pipeline normalizes inflected forms.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("running dogs");

  for (const std::string& lemma : doc.lemma_texts()) {
    std::cout << "[" << lemma << "]";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
[run][dog]
```

### Example 2

Without a lemmatizer stage, every entry is an empty string.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  bool all_empty = true;
  for (const std::string& lemma : doc.lemma_texts()) {
    if (!lemma.empty()) all_empty = false;
  }
  std::cout << "all_empty_without_lemmatizer=" << all_empty << "\n";
  return 0;
}
```

Output

```text
all_empty_without_lemmatizer=1
```

### Example 3

Punctuation still gets a lemma -- its own surface form.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  for (const std::string& lemma : doc.lemma_texts()) {
    std::cout << "[" << lemma << "]";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
[laurel][write][api][.]
```

## See Also

- [`token_texts`](token_texts.md) -- the surface-form counterpart.
- [`LemmatizerStage`](../../tasks/LemmatizerStage.md) -- what fills
  `lemma` in.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="left_edge_of.html">
    <span class="label">Previous</span>
    <span class="title">left_edge_of</span>
  </a>
  <a class="next-link" href="lineage_of.html">
    <span class="label">Next</span>
    <span class="title">lineage_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
