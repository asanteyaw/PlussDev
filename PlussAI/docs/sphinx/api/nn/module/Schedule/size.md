<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a148bcf49171 -->

# `pluss::ai::nn::Schedule::size`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->size()`).

## Signature

```cpp
std::size_t nn::Schedule::Impl::size() const noexcept;
```

## Parameters

None.

## Returns

`std::size_t` -- the number of stages currently in the chain, in
`[0, SIZE_MAX]`. Never throws.

## Errors

None.

## Practical Examples

Three examples: growth as stages are added, `reserve` alone never changing
it, and building a `Schedule` through the variadic constructor.

### Example 1

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

### Example 2

[`reserve`](reserve.md) is a capacity hint only -- it never adds a stage,
so `size()` stays `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

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

### Example 3

The variadic `Schedule(Layers&&...)` constructor calls `reserve` once, then
`then`s each argument -- `size()` reflects the full count immediately after
construction.

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

- [`empty`](empty.md) -- `size() == 0`.
- [`reserve`](reserve.md) -- a capacity hint, never changes `size()` alone.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
  <a class="next-link" href="stage_info.html">
    <span class="label">Next</span>
    <span class="title">stage_info</span>
  </a>
</div>
<!-- apiref:nav:end -->
