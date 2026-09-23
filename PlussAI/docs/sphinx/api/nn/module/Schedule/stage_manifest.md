<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dbed88bd2728 -->

# `pluss::ai::nn::Schedule::stage_manifest`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->stage_manifest()`).

## Signature

```cpp
std::vector<Schedule::Impl::StageInfo> nn::Schedule::Impl::stage_manifest() const;
```

## Parameters

None.

## Returns

`std::vector<StageInfo>` -- [`stage_info(i)`](stage_info.md) for every
`i` in `[0, size())`, in order. Equivalent to calling `stage_info` in a
loop, bundled into one call for logging or printing a whole chain's shape
at once.

## Errors

None -- an empty `Schedule` returns an empty vector rather than throwing.

## Practical Examples

Three examples: the full manifest of an ordinary chain, printing just the
`module_stage` flag across every stage, and the empty-schedule case.

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

  const auto manifest = net->stage_manifest();
  for (const auto& info : manifest) {
    std::cout << "'" << info.name << "' module_stage=" << std::boolalpha << info.module_stage << "\n";
  }

  return 0;
}
```

Output

```text
'in' module_stage=true
'out' module_stage=true
```

### Example 2

A mixed chain -- a plain lambda stage next to a real layer -- shows
`module_stage` distinguishing them across the whole manifest at once.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  auto double_it = [](const pluss::Series& x) { return x.mul(2.0); };

  nn::Schedule net;
  net.then("scale", double_it);    // not an nn::Module
  net.then("head", nn::Fusion(3, 2));

  for (const auto& info : net->stage_manifest()) {
    std::cout << "'" << info.name << "' module_stage=" << std::boolalpha << info.module_stage << "\n";
  }

  return 0;
}
```

Output

```text
'scale' module_stage=false
'head' module_stage=true
```

### Example 3

An empty `Schedule`'s manifest is an empty vector, not an error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  std::cout << "stage_manifest().size()=" << net->stage_manifest().size() << "\n";

  return 0;
}
```

Output

```text
stage_manifest().size()=0
```

## See Also

- [`stage_info`](stage_info.md) -- the single-stage form this calls in a
  loop.
- [`size`](size.md) -- the manifest's length before calling it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stage_info.html">
    <span class="label">Previous</span>
    <span class="title">stage_info</span>
  </a>
  <a class="next-link" href="then.html">
    <span class="label">Next</span>
    <span class="title">then</span>
  </a>
</div>
<!-- apiref:nav:end -->
