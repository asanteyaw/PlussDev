<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::available_languages`

## Signature

```cpp
std::vector<pluss::ai::nlp::Language> pluss::ai::nlp::resources::available_languages();
```

## What It Does

Lists the languages that have a data pack, in a fixed order. The unknown language is not listed, because it has no data. Use the list to drive a loop over the supported languages, such as when checking every pack.

## Parameters

None.

## Returns

A `std::vector<Language>` of the supported languages.

## Errors

None.

## Practical Examples

### Example 1

Listing the supported languages by name.

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

The number of supported languages.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::cout << "languages = " << res::available_languages().size() << "\n";
  return 0;
}
```

Output

```text
languages = 4
```

### Example 3

Checking whether the unknown language is listed. It is not.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  bool listed = false;
  for (const nlp::Language lang : res::available_languages()) listed = listed || lang == nlp::Language::Unknown;
  std::cout << "unknown listed = " << listed << "\n";
  return 0;
}
```

Output

```text
unknown listed = 0
```

## See Also

- [`pack`](pack.md) -- reads the pack of each listed language.
- [`language_name`](language_name.md) -- names each one.
