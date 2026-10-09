<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c4704d6f6c78 -->
# `pluss::ai::nlp::Doc::token_texts`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::string> token_texts() const;
```

## What It Does

Returns each token's surface form (`Token::orth`), in token order --
the quickest way to read a tokenized `Doc` back out as plain strings
without walking `tokens` by hand. Punctuation tokens are included,
each as its own entry.

## Parameters

None.

## Returns

A `std::vector<std::string>`, one entry per token, empty if `tokens` is
empty.

## Errors

None.

## Practical Examples

### Example 1

A simple sentence, tokenized and read back.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");
  const std::vector<std::string> texts = doc.token_texts();

  std::cout << "count=" << texts.size() << " first=" << texts.front() << "\n";
  return 0;
}
```

Output

```text
count=4 first=Laurel
```

### Example 2

Punctuation, an email address, and a currency amount each become their
own token.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("help@pluss.dev costs $42.");

  for (const std::string& token : doc.token_texts()) {
    std::cout << "[" << token << "]";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
[help@pluss.dev][costs][$][42][.]
```

### Example 3

A `Doc` that never went through a tokenizer stage has no tokens at
all.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::Doc doc{};
  std::cout << "count=" << doc.token_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

## See Also

- [`lemma_texts`](lemma_texts.md) -- the same shape, reading `lemma`
  instead of `orth`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sentence_texts.html">
    <span class="label">Previous</span>
    <span class="title">sentence_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
