<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d779e20c0c61 -->

# `pluss::ai::nn::Schedule::insert_as`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
template <typename In, typename Out, typename Layer>
nn::Schedule& nn::Schedule::insert_as(std::size_t index, Layer&& layer);

template <typename In, typename Out, typename Layer>
nn::Schedule& nn::Schedule::insert_as(std::size_t index, std::string name, Layer&& layer);
```

[`insert`](insert.md) with the stage's real input/output types stated
explicitly, the same relationship [`then_as`](then_as.md) has to
[`then`](then.md).

## Parameters

`typename In, typename Out` (explicit template arguments)
: The stage's declared input and output types -- see [`then_as`](then_as.md).

`std::size_t index`, `Layer&& layer`, `std::string name`
: Same as [`insert`](insert.md).

## Returns

`Schedule&` -- a reference to `*this`, for chaining.

## Errors

Same as [`insert`](insert.md): out-of-range `index`, a taken `name`, or a
declared type that does not satisfy one of the two neighbors the stage lands
between.

## Practical Examples

Three examples: the type-mismatch rejection (reused from
[`insert`](insert.md), since `insert`'s own default-typed overload delegates
to this one internally), a successful heterogeneous insert between two
compatible tuple-typed neighbors, and the explicit-name overload.

### Example 1

`insert`'s own `Series -> Series` overload is implemented in terms of this
one (`insert(i, name, layer)` calls `insert_as<Series, Series>(i, name,
layer)` internally) -- this is the same rejection shown on
[`insert`'s own page](insert.md#example-2), reached directly through
`insert_as` this time.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2));

  try {
    net.insert_as<Series, std::tuple<Series, Series>>(1, "bad", nn::RNN(4, 4));
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  std::cout << "size() is still " << net->size() << "\n";

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule type mismatch: new stage output NSt3__15tupleIJN5pluss5codem6SeriesES3_EEE is incompatible with next stage input N5pluss5codem6SeriesE
size() is still 2
```

### Example 2

Inserting a tuple-consuming stage between two stages that already agree on
the tuple type -- `insert_as` succeeds exactly like `then_as` would for the
equivalent append.

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
  struct ScaleHidden {
    std::tuple<Series, Series> forward(const std::tuple<Series, Series>& in) const {
      return {std::get<0>(in), std::get<1>(in).mul(0.5)};
    }
  };

  nn::Schedule net;
  net.then_as<Series, std::tuple<Series, Series>>("rnn", nn::RNN(3, 4));
  net.then_as<std::tuple<Series, Series>, Series>("take_output", TakeOutput{});

  net.insert_as<std::tuple<Series, Series>, std::tuple<Series, Series>>(1, "scale_hidden", ScaleHidden{});

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
stage 0: 'rnn'
stage 1: 'scale_hidden'
stage 2: 'take_output'
```

### Example 3

The auto-naming overload -- `insert_as<In, Out>(index, layer)` with no
explicit name, generating one the same way `then`/`insert` do.

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
  net.then_as<Series, std::tuple<Series, Series>>(nn::RNN(3, 4));       // auto-named "0"
  net.insert_as<std::tuple<Series, Series>, Series>(1, TakeOutput{});   // auto-named "1"

  std::cout << "stage 1 name='" << net->name_at(1) << "'\n";

  return 0;
}
```

Output

```text
stage 1 name='1'
```

## See Also

- [`insert`](insert.md) -- the `Series -> Series` shorthand this generalizes.
- [`then_as`](then_as.md) -- the equivalent append-only operation.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="insert.html">
    <span class="label">Previous</span>
    <span class="title">insert</span>
  </a>
  <a class="next-link" href="module_at.html">
    <span class="label">Next</span>
    <span class="title">module_at</span>
  </a>
</div>
<!-- apiref:nav:end -->
