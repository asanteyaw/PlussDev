<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 40d7277d3812 -->

# `pluss::ai::nn::Schedule::name_at`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->name_at(index)`).

## Signature

```cpp
const std::string& nn::Schedule::Impl::name_at(std::size_t index) const;
```

## Parameters

`std::size_t index`
: A position in `[0, size())`.

## Returns

`const std::string&` -- the stage's name at that position, whether it was
given explicitly or auto-generated.

## Errors

- `index >= size()` -- throws (the same `check_index_` guard every
  position-based accessor on this page shares).

## Practical Examples

Three examples: reading names in position order for an explicitly-named
chain, the round trip with [`index_of`](index_of.md), and how
auto-generated names read once a stage has been inserted out of call order.

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

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
stage 0: 'in'
stage 1: 'out'
```

### Example 2

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

### Example 3

Auto-generated names reflect **call order**, not final position (see the
category [Introduction](../Introduction.md)) -- reading them by position
after an `insert` shows the counter's value at each stage's own moment of
creation, not a clean sequence.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then(nn::Fusion(3, 4));   // auto-named "0"
  net.then(nn::Fusion(4, 2));   // auto-named "1"
  net.insert(1, nn::Fusion(4, 4));  // auto-named "2"

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": name_at(" << i << ")='" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
stage 0: name_at(0)='0'
stage 1: name_at(1)='2'
stage 2: name_at(2)='1'
```

## See Also

- [`index_of`](index_of.md) -- the inverse lookup, name to position.
- [`contains`](contains.md) -- checks existence without needing a position.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_of.html">
    <span class="label">Previous</span>
    <span class="title">index_of</span>
  </a>
  <a class="next-link" href="insert.html">
    <span class="label">Next</span>
    <span class="title">insert</span>
  </a>
</div>
<!-- apiref:nav:end -->
