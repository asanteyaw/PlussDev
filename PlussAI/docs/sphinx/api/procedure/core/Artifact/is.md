<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9e02be09f8c4 -->

# `pluss::ai::Artifact::is`

[Back to `pluss::ai::Artifact`](../Artifact.md)

## Signature

```cpp
template <typename T>
bool is() const noexcept;
```

## What It Does

Reports whether the artifact currently holds a `T`. Every adapter
runner's dispatch logic -- a [`lymph`](../../lymph.md) step deciding
whether its input is a `LabeledBatch` or a `Payload`, for instance --
is built on chains of `is<T>()` checks before calling `get<T>()`.

## Parameters

`T` (template parameter)
: The type to test for.

## Returns

`true` or `false`.

## Errors

None -- never throws, regardless of `T`.

## Practical Examples

### Example 1

A scalar artifact reports `true` for the type it was built with.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "is_int64=" << a.is<std::int64_t>() << "\n";
  return 0;
}
```

Output

```text
is_int64=1
```

### Example 2

The same artifact reports `false` for any other type.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "is_double=" << a.is<double>() << "\n";
  return 0;
}
```

Output

```text
is_double=0
```

### Example 3

A `Series` artifact reports `true` for `Series`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);
  const ai::Artifact a = ai::procedure::input("x", x);

  std::cout << "is_series=" << a.is<pluss::Series>() << "\n";
  return 0;
}
```

Output

```text
is_series=1
```

## See Also

- [`get`](get.md) -- fetch the value once its type is known.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="name.html">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>
<!-- apiref:nav:end -->
