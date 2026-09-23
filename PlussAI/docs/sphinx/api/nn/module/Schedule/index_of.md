<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c608114cda17 -->

# `pluss::ai::nn::Schedule::index_of`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->index_of(name)`).

## Signature

```cpp
std::size_t nn::Schedule::Impl::index_of(std::string_view name) const;
```

## Parameters

`std::string_view name`
: The stage name to look up.

## Returns

`std::size_t` -- the stage's position, in `[0, size())`.

## Errors

- `name` does not match any current stage -- throws. Use
  [`contains`](contains.md) first when a missing name is expected, not a
  bug.

## Practical Examples

Three examples: a successful lookup, the not-found error, and using the
result together with [`name_at`](name_at.md) as an inverse round trip.

### Example 1

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

### Example 2

`contains` never throws for a name that does not exist; `index_of` always
does. Reach for whichever matches whether a miss is expected.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));

  std::cout << "contains('missing')=" << std::boolalpha << net->contains("missing") << " (no throw)\n";

  try {
    net->index_of("missing");
  } catch (const std::exception& e) {
    std::cout << "index_of('missing') caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
contains('missing')=false (no throw)
index_of('missing') caught: pluss::nn::Schedule::index_of: unknown stage name 'missing'
```

### Example 3

`index_of` and `name_at` are inverses -- feeding one's result to the other
returns the original name.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2));

  // index_of and name_at are inverses of each other.
  const std::size_t i = net->index_of("out");
  const std::string& round_trip = net->name_at(i);
  std::cout << "index_of('out')=" << i << ", name_at(" << i << ")='" << round_trip << "'\n";

  return 0;
}
```

Output

```text
index_of('out')=1, name_at(1)='out'
```

## See Also

- [`contains`](contains.md) -- the non-throwing existence check.
- [`name_at`](name_at.md) -- the inverse lookup, position to name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward.html">
    <span class="label">Previous</span>
    <span class="title">forward</span>
  </a>
  <a class="next-link" href="input_type_at.html">
    <span class="label">Next</span>
    <span class="title">input_type_at</span>
  </a>
</div>
<!-- apiref:nav:end -->
