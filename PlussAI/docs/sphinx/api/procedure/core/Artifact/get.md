<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 61cdaa66e36e -->

# `pluss::ai::Artifact::get`

[Back to `pluss::ai::Artifact`](../Artifact.md)

## Signature

```cpp
template <typename T>
const T& get() const;
```

## What It Does

Returns the artifact's value as `T`, throwing if the variant does not
currently hold a `T`. This is the type-checked counterpart to
[`value()`](value.md): most call sites that know what type they expect --
reading a [`ProcedureResult`](../ProcedureResult.md) artifact by name, or
a runner reading one of its own inputs -- use `get<T>()` rather than
inspecting the raw variant.

## Parameters

`T` (template parameter)
: The expected type. Must be one of `Artifact::Value`'s listed
  alternatives.

## Returns

A `const T&` into the artifact's own storage.

## Errors

- Throws if the artifact does not currently hold a `T`.

## Practical Examples

### Example 1

Reading back a scalar artifact as the type it was built with.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "value=" << a.get<std::int64_t>() << "\n";
  return 0;
}
```

Output

```text
value=42
```

### Example 2

Reading a `Series` artifact back and calling a real `Series` method on
the result.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);
  const ai::Artifact a = ai::procedure::input("x", x);

  std::cout << "elements=" << a.get<pluss::Series>().numel() << "\n";
  return 0;
}
```

Output

```text
elements=2
```

### Example 3

Asking for the wrong type throws rather than returning a default value.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  bool threw = false;
  try {
    a.get<double>();
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

## See Also

- [`is`](is.md) -- check the type first without risking a throw.
- [`value`](value.md) -- the raw, untyped variant.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="is.html">
    <span class="label">Next</span>
    <span class="title">is</span>
  </a>
</div>
<!-- apiref:nav:end -->
