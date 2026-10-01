<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eb6465d78046 -->

# `pluss::ai::nn::Schedule::erase`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->erase(index)`) -- `Schedule` does not
re-declare it, unlike `append`/`insert`/`extend`/`run`.

## Signature

```cpp
void nn::Schedule::Impl::erase(std::size_t index);
```

## What It Does

Removes one stage from the chain by position and shifts every later stage
left to close the gap, rebuilding the module-registration tree so
`parameters()`/`buffers()`/`modules()` reflect the removal immediately. It
does **not** check whether the two stages now sitting next to each other
are still type-compatible -- see Example 2 for what that gap means in
practice, and prefer rebuilding the `Schedule` from scratch when removing a
stage would break the surrounding chain's types.

## Parameters

`std::size_t index`
: The position to remove, in `[0, size())`.

## Returns

`void`. Removes the stage, shifts every later stage left by one, and
rebuilds the module-registration tree (`parameters()`/`buffers()`/`modules()`
reflect the removal immediately) -- but does **not** re-validate that the
stages now adjacent to each other are still type-compatible. See Example 2.

## Errors

- `index >= size()` -- throws.

## Practical Examples

Three examples: removing a stage and confirming the parameter count drops
with it, the chain-validation gap `erase` leaves behind, and the
out-of-range error.

### Example 1

Erasing the middle stage of a three-stage chain -- `size()` and
`parameters().size()` both drop, and the remaining stages close the gap.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("mid", nn::Fusion(4, 4));
  net.append("out", nn::Fusion(4, 2));

  std::cout << "before: size()=" << net->size() << ", parameters().size()=" << net.parameters().size() << "\n";

  net->erase(1);  // remove "mid"

  std::cout << "after erasing 'mid': size()=" << net->size() << ", parameters().size()=" << net.parameters().size() << "\n";
  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "  stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
before: size()=3, parameters().size()=6
after erasing 'mid': size()=2, parameters().size()=4
  stage 0: 'in'
  stage 1: 'out'
```

### Example 2

`erase` never re-checks the chain. Removing the `RNN` stage that "bridged"
a `Series`-only stage into a tuple-consuming one leaves a chain that looks
fine (`erase` itself succeeds) but fails the next time it actually runs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  struct TakeOutput {
    Series forward(const std::tuple<Series, Series>& in) const { return std::get<0>(in); }
  };

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append_as<Series, std::tuple<Series, Series>>("rnn", nn::RNN(4, 4));
  net.append_as<std::tuple<Series, Series>, Series>("take_output", TakeOutput{});

  // erase() does NOT re-validate the surrounding chain -- it only rebuilds
  // module records. Removing "rnn" leaves "in" (Series->Series) feeding
  // directly into "take_output" (tuple<Series,Series>->Series), a type
  // mismatch erase() lets through silently.
  net->erase(1);
  std::cout << "erase() succeeded -- size()=" << net->size() << "\n";

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  const Series x = pluss::make_series({1.0, 0.5, -0.5}, {1, 3}, f64);

  try {
    net.forward(x);
    std::cout << "(forward did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "forward() caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
erase() succeeded -- size()=2
forward() caught: pluss::nn::Schedule::run: stage 'take_output' expected input type NSt3__15tupleIJN5pluss5codem6SeriesES3_EEE but got a different runtime type
```

### Example 3

Erasing past the end of the chain throws instead of doing nothing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("only", nn::Fusion(3, 2));

  try {
    net->erase(5);  // out of range -- size() is 1
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule::erase index out of range
```

## See Also

- [`append`](append.md) / [`insert`](insert.md) -- the operations `erase`
  reverses.
- [`clear`](clear.md) -- erases every stage at once.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="extend.html">
    <span class="label">Next</span>
    <span class="title">extend</span>
  </a>
</div>
<!-- apiref:nav:end -->
