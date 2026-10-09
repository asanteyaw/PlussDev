<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::is_abbreviation`

## Signature

```cpp
bool pluss::ai::nlp::resources::is_abbreviation(pluss::ai::nlp::Language lang, std::string_view token) noexcept;
```

## What It Does

Reports whether `token` is one of the language's abbreviations, the words written with a trailing period that do not end a sentence. The check lowercases the token first, so `Dr.` and `dr.` both match. The token must include its period; `Dr` alone is not an abbreviation.

## Parameters

`lang` is the language whose list is checked.

`token` is the token text, with its period.

## Returns

`true` when the lowercased token is listed for `lang`, `false` otherwise.

## Errors

None. The function is `noexcept`.

## Practical Examples

### Example 1

Checking a few English abbreviations.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "Dr. = " << res::is_abbreviation(nlp::Language::English, "Dr.") << "\n";
  std::cout << "etc. = " << res::is_abbreviation(nlp::Language::English, "etc.") << "\n";
  return 0;
}
```

Output

```text
Dr. = 1
etc. = 1
```

### Example 2

The period matters. Without it the token is not an abbreviation.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "Dr = " << res::is_abbreviation(nlp::Language::English, "Dr") << "\n";
  return 0;
}
```

Output

```text
Dr = 0
```

### Example 3

Abbreviations are per language. The French form `M.` is listed in French and not in English.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "M. in French = " << res::is_abbreviation(nlp::Language::French, "M.") << "\n";
  std::cout << "M. in English = " << res::is_abbreviation(nlp::Language::English, "M.") << "\n";
  return 0;
}
```

Output

```text
M. in French = 1
M. in English = 0
```

## See Also

- [`is_title`](is_title.md) -- the related check for titles.
- [`LanguagePack`](LanguagePack.md) -- the abbreviation list.
