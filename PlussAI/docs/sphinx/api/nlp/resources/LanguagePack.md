<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::LanguagePack`

The complete data set for one language: its identity, abbreviations, titles, lemma rules, and token merges.

## Declaration

```cpp
struct pluss::ai::nlp::resources::LanguagePack {
  pluss::ai::nlp::Language lang = pluss::ai::nlp::Language::Unknown;
  std::string_view code{};
  std::string_view name{};
  pluss::RangeView<std::string_view> abbreviations{};
  pluss::RangeView<std::string_view> titles{};
  pluss::RangeView<LemmaRule> lemma_rules{};
  pluss::RangeView<TokenMergeRule> token_merges{};
};
```

## Fields

### `lang`

`nlp::Language`. The language this pack describes. The unknown language has an empty pack.

### `code`

`std::string_view`. The two-letter language code, such as `en`, as returned by [`language_code`](language_code.md).

### `name`

`std::string_view`. The language's display name, such as `English`.

### `abbreviations`

`RangeView<std::string_view>`. The lowercase abbreviations, each with its trailing period, such as `dr.`. The sentencizer does not end a sentence at these.

### `titles`

`RangeView<std::string_view>`. The lowercase titles that precede a name, such as `mr.` and `sir`. The entity recognizer labels the name that follows.

### `lemma_rules`

`RangeView<LemmaRule>`. The exact surface-to-lemma pairs the lemmatizer consults.

### `token_merges`

`RangeView<TokenMergeRule>`. The adjacent-token pairs that the tokenizer joins when language exceptions are on.

## Guidance

A pack is read through [`pack`](pack.md) and is never modified. Each list in a pack is independent of the others;
a language may have abbreviations but no lemma rules, and the stages handle an empty list as no data.

## See Also

- [`pack`](pack.md) -- returns a pack.
- [`LemmaRule`](LemmaRule.md) and [`TokenMergeRule`](TokenMergeRule.md) -- the records the lists hold.

## Practical Examples

### Example 1

Printing the identity fields of a language pack.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  const res::LanguagePack& pack = res::pack(nlp::Language::French);
  std::cout << pack.name << " (" << pack.code << ")\n";
  return 0;
}
```

Output

```text
French (fr)
```

### Example 2

Counting each list of the pack, so the data can be compared across languages.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  for (const nlp::Language lang : res::available_languages()) {
    const res::LanguagePack& pack = res::pack(lang);
    std::cout << pack.name << ": abbreviations = " << pack.abbreviations.size() << ", titles = " << pack.titles.size() << "\n";
  }
  return 0;
}
```

Output

```text
English: abbreviations = 10, titles = 6
French: abbreviations = 5, titles = 5
Spanish: abbreviations = 6, titles = 5
German: abbreviations = 4, titles = 4
```

### Example 3

The unknown language's pack is empty, so a document in an unsupported language gets no language data.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  const res::LanguagePack& unknown = res::pack(nlp::Language::Unknown);
  std::cout << "abbreviations = " << unknown.abbreviations.size() << ", merges = " << unknown.token_merges.size() << "\n";
  return 0;
}
```

Output

```text
abbreviations = 0, merges = 0
```

## Changelog

- **0.1** -- initial documented surface.
