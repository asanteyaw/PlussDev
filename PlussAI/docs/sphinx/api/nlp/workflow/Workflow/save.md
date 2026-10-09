<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5566daa50182 -->

# `pluss::ai::nlp::Workflow::save`

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## Signature

```cpp
void pluss::ai::nlp::Workflow::save(std::string_view filepath) const;
```

## What It Does

Writes the workflow to a file: its language, its stage list (each by registry name), and every stage's state lines. A saved file restores a workflow whose `infer` results match the original, so a trained pipeline can be shipped and reloaded. The file uses the current schema version; older files still load. The call does not modify the workflow.

## Parameters

`std::string_view filepath`
: The destination file. It is created or overwritten.

## Returns

Nothing (`void`).

## Errors

Throws when the file cannot be written.

## Practical Examples

### Example 1

Saving and reloading a workflow keeps its stage count.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(), nlp::stage::lemmatize()}};
  const std::string path = "/tmp/plussai_doc_workflow_save_example1.bin";
  wf.save(path);
  const nlp::Workflow restored = nlp::Workflow::load(path);
  std::cout << "stages=" << restored.stages.size() << "\n";
  return 0;
}
```

Output

```text
stages=3
```

### Example 2

A reloaded workflow produces the same tokens as the original.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const std::string path = "/tmp/plussai_doc_workflow_save_example2.bin";
  wf.save(path);
  const nlp::Workflow restored = nlp::Workflow::load(path);
  const bool same = restored.infer(std::string_view("Laurel writes APIs.")).token_texts() == wf.infer(std::string_view("Laurel writes APIs.")).token_texts();
  std::cout << "same_tokens=" << same << "\n";
  return 0;
}
```

Output

```text
same_tokens=1
```

### Example 3

A trained classifier keeps its learned behavior across a save.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::textcat()}};
  wf.learn(std::vector<std::string>{"Compiler errors break builds", "C++ headers need cleanup", "Fresh bread and soup", "Bake a sweet cake"}, std::vector<std::string>{"tech", "tech", "food", "food"});
  const std::string path = "/tmp/plussai_doc_workflow_save_example3.bin";
  wf.save(path);
  const nlp::Workflow restored = nlp::Workflow::load(path);
  std::cout << restored.infer(std::string_view("Bread soup and dinner")).doc_label << "\n";
  return 0;
}
```

Output

```text
food
```

## See Also

- [`load`](load.md) -- reads the file back.
- [`io`](../../io.md) -- the file format in detail.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
