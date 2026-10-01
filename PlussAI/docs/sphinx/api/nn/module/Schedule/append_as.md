<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e94c99402f53 -->

# `pluss::ai::nn::Schedule::append_as`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
template <typename In, typename Out, typename Layer>
nn::Schedule& nn::Schedule::append_as(Layer&& layer);

template <typename In, typename Out, typename Layer>
nn::Schedule& nn::Schedule::append_as(std::string name, Layer&& layer);
```

## What It Does

`append_as<In, Out>` is [`append`](append.md) with the stage's real input/output
types stated explicitly, instead of assumed to be `Series -> Series`. `In`
and `Out` can be any type the stage's `forward`/`operator()` actually
produces -- most usefully `std::tuple<Series, Series>` (`RNN`/`GRU`'s own
`forward` return type) or a deeper nested tuple (`LSTM`'s). Appends the
stage to the end of the chain the same way `append` does, after checking its
declared types against whatever the chain currently ends with.

## Parameters

`typename In, typename Out` (explicit template arguments)
: The stage's declared input and output types. Checked against `layer`'s
  actual callable signature at compile time (a `static_assert` fails if
  `layer` cannot actually be invoked as `In -> Out`), and against
  neighboring stages' declared types at run time, the same way `append` does.

`Layer&& layer`, `std::string name`
: Same as [`append`](append.md) -- a callable exposing `forward(In) -> Out` or
  `operator()(In) -> Out`, and an optional unique stage name.

## Returns

`Schedule&` -- a reference to `*this`, for chaining.

## Errors

Same as [`append`](append.md): a taken `name`, or a declared type that does not
match a neighboring stage's declared type.

## Practical Examples

Three examples building one heterogeneous pipeline in stages: attaching an
`RNN` (whose `forward` returns a tuple) as a stage, attaching a follow-up
stage that consumes that tuple, and running the completed pipeline.

### Example 1

`RNN::forward` returns `std::tuple<Series, Series>` (output, final hidden
state), not a plain `Series` -- [`append`](append.md) alone (which assumes
`Series -> Series`) cannot register it. `append_as<In, Out>` states the real
stage type explicitly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append_as<pluss::Series, std::tuple<pluss::Series, pluss::Series>>(nn::RNN(3, 4));

  std::cout << "stage 0 output_type == typeid(tuple<Series,Series>): "
            << std::boolalpha
            << (net->output_type_at(0) == typeid(std::tuple<pluss::Series, pluss::Series>)) << "\n";

  return 0;
}
```

Output

```text
stage 0 output_type == typeid(tuple<Series,Series>): true
```

### Example 2

A plain callable that consumes the tuple and keeps only the sequence
output, discarding the final hidden state -- registered with the
explicit-name overload, immediately after the `RNN` stage from Example 1.

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
  net.append_as<Series, std::tuple<Series, Series>>("rnn", nn::RNN(3, 4));
  net.append_as<std::tuple<Series, Series>, Series>("take_output", TakeOutput{});

  std::cout << "stage 1 name='" << net->name_at(1) << "'\n";
  std::cout << "final chain output_type == typeid(Series): " << std::boolalpha
            << (net->output_type_at(1) == typeid(Series)) << "\n";

  return 0;
}
```

Output

```text
stage 1 name='take_output'
final chain output_type == typeid(Series): true
```

### Example 3

The completed two-stage pipeline, run end to end on a real batch --
`run<Series>` (see [`run`](run.md)) drives it exactly like `forward` would
if the chain started and ended in plain `Series`.

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
  net.append_as<Series, std::tuple<Series, Series>>("rnn", nn::RNN(3, 4));
  net.append_as<std::tuple<Series, Series>, Series>("take_output", TakeOutput{});

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);

  // seq_len=2, batch=1, input_size=3 (batch_first=false is RNN's default)
  const Series x = pluss::make_series({1.0, 0.5, -0.5, 0.2, -0.3, 0.1}, {2, 1, 3}, f64);
  const Series y = net.run<Series>(x);

  std::cout << "pipeline output shape: " << y.sizes()[0] << "x" << y.sizes()[1] << "x" << y.sizes()[2] << "\n";

  return 0;
}
```

Output

```text
pipeline output shape: 2x1x4
```

## See Also

- [`append`](append.md) -- the `Series -> Series` shorthand this generalizes.
- [`run`](run.md) -- drives a heterogeneous chain end to end.
- [`insert_as`](insert_as.md) -- the same idea at a specific position
  instead of the end.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append.html">
    <span class="label">Previous</span>
    <span class="title">append</span>
  </a>
  <a class="next-link" href="clear.html">
    <span class="label">Next</span>
    <span class="title">clear</span>
  </a>
</div>
<!-- apiref:nav:end -->
