<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 40ebe76ce4f0 -->

# `pluss::ai::Artifact::empty`

[Back to `pluss::ai::Artifact`](../Artifact.md)

## Signature

```cpp
bool empty() const noexcept;
```

## What It Does

Reports whether the artifact's `Value` currently holds `std::monostate`
-- the variant's "no value" alternative. A default-constructed `Artifact`
and a named-only `Artifact(std::string name)` are both `empty()`; any
`Artifact` built with an explicit value (via the two-argument constructor
or [`procedure::input`](../../build/input.md)) is not.

## Parameters

None.

## Returns

`true` if the artifact holds no value, `false` otherwise.

## Errors

None.

## Practical Examples

### Example 1

A typed artifact built via `procedure::input` is never empty.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "empty=" << a.empty() << "\n";
  return 0;
}
```

Output

```text
empty=0
```

### Example 2

A name-only artifact (what a `SinkSpec`-backed step returns when it has
nothing to pass through) is empty.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a("placeholder");
  std::cout << "empty=" << a.empty() << "\n";
  return 0;
}
```

Output

```text
empty=1
```

### Example 3

A default-constructed artifact is also empty.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a{};
  std::cout << "empty=" << a.empty() << "\n";
  return 0;
}
```

Output

```text
empty=1
```

## See Also

- [`value`](value.md) / [`is`](is.md) -- inspecting what the value
  actually is, once it's known not to be empty.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
<!-- apiref:nav:end -->
