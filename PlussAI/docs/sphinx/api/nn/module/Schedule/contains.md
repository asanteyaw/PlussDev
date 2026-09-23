<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 390afc8ba7fc -->

# `pluss::ai::nn::Schedule::contains`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->contains(name)`).

## Signature

```cpp
bool nn::Schedule::Impl::contains(std::string_view name) const noexcept;
```

## Parameters

`std::string_view name`
: The stage name to look for -- exact match against what
  [`then`](then.md)/[`insert`](insert.md) registered it under, or the
  auto-generated name if none was given.

## Returns

`bool`. Never throws, even for a name that has never existed.

## Errors

None.

## Practical Examples

Three examples: a present and an absent name, the throwing counterpart
([`index_of`](index_of.md)) for contrast, and confirming `then` rejects a
name `contains` already reports as taken.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));

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
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2));

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

`then`'s own uniqueness check (see its Errors section) is exactly
`contains(name)` under the hood -- a name `contains` reports as taken is
always rejected by `then`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("a", nn::Fusion(3, 4));
  std::cout << "contains('a') before retry: " << std::boolalpha << net->contains("a") << "\n";

  try {
    net.then("a", nn::Fusion(4, 2));
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
