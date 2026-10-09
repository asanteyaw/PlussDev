<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69b17623b9d1 -->
# `pluss::ai::Step::save_component`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
void save_component(std::string_view path) const;
```

## What It Does

Invokes `persist().save(path)` if the step has a `Saver` closure
([`can_save_component()`](can_save_component.md) is `true`); otherwise
does nothing. [`Procedure::save`](../Procedure/save.md) calls this once
per persistable step, passing each step's own component directory inside
the bundle.

## Parameters

`std::string_view path`
: Where to write the component's artifact. The exact format (a file, a
  directory, several sidecar files) is entirely up to the `Saver` the
  wrapped engine adapter supplied.

## Returns

Nothing (`void`).

## Errors

- Propagates whatever the underlying `Saver` closure itself throws (for
  example a filesystem write failure); the call is otherwise a silent
  no-op when there is no `Saver` to invoke.

## Practical Examples

### Example 1

A hand-attached `Saver` writes a marker file at the given path.

```cpp
#include <pluss/ai/procedure.h>

#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.save = [](std::string_view path) {
    std::ofstream out(std::string(path) + ".marker");
    out << "saved";
  };
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  const std::string path = "/tmp/plussai_doc_step_save_component_example1";
  std::filesystem::remove(path + ".marker");
  s.save_component(path);
  std::cout << "marker_exists=" << std::filesystem::exists(path + ".marker") << "\n";
  std::filesystem::remove(path + ".marker");
  return 0;
}
```

Output

```text
marker_exists=1
```

### Example 2

A sink step has no `Saver`, so calling `save_component` is a harmless
no-op.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  s.save_component("/tmp/plussai_doc_step_save_component_example2");
  std::cout << "no_throw=1\n";
  return 0;
}
```

Output

```text
no_throw=1
```

### Example 3

The exact content a `Saver` writes is entirely up to the closure -- here
it is read back to confirm.

```cpp
#include <pluss/ai/procedure.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.save = [](std::string_view path) {
    std::ofstream out(std::string(path) + ".marker");
    out << "saved";
  };
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  const std::string path = "/tmp/plussai_doc_step_save_component_example3";
  std::filesystem::remove(path + ".marker");
  s.save_component(path);
  std::ifstream in(path + ".marker");
  const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::cout << "content=" << content << "\n";
  std::filesystem::remove(path + ".marker");
  return 0;
}
```

Output

```text
content=saved
```

## See Also

- [`can_save_component`](can_save_component.md) -- checking first
  whether there is anything to save.
- [`Procedure::save`](../Procedure/save.md) -- the caller that invokes
  this once per persistable step.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="runnable.html">
    <span class="label">Previous</span>
    <span class="title">runnable</span>
  </a>
  <a class="next-link" href="trained.html">
    <span class="label">Next</span>
    <span class="title">trained</span>
  </a>
</div>
<!-- apiref:nav:end -->
