<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::try_merge_token_exception`

## Signature

```cpp
bool pluss::ai::nlp::resources::try_merge_token_exception(pluss::ai::nlp::Language lang, std::string_view left, std::string_view right, std::string& merged);
```

## What It Does

Checks whether the adjacent tokens `left` and `right` form a single unit in the language. When they do, the merged token is written to `merged` and the function returns true. The match is exact and case-sensitive, so `Dr` followed by `.` merges and `dr` followed by `.` does not.

## Parameters

`lang` is the language whose merge rules apply.

`left` and `right` are the two adjacent token texts, in order.

`merged` receives the merged text when a rule matches. It is left unchanged otherwise.

## Returns

`true` when a rule merges the pair, with `merged` set; `false` otherwise.

## Errors

None. A pair with no rule returns false.

## Practical Examples

### Example 1

Merging an honorific and its period.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::string merged;
  const bool merges = res::try_merge_token_exception(nlp::Language::English, "Dr", ".", merged);
  std::cout << "merges = " << merges << ", merged = " << merged << "\n";
  return 0;
}
```

Output

```text
merges = 1, merged = Dr.
```

### Example 2

A pair with no rule. The function returns false and the output string is not set.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::string merged = "unchanged";
  const bool merges = res::try_merge_token_exception(nlp::Language::English, "Ada", ".", merged);
  std::cout << "merges = " << merges << ", merged = " << merged << "\n";
  return 0;
}
```

Output

```text
merges = 0, merged = unchanged
```

### Example 3

Case-sensitivity. The lowercase honorific does not merge.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::string merged;
  std::cout << "dr + . merges = " << res::try_merge_token_exception(nlp::Language::English, "dr", ".", merged) << "\n";
  return 0;
}
```

Output

```text
dr + . merges = 0
```

## See Also

- [`TokenMergeRule`](TokenMergeRule.md) -- the rule records.
- [`RuleTokenizerStage`](../tasks/RuleTokenizerStage.md) -- applies the merge during tokenization.
