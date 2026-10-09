<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a9ad1969a887 -->

# `pluss::ai::Artifact::value`

[Back to `pluss::ai::Artifact`](../Artifact.md)

## Signature

```cpp
const Artifact::Value& value() const noexcept;
Artifact::Value& value() noexcept;
```

## What It Does

Returns the artifact's raw `std::variant` payload. Most callers use the
type-checked [`is`](is.md)/[`get`](get.md) pair instead; `value()` is for
code that genuinely needs the variant itself -- forwarding it unchanged
into a new `Artifact` under a different name, or switching on
`.index()`. The mutable overload lets a caller already holding a
non-`const` `Artifact` replace its payload in place.

## Parameters

None.

## Returns

A `const Artifact::Value&` (or `Artifact::Value&` on a non-`const`
`Artifact`).

## Errors

None.

## Practical Examples

### Example 1

Reading a scalar artifact's value through `std::get` directly on the
variant.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  std::cout << "value=" << std::get<std::int64_t>(a.value()) << "\n";
  return 0;
}
```

Output

```text
value=42
```

### Example 2

The mutable overload lets a caller replace the payload in place.

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::Artifact a = ai::procedure::input("answer", std::int64_t{42});
  a.value() = ai::Artifact::Value(std::int64_t{43});
  std::cout << "value=" << std::get<std::int64_t>(a.value()) << "\n";
  return 0;
}
```

Output

```text
value=43
```

### Example 3

A name-only (empty) artifact's `value()` holds `std::monostate`, index
`0` in the variant's alternative list.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Artifact a("placeholder");
  std::cout << "index=" << a.value().index() << "\n";
  return 0;
}
```

Output

```text
index=0
```

## See Also

- [`is`](is.md) / [`get`](get.md) -- the type-checked accessors most
  callers want instead.
- [`empty`](empty.md) -- whether the value is `std::monostate`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="named.html">
    <span class="label">Previous</span>
    <span class="title">named</span>
  </a>
</div>
<!-- apiref:nav:end -->
