<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::language_name`

## Signature

```cpp
std::string_view pluss::ai::nlp::resources::language_name(pluss::ai::nlp::Language lang) noexcept;
```

## What It Does

Returns the display name of `lang`, such as `English`. Use it for labels a person reads; use [`language_code`](language_code.md) for storage and comparisons.

## Parameters

`lang` is the language whose name is wanted.

## Returns

The name as a `std::string_view` into static storage.

## Errors

None. The function is `noexcept`.

## Practical Examples

### Example 1

The name of each supported language.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  for (const nlp::Language lang : res::available_languages()) std::cout << res::language_name(lang) << "\n";
  return 0;
}
```

Output

```text
English
French
Spanish
German
```

### Example 2

A label built from the name and the code, as a settings screen might show it.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << res::language_name(nlp::Language::Spanish) << " [" << res::language_code(nlp::Language::Spanish) << "]\n";
  return 0;
}
```

Output

```text
Spanish [es]
```

### Example 3

The unknown language's name.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << res::language_name(nlp::Language::Unknown) << "\n";
  return 0;
}
```

Output

```text
Unknown
```

## See Also

- [`language_code`](language_code.md) -- the code form.
- [`available_languages`](available_languages.md) -- the names to list.
