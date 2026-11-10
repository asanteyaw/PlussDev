<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::resources::TokenMergeRule`

One pair of adjacent tokens that forms a single unit in a language.

## Declaration

### `pluss::ai::nlp::resources::TokenMergeRule` fields

| Field | Type | Default |
|---|---|---|
| `left` | `std::string_view` | empty |
| `right` | `std::string_view` | empty |
| `merged` | `std::string_view` | empty |

## Fields

### `left`

`std::string_view`. The left token, matched exactly and case-sensitively, as the splitter produced it. `Dr` in `Dr .`.

### `right`

`std::string_view`. The right token, matched exactly. For honorifics this is the period `.`.

### `merged`

`std::string_view`. The single token the pair becomes, such as `Dr.`.

## Guidance

A merge rule applies only to two adjacent tokens, in order, and only when the left token matches exactly. A
merge keeps the character span of both tokens and recomputes the merged token's morphology.

## See Also

- [`try_merge_token_exception`](try_merge_token_exception.md) -- the lookup that applies these rules.
- [`LanguagePack`](LanguagePack.md) -- the list these rules belong to.

## Practical Examples

### Example 1

The English honorific rules, printed as left, right, and merged.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  for (const res::TokenMergeRule& rule : res::pack(nlp::Language::English).token_merges) {
    std::cout << rule.left << " + " << rule.right << " -> " << rule.merged << "\n";
  }
  return 0;
}
```

Output

```text
Dr + . -> Dr.
Mr + . -> Mr.
Mrs + . -> Mrs.
Ms + . -> Ms.
Prof + . -> Prof.
```

### Example 2

Case-sensitivity. The rule matches `Dr`, not `dr`, so the lookup of the lowercase pair fails.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  std::string merged;
  std::cout << "Dr + . = " << res::try_merge_token_exception(nlp::Language::English, "Dr", ".", merged) << "\n";
  std::cout << "dr + . = " << res::try_merge_token_exception(nlp::Language::English, "dr", ".", merged) << "\n";
  return 0;
}
```

Output

```text
Dr + . = 1
dr + . = 0
```

### Example 3

The merge in use. The tokenizer applies it when the language exceptions option is on, so `Dr . Ada` becomes `Dr.` followed by `Ada`.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;
namespace res = pluss::ai::nlp::resources;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  for (const std::string& text : wf.infer("Dr . Ada").token_texts()) std::cout << "[" << text << "]\n";
  return 0;
}
```

Output

```text
[Dr.]
[Ada]
```

## Changelog

- **0.1** -- initial documented surface.
