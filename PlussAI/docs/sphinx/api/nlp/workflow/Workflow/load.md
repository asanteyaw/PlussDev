<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e3b1470fa3a0 -->

# `pluss::ai::nlp::Workflow::load`

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## Signature

```cpp
static pluss::ai::nlp::Workflow pluss::ai::nlp::Workflow::load(std::string_view filepath);
```

## What It Does

Reads a workflow saved by `save` and returns it, rebuilding each stage through the stage registry and restoring its state. Use it to reload a trained pipeline in a fresh process. Every stage named in the file must be known to the registry, which holds the built-in stages and any custom ones registered by the program.

## Parameters

`std::string_view filepath`
: A file written by `save`.

## Returns

The reconstructed `Workflow`.

## Errors

Throws when the file is missing, truncated, corrupted, or uses a schema version this build cannot read, and when it names a stage the registry does not know.

## Practical Examples

### Example 1

Loading a file written by `save` returns the saved stage list.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const std::string path = "/tmp/plussai_doc_workflow_load_example1.bin";
  wf.save(path);
  const nlp::Workflow loaded = nlp::Workflow::load(path);
  std::cout << "stages=" << loaded.stages.size() << "\n";
  return 0;
}
```

Output

```text
stages=2
```

### Example 2

A missing file is reported as an error rather than an empty workflow.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  bool threw = false;
  try {
    nlp::Workflow::load("/tmp/plussai_doc_no_such_workflow.bin");
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

### Example 3

A reloaded classifier classifies new text the way the original did.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::textcat()}};
  wf.learn(std::vector<std::string>{"Compiler errors break builds", "C++ headers need cleanup", "Fresh bread and soup", "Bake a sweet cake"}, std::vector<std::string>{"tech", "tech", "food", "food"});
  const std::string path = "/tmp/plussai_doc_workflow_load_example3.bin";
  wf.save(path);
  const nlp::Workflow loaded = nlp::Workflow::load(path);
  std::cout << loaded.infer(std::string_view("Headers and compiler warnings")).doc_label << "\n";
  return 0;
}
```

Output

```text
tech
```

## See Also

- [`save`](save.md) -- writes the file this reads.
- [`registry`](../../registry.md) -- how a custom stage is found by name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
