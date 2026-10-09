<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::pack`

## Signature

```cpp
const pluss::ai::nlp::resources::LanguagePack& pluss::ai::nlp::resources::pack(pluss::ai::nlp::Language lang) noexcept;
```

## What It Does

Returns the data pack of `lang`: its abbreviations, titles, lemma rules, and token merges. The pack is built into the library and never changes, so the returned reference is valid for the life of the program. An unrecognized language value gives the empty pack.

## Parameters

`lang` is the language whose data is wanted, such as `Language::French`.

## Returns

A `const` reference to the language's [`LanguagePack`](LanguagePack.md).

## Errors

None. The function is `noexcept`.

## Practical Examples

### Example 1

Reading the English pack's name and code.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  const res::LanguagePack& english = res::pack(nlp::Language::English);
  std::cout << english.name << " (" << english.code << ")\n";
  return 0;
}
```

Output

```text
English (en)
```

### Example 2

Comparing the abbreviation lists of two languages, which the program prints as counts.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "english = " << res::pack(nlp::Language::English).abbreviations.size() << "\n";
  std::cout << "spanish = " << res::pack(nlp::Language::Spanish).abbreviations.size() << "\n";
  return 0;
}
```

Output

```text
english = 10
spanish = 6
```

### Example 3

Reading a title from the French pack, the honorific the entity recognizer treats as a person marker.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  const auto titles = res::pack(nlp::Language::French).titles;
  std::cout << "first title = " << titles[0] << "\n";
  return 0;
}
```

Output

```text
first title = m.
```

## See Also

- [`LanguagePack`](LanguagePack.md) -- the record this returns.
- [`available_languages`](available_languages.md) -- lists the languages with data.
