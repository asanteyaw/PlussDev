# `pluss::tils::CaseXf`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
enum class CaseXf : std::uint8_t {
  None, Upper, Lower, Title, Sentence
};
```

`CaseXf` changes the case of rendered text. `Title` capitalizes the first letter of every word. `Sentence` capitalizes only the first letter of each sentence and lowercases the rest; a new sentence starts after a period, an exclamation mark, or a question mark.

## Parameters

`CaseXf` has no parameters. It is an enumeration.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 shows all four transforms.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("Upper: {}", "hello world", PrintOptions{.case_transform = CaseXf::Upper});
  out.writelnf("Lower: {}", "HELLO WORLD", PrintOptions{.case_transform = CaseXf::Lower});
  out.writelnf("Title: {}", "the quick brown fox", PrintOptions{.case_transform = CaseXf::Title});
  out.writelnf("Sentence: {}", "the QUICK brown fox. a SECOND one.", PrintOptions{.case_transform = CaseXf::Sentence});
  return 0;
}
```

Output

```text
Upper: HELLO WORLD
Lower: hello world
Title: The Quick Brown Fox
Sentence: The quick brown fox. A second one.
```

Each transform changes the text in its own way, and sentence case treats each sentence separately.

## See Also

- [Strings](core-print-options/strings.md)
