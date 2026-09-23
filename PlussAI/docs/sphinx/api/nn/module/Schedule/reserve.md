<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 03a896b58aca -->

# `pluss::ai::nn::Schedule::reserve`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->reserve(count)`).

## Signature

```cpp
void nn::Schedule::Impl::reserve(std::size_t count);
```

## Parameters

`std::size_t count`
: A capacity hint for the internal stage vector -- `std::vector::reserve`
  underneath. Purely an allocation optimization; has no effect on
  `size()`/`empty()`/lookups.

## Returns

`void`. Never throws.

## Errors

None.

## Practical Examples

Three examples: `reserve` never adding a stage, a fresh `Schedule` for
contrast, and the variadic constructor, which calls `reserve` internally
before adding its arguments.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // reserve() is purely a capacity hint for the internal stage vector --
  // it never adds a stage. empty() stays true until the first then()/insert().
  nn::Schedule net;
  net->reserve(8);
  std::cout << "after reserve(8): size()=" << net->size() << " empty()=" << std::boolalpha << net->empty() << "\n";

  return 0;
}
```

Output

```text
after reserve(8): size()=0 empty()=true
```

### Example 2

A fresh `Schedule`, with no `reserve` call at all, starts in the identical
observable state.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  std::cout << "fresh Schedule: size()=" << net->size() << " empty()=" << std::boolalpha << net->empty() << "\n";

  net.then(nn::Fusion(3, 2));
  std::cout << "after one then(): size()=" << net->size() << " empty()=" << net->empty() << "\n";

  return 0;
}
```

Output

```text
fresh Schedule: size()=0 empty()=true
after one then(): size()=1 empty()=false
```

### Example 3

The variadic `Schedule(Layers&&...)` constructor calls
`reserve(sizeof...(layers))` once up front, then `then()`s each argument in
order -- an alternative to building a `Schedule` with repeated `then()`
calls.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net(nn::Fusion(3, 4), nn::Fusion(4, 2));

  std::cout << "size()=" << net->size() << "\n";
  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "  stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
size()=2
  stage 0: '0'
  stage 1: '1'
```

## See Also

- [`size`](size.md) / [`empty`](empty.md) -- unaffected by `reserve` alone.
- [`then`](then.md) -- what actually adds a stage.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="output_type_at.html">
    <span class="label">Previous</span>
    <span class="title">output_type_at</span>
  </a>
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
