<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::lookup_lemma`

## Signature

```cpp
std::string pluss::ai::nlp::resources::lookup_lemma(pluss::ai::nlp::Language lang, std::string_view token);
```

## What It Does

Looks up the base form of `token` in the language's lemma table. The lookup lowercases the token and returns the lemma of the first matching rule. A token the table does not list returns an empty string, which tells the lemmatizer to fall back to its rule-based lemma.

## Parameters

`lang` is the language whose table is searched.

`token` is the surface form to look up.

## Returns

The lemma as a `std::string`, or an empty string when the table has no entry.

## Errors

None. A token the table does not hold returns an empty string.

## Practical Examples

### Example 1

Looking up irregular English forms.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "wrote -> " << res::lookup_lemma(nlp::Language::English, "wrote") << "\n";
  std::cout << "were -> " << res::lookup_lemma(nlp::Language::English, "were") << "\n";
  return 0;
}
```

Output

```text
wrote -> write
were -> be
```

### Example 2

The lookup is case-insensitive, so an uppercase form finds the same lemma.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "WROTE -> " << res::lookup_lemma(nlp::Language::English, "WROTE") << "\n";
  return 0;
}
```

Output

```text
WROTE -> write
```

### Example 3

A regular form is not in the table, so the lookup returns an empty string.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "books -> [" << res::lookup_lemma(nlp::Language::English, "books") << "]\n";
  return 0;
}
```

Output

```text
books -> []
```

## See Also

- [`LemmaRule`](LemmaRule.md) -- the records the table holds.
- [`LemmatizerStage`](../tasks/LemmatizerStage.md) -- consumes the lookup.
