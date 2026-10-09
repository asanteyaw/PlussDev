<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 600fe2aa6ffd -->

# `pluss::ai::Artifact::named`

[Back to `pluss::ai::Artifact`](../Artifact.md)

## Signature

```cpp
bool named() const noexcept;
```

## What It Does

Reports whether `name()` is non-empty. [`Step::run`](../Step/run.md)
checks this on a runner's result: an unnamed `Artifact` returned from a
runner gets a default name assigned (the step's own single declared
output, or the step's own name) before it is published into the artifact
pool.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

An artifact built via `procedure::input` is always named.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "named=" << a.named() << "\n";
  return 0;
}
```

Output

```text
named=1
```

### Example 2

A default-constructed artifact has an empty name, so it is not named.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a{};
  std::cout << "named=" << a.named() << "\n";
  return 0;
}
```

Output

```text
named=0
```

### Example 3

A name-only artifact is named even though it carries no value.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a("placeholder");
  std::cout << "named=" << a.named() << "\n";
  return 0;
}
```

Output

```text
named=1
```

## See Also

- [`name`](name.md) -- the name string itself.
- [`Step::run`](../Step/run.md) -- where an unnamed result gets a
  default name assigned.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="name.html">
    <span class="label">Previous</span>
    <span class="title">name</span>
  </a>
  <a class="next-link" href="value.html">
    <span class="label">Next</span>
    <span class="title">value</span>
  </a>
</div>
<!-- apiref:nav:end -->
