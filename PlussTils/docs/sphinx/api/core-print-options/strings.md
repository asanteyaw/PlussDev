# `PrintOptions: strings`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

`quote_strings` wraps each string in a container with `quote_char`. `case_transform` changes the case of the rendered text. `escape_non_printable` is accepted, but it is a reserved field: it does not yet change how a non-printable byte is written.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 quotes the strings inside a list.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("names:", std::vector<std::string>{"ama", "kofi, jr"}, PrintOptions{.quote_strings = true});
  return 0;
}
```

Output

```text
names: ["ama", "kofi, jr"]
```

Each name is wrapped in quotes, so a name that contains a comma is still clearly one element.

### Example 2

Example 2 sets a different quote character.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("names:", std::vector<std::string>{"ama", "kofi"},
              PrintOptions{.quote_strings = true, .quote_char = '\''});
  return 0;
}
```

Output

```text
names: ['ama', 'kofi']
```

A single quote wraps each name instead of the default double quote.

### Example 3

Example 3 shows all four case transforms.

```cpp
#include <pluss/tils/auxiliary/output.h>

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

Upper and lower change every letter, title capitalizes each word, and sentence capitalizes the first letter of each sentence and lowercases the rest.

### Example 4

Example 4 sets escape_non_printable and compares it with the option left off.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  // escape_non_printable is accepted, but byte-level escaping is not implemented yet, so a tab
  // passes through unescaped. This is a documented, reserved field, not a claim of current behavior.
  out.writeln("with a tab:", "a\tb", PrintOptions{.escape_non_printable = true});
  out.writeln("without the option:", "a\tb");
  return 0;
}
```

Output

```text
with a tab: a	b
without the option: a	b
```

Both lines carry the raw tab character. The option is accepted, but it has no visible effect yet.

## See Also

- [`CaseXf`](../core-case-xf.md)
