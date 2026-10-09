<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::io::read_workflow`

## Signature

```cpp
pluss::ai::nlp::Workflow pluss::ai::nlp::io::read_workflow(std::istream& in);
```

## What It Does

Reads a workflow written by [`write_workflow`](write_workflow.md). Each stage is rebuilt through the stage registry and then given its saved state. The schema version is checked first, and the checksum over the stage payload is checked before the stages are parsed, so a damaged file is refused with a diagnostic rather than producing a partial pipeline.

## Parameters

`in` is the source stream holding the saved workflow.

## Returns

The restored [`Workflow`](../workflow/Workflow.md), with the same language, stages, and learned state as the one that was saved.

## Errors

- The stream does not hold a saved workflow, its schema is outside the loadable range, its stage count does not match, or its checksum does not match: the call fails with a diagnostic that names the problem.

## Practical Examples

### Example 1

Round trip. A pipeline is saved and read back, and the restored language and stage count are printed.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  std::ostringstream out;
  nlp::io::write_workflow(out, wf);
  std::istringstream in(out.str());
  const nlp::Workflow restored = nlp::io::read_workflow(in);
  std::cout << "stages = " << restored.stages.size() << ", language = " << static_cast<int>(restored.lang) << "\n";
  return 0;
}
```

Output

```text
stages = 2, language = 1
```

### Example 2

A restored pipeline runs. The output of the restored workflow matches the original on the same text.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  std::ostringstream out;
  nlp::io::write_workflow(out, wf);
  std::istringstream in(out.str());
  const nlp::Workflow restored = nlp::io::read_workflow(in);
  std::cout << "same tokens = " << (restored.infer("Ada writes.").token_texts() == wf.infer("Ada writes.").token_texts()) << "\n";
  return 0;
}
```

Output

```text
same tokens = 1
```

### Example 3

A damaged file. The file is cut in half, so the checksum does not match and the read is refused; the program reports the refusal without printing the internal message.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  std::ostringstream out;
  nlp::io::write_workflow(out, wf);
  const std::string whole = out.str();
  std::istringstream cut(whole.substr(0, whole.size() / 2));
  try {
    nlp::io::read_workflow(cut);
    std::cout << "accepted\n";
  } catch (const std::exception&) {
    std::cout << "rejected\n";
  }
  return 0;
}
```

Output

```text
rejected
```

## See Also

- [`write_workflow`](write_workflow.md) -- produces the file this function reads.
- [`Introduction`](Introduction.md) -- the checks that protect the load.
