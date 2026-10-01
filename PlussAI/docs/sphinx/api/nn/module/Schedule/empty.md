<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ffd8fd43f76b -->

# `pluss::ai::nn::Schedule::empty`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->empty()`).

## Signature

```cpp
bool nn::Schedule::Impl::empty() const noexcept;
```

## What It Does

A convenience spelling of `size() == 0` -- useful before calling
[`forward`](forward.md)/[`run`](run.md), which both throw on a `Schedule`
with no stages, or simply for a clearer condition than comparing `size()`
to `0` directly.

## Parameters

None.

## Returns

`bool` -- exactly `size() == 0`. Never throws.

## Errors

None.

## Practical Examples

Three examples: a fresh `Schedule`, one after a stage is added, and
confirming `reserve` alone never clears it.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  std::cout << "fresh Schedule: size()=" << net->size() << " empty()=" << std::boolalpha << net->empty() << "\n";

  net.append(nn::Fusion(3, 2));
  std::cout << "after one append(): size()=" << net->size() << " empty()=" << net->empty() << "\n";

  return 0;
}
```

Output

```text
fresh Schedule: size()=0 empty()=true
after one append(): size()=1 empty()=false
```

### Example 2

`reserve` never adds a stage, so `empty()` stays `true`.

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

[`clear`](clear.md) empties a populated `Schedule` back to `empty() == true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));
  std::cout << "before: size()=" << net->size() << "\n";

  net.clear();
  std::cout << "after clear(): size()=" << net->size() << " empty()=" << std::boolalpha << net->empty() << "\n";

  return 0;
}
```

Output

```text
before: size()=2
after clear(): size()=0 empty()=true
```

## See Also

- [`size`](size.md) -- the count `empty` is derived from.
- [`clear`](clear.md) -- the operation that reliably makes `empty()` true.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="erase.html">
    <span class="label">Next</span>
    <span class="title">erase</span>
  </a>
</div>
<!-- apiref:nav:end -->
