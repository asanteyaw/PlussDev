<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::is_title`

## Signature

```cpp
bool pluss::ai::nlp::resources::is_title(pluss::ai::nlp::Language lang, std::string_view token) noexcept;
```

## What It Does

Reports whether `token` is one of the language's titles, the words that precede a person's name, such as `Mr.`. The check lowercases the token first. A title is a different list from the abbreviations: `Sir` is a title and is not written with a period.

## Parameters

`lang` is the language whose list is checked.

`token` is the token text to test.

## Returns

`true` when the lowercased token is listed as a title for `lang`, `false` otherwise.

## Errors

None. The function is `noexcept`.

## Practical Examples

### Example 1

Checking English titles, with and without the period.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "Mr. = " << res::is_title(nlp::Language::English, "Mr.") << "\n";
  std::cout << "Sir = " << res::is_title(nlp::Language::English, "Sir") << "\n";
  return 0;
}
```

Output

```text
Mr. = 1
Sir = 1
```

### Example 2

A common word that is not a title.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "Ada = " << res::is_title(nlp::Language::English, "Ada") << "\n";
  return 0;
}
```

Output

```text
Ada = 0
```

### Example 3

Titles are checked against the language's own list.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "Dr. in English = " << res::is_title(nlp::Language::English, "Dr.") << "\n";
  std::cout << "Dr. in German = " << res::is_title(nlp::Language::German, "Dr.") << "\n";
  return 0;
}
```

Output

```text
Dr. in English = 1
Dr. in German = 1
```

## See Also

- [`is_abbreviation`](is_abbreviation.md) -- the related check for abbreviations.
- [`LanguagePack`](LanguagePack.md) -- the title list.
