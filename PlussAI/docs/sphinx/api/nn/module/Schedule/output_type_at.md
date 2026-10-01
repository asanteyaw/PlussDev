<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b3f3bb9d114f -->

# `pluss::ai::nn::Schedule::output_type_at`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->output_type_at(index)`).

## Signature

```cpp
const std::type_info& nn::Schedule::Impl::output_type_at(std::size_t index) const;
```

## What It Does

The output counterpart to [`input_type_at`](input_type_at.md): reports what
type a given stage declared as its output when added. Checking the last
stage's declared output type is exactly what
[`forward`](forward.md)/[`run<ReturnType>`](run.md) do internally before
running the chain, so this is the accessor to reach for when diagnosing why
one of those calls rejects a `ReturnType`.

## Parameters

`std::size_t index`
: A position in `[0, size())`.

## Returns

`const std::type_info&` -- the type that stage declared as its output when
added. The final stage's `output_type_at` is what
[`forward`](forward.md)/[`run<ReturnType>`](run.md) must match `ReturnType`
against.

## Errors

- `index >= size()` -- throws.

## Practical Examples

Three examples: confirming an ordinary chain's stages all declare `Series`,
the adjacency invariant this accessor's own value participates in, and a
stage whose declared output is a tuple rather than `Series`.

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
    std::cout << "stage " << i << " output_type_at==typeid(Series): " << std::boolalpha
              << (net->output_type_at(i) == typeid(pluss::Series)) << "\n";
  }

  return 0;
}
```

Output

```text
stage 0 output_type_at==typeid(Series): true
stage 1 output_type_at==typeid(Series): true
```

### Example 2

`append`/`insert` enforce this exact equality every time a stage is added:
one stage's declared output type must equal the next stage's declared input
type.

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

An `RNN` stage's declared output is `std::tuple<Series, Series>`, not
`Series` -- the reason it cannot be added with plain [`append`](append.md).

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

  std::cout << "'rnn' output_type is Series: " << std::boolalpha
            << (net->output_type_at(0) == typeid(Series)) << "\n";
  std::cout << "'rnn' output_type is tuple<Series,Series>: "
            << (net->output_type_at(0) == typeid(std::tuple<Series, Series>)) << "\n";

  return 0;
}
```

Output

```text
'rnn' output_type is Series: false
'rnn' output_type is tuple<Series,Series>: true
```

## See Also

- [`input_type_at`](input_type_at.md) -- the paired accessor for a stage's
  input type.
- [`stage_info`](stage_info.md) -- both types together, plus the name and
  `module_stage` flag, as one struct.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="name_at.html">
    <span class="label">Previous</span>
    <span class="title">name_at</span>
  </a>
  <a class="next-link" href="reserve.html">
    <span class="label">Next</span>
    <span class="title">reserve</span>
  </a>
</div>
<!-- apiref:nav:end -->
