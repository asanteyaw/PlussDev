<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::language_code`

## Signature

```cpp
std::string_view pluss::ai::nlp::resources::language_code(pluss::ai::nlp::Language lang) noexcept;
```

## What It Does

Returns the two-letter code of `lang`, the reverse of [`parse_language_code`](parse_language_code.md). The unknown language has the code `unknown`.

## Parameters

`lang` is the language whose code is wanted.

## Returns

The code as a `std::string_view` into static storage, such as `en` for English.

## Errors

None. The function is `noexcept`.

## Practical Examples

### Example 1

The code of each supported language.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  for (const nlp::Language lang : res::available_languages()) std::cout << res::language_code(lang) << "\n";
  return 0;
}
```

Output

```text
en
fr
es
de
```

### Example 2

A round trip. Parsing a code and asking for its code again gives the same string.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "round trip = " << res::language_code(res::parse_language_code("de")) << "\n";
  return 0;
}
```

Output

```text
round trip = de
```

### Example 3

The unknown language's code.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "unknown code = " << res::language_code(nlp::Language::Unknown) << "\n";
  return 0;
}
```

Output

```text
unknown code = unknown
```

## See Also

- [`parse_language_code`](parse_language_code.md) -- the reverse conversion.
- [`language_name`](language_name.md) -- the display name.
