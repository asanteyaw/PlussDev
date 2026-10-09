<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::io::write_workflow`

## Signature

```cpp
void pluss::ai::nlp::io::write_workflow(std::ostream& out, const pluss::ai::nlp::Workflow& workflow);
```

## What It Does

Writes `workflow` to `out` in the current saved-workflow format. Every stage's state is included, so the file is complete on its own: [`read_workflow`](read_workflow.md) restores the same pipeline from it without any other input. The workflow itself is not changed.

## Parameters

`out` is the destination stream, such as an `std::ofstream` or an `std::ostringstream`.

`workflow` is the pipeline to save. Its stages must be registered names, or the file cannot be read back.

## Returns

Nothing.

## Errors


## Practical Examples

### Example 1

Saving a pipeline to a string and counting the lines the file occupies, which shows the header and one line per entry.

```cpp
#include <pluss/nlp.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  std::ostringstream out;
  nlp::io::write_workflow(out, wf);
  std::cout << "lines written = " << std::count(out.str().begin(), out.str().end(), '\n') << "\n";
  return 0;
}
```

Output

```text
lines written = 36
```

### Example 2

Saving a pipeline with a trained stage. The file grows with the learned state, which the program shows by comparing sizes with an untrained pipeline.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow plain{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Workflow trained{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::textcat()}};
  trained.learn(std::vector<std::string>{"Bake bread.", "Write docs."}, std::vector<std::string>{"food", "tech"}, nlp::TrainOptions{});
  std::ostringstream small, large;
  nlp::io::write_workflow(small, plain);
  nlp::io::write_workflow(large, trained);
  std::cout << "larger with training = " << (large.str().size() > small.str().size()) << "\n";
  return 0;
}
```

Output

```text
larger with training = 1
```

### Example 3

Checking that the saved text carries the schema line, which every file starts with.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::French, .stages = {nlp::stage::tokenize()}};
  std::ostringstream out;
  nlp::io::write_workflow(out, wf);
  std::cout << "header starts with schema = " << (out.str().find("schema=") != std::string::npos) << "\n";
  return 0;
}
```

Output

```text
header starts with schema = 1
```

## See Also

- [`read_workflow`](read_workflow.md) -- restores what this function writes.
- [`Introduction`](Introduction.md) -- the file's contents.
