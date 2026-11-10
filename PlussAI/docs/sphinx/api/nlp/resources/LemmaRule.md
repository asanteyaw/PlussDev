<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::LemmaRule`

One exact surface-to-lemma pair in a language's lemma table.

## Declaration

### `pluss::ai::nlp::resources::LemmaRule` fields

| Field | Type | Default |
|---|---|---|
| `surface` | `std::string_view` | empty |
| `lemma` | `std::string_view` | empty |

## Fields

### `surface`

`std::string_view`. The surface form, lowercase, as the lookup key. The match is on the lowercased token, so `WROTE` and `wrote` both find `wrote`.

### `lemma`

`std::string_view`. The base form the surface maps to, such as `write` for `wrote`.

## Guidance

A rule is a single exact pair. The lemmatizer checks the rule table in order and takes the first match, so
keep each surface form once. The table is read-only at runtime; these records are the source of the built-in data.

## See Also

- [`LanguagePack`](LanguagePack.md) -- holds these rules.
- [`lookup_lemma`](lookup_lemma.md) -- the function that reads them.

## Practical Examples

### Example 1

Looking up a lemma from a language pack. The program scans the pack's rules and prints the first matching pair.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  for (const res::LemmaRule& rule : res::pack(nlp::Language::English).lemma_rules) {
    if (rule.surface == "wrote") std::cout << rule.surface << " -> " << rule.lemma << "\n";
  }
  return 0;
}
```

Output

```text
wrote -> write
```

### Example 2

The table has a fixed number of entries per language, which the program prints for English and German.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "english rules = " << res::pack(nlp::Language::English).lemma_rules.size() << "\n";
  std::cout << "german rules = " << res::pack(nlp::Language::German).lemma_rules.size() << "\n";
  return 0;
}
```

Output

```text
english rules = 6
german rules = 5
```

### Example 3

Looking up a rule by its lowercase key. The lookup function gives the same answer for any capitalization of the surface.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << res::lookup_lemma(nlp::Language::English, "WROTE") << "\n";
  return 0;
}
```

Output

```text
write
```

## Changelog

- **0.1** -- initial documented surface.
