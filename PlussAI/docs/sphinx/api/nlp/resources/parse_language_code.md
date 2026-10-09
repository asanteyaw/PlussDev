<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::parse_language_code`

## Signature

```cpp
pluss::ai::nlp::Language pluss::ai::nlp::resources::parse_language_code(std::string_view code);
```

## What It Does

Converts a two-letter language code to its language. The match is exact and case-sensitive: `en` is English, and `EN` is not recognized. A code that is not recognized gives the unknown language rather than an error, so a caller can branch on the result.

## Parameters

`code` is the language code to parse, one of `en`, `fr`, `es`, or `de`.

## Returns

The matching `Language`, or `Language::Unknown` when the code is not recognized.

## Errors

None. An unrecognized code returns `Language::Unknown`.

## Practical Examples

### Example 1

Parsing the four supported codes.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  for (const char* code : {"en", "fr", "es", "de"}) {
    std::cout << code << " -> " << res::language_name(res::parse_language_code(code)) << "\n";
  }
  return 0;
}
```

Output

```text
en -> English
fr -> French
es -> Spanish
de -> German
```

### Example 2

The match is case-sensitive, so an uppercase code is not recognized.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "EN -> " << static_cast<int>(res::parse_language_code("EN")) << "\n";
  return 0;
}
```

Output

```text
EN -> 0
```

### Example 3

An unknown code gives the unknown language, which the program checks.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "xx is unknown = " << (res::parse_language_code("xx") == nlp::Language::Unknown) << "\n";
  return 0;
}
```

Output

```text
xx is unknown = 1
```

## See Also

- [`language_code`](language_code.md) -- the reverse conversion.
- [`pack`](pack.md) -- the data of the parsed language.
