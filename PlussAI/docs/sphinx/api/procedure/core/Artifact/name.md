<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 82eabda5e7ba -->

# `pluss::ai::Artifact::name`

[Back to `pluss::ai::Artifact`](../Artifact.md)

## Signature

```cpp
const std::string& name() const noexcept;
```

## What It Does

Returns the artifact's name -- what [`ProcedureResult::get`](../ProcedureResult/get.md)
matches by, and what [`Link`](../Link.md)'s `from_output`/`to_input`
address.

## Parameters

None.

## Returns

A `const std::string&`.

## Errors

None.

## Practical Examples

### Example 1

An artifact built via `procedure::input` carries the name it was given.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "name=" << a.name() << "\n";
  return 0;
}
```

Output

```text
name=answer
```

### Example 2

A name-only artifact's name is whatever was passed to the one-argument
constructor.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a("unnamed_slot");
  std::cout << "name=" << a.name() << "\n";
  return 0;
}
```

Output

```text
name=unnamed_slot
```

### Example 3

A `Series` artifact's name is independent of its value's own type.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);
  const ai::Artifact a = ai::procedure::input("x", x);

  std::cout << "name=" << a.name() << "\n";
  return 0;
}
```

Output

```text
name=x
```

## See Also

- [`named`](named.md) -- whether the name is non-empty.
- [`ProcedureResult::get`](../ProcedureResult/get.md) -- looking an
  artifact up by this name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is.html">
    <span class="label">Previous</span>
    <span class="title">is</span>
  </a>
  <a class="next-link" href="named.html">
    <span class="label">Next</span>
    <span class="title">named</span>
  </a>
</div>
<!-- apiref:nav:end -->
