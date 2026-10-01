<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 390afc8ba7fc -->

# `pluss::ai::nn::Schedule::contains`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->contains(name)`).

## Signature

```cpp
bool nn::Schedule::Impl::contains(std::string_view name) const noexcept;
```

## What It Does

Checks whether a stage name is currently registered on this `Schedule`,
without throwing on a miss -- the safe way to probe for a name before
[`index_of`](index_of.md) (which throws if the name is not found) or before
adding a stage under a name that must be unique.

## Parameters

`std::string_view name`
: The stage name to look for -- exact match against what
  [`append`](append.md)/[`insert`](insert.md) registered it under, or the
  auto-generated name if none was given.

## Returns

`bool`. Never throws, even for a name that has never existed.

## Errors

None.

## Practical Examples

Three examples: a present and an absent name, the throwing counterpart
([`index_of`](index_of.md)) for contrast, and confirming `append` rejects a
name `contains` already reports as taken.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));

  std::cout << "contains('in')=" << std::boolalpha << net->contains("in") << "\n";
  std::cout << "contains('out')=" << net->contains("out") << "\n";

  return 0;
}
```

Output

```text
contains('in')=true
contains('out')=false
```

### Example 2

[`index_of`](index_of.md) is the throwing counterpart to `contains` -- reach
for `contains` first when a missing name is an expected possibility rather
than a bug.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));

  std::cout << "index_of('out')=" << net->index_of("out") << "\n";

  try {
    net->index_of("missing");
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
index_of('out')=1
caught: pluss::nn::Schedule::index_of: unknown stage name 'missing'
```

### Example 3

`append`'s own uniqueness check (see its Errors section) is exactly
`contains(name)` under the hood -- a name `contains` reports as taken is
always rejected by `append`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("a", nn::Fusion(3, 4));
  std::cout << "contains('a') before retry: " << std::boolalpha << net->contains("a") << "\n";

  try {
    net.append("a", nn::Fusion(4, 2));
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
contains('a') before retry: true
caught: pluss::nn::Schedule duplicate stage name 'a'
```

## See Also

- [`index_of`](index_of.md) -- the throwing form, also returning the
  position.
- [`name_at`](name_at.md) -- the inverse lookup, position to name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear.html">
    <span class="label">Previous</span>
    <span class="title">clear</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
<!-- apiref:nav:end -->
