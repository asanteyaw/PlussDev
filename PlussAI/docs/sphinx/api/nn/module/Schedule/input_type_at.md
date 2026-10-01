<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 736a2e876c4f -->

# `pluss::ai::nn::Schedule::input_type_at`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->input_type_at(index)`).

## Signature

```cpp
const std::type_info& nn::Schedule::Impl::input_type_at(std::size_t index) const;
```

## What It Does

Exposes the type-checking machinery `append`/`insert`'s neighbor checks are
built on: reports what type a given stage declared as its input at the
time it was added. Useful for diagnosing a `Schedule` built elsewhere (by
another function, or programmatically) without reading the code that built
it -- inspect the declared types directly instead.

## Parameters

`std::size_t index`
: A position in `[0, size())`.

## Returns

`const std::type_info&` -- the type that stage declared as its input when
added (via [`append`](append.md)'s implicit `Series`, or
[`append_as`](append_as.md)'s explicit `In`). Compare with `==`/`!=` against
`typeid(SomeType)`; `.name()` gives a raw, platform-specific mangled string,
not meant to be read directly.

## Errors

- `index >= size()` -- throws.

## Practical Examples

Three examples: confirming every stage in an ordinary chain declares
`Series`, the adjacency invariant `append`/`insert` themselves enforce, and a
stage whose declared type is not `Series` at all.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << " input_type_at==typeid(Series): " << std::boolalpha
              << (net->input_type_at(i) == typeid(pluss::Series)) << "\n";
  }

  return 0;
}
```

Output

```text
stage 0 input_type_at==typeid(Series): true
stage 1 input_type_at==typeid(Series): true
```

### Example 2

This is the exact invariant `append`/`insert` enforce every time a stage is
added: one stage's declared output type must equal the next stage's
declared input type.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));

  for (std::size_t i = 0; i + 1 < net->size(); ++i) {
    std::cout << "output_type_at(" << i << ") == input_type_at(" << (i + 1) << "): " << std::boolalpha
              << (net->output_type_at(i) == net->input_type_at(i + 1)) << "\n";
  }

  return 0;
}
```

Output

```text
output_type_at(0) == input_type_at(1): true
```

### Example 3

An `RNN` stage, registered via [`append_as`](append_as.md), declares its input
as plain `Series` even though its **output** is a tuple -- only `Out` needs
to be stated explicitly when `In` is still `Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Schedule net;
  net.append_as<Series, std::tuple<Series, Series>>("rnn", nn::RNN(3, 4));

  std::cout << "'rnn' input_type is Series: " << std::boolalpha
            << (net->input_type_at(0) == typeid(Series)) << "\n";

  return 0;
}
```

Output

```text
'rnn' input_type is Series: true
```

## See Also

- [`output_type_at`](output_type_at.md) -- the paired accessor for a
  stage's output type.
- [`stage_info`](stage_info.md) -- both types together, plus the name and
  `module_stage` flag, as one struct.

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
