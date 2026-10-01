<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b31ec27ee69 -->

# `pluss::ai::nn::Schedule::run`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
template <typename ReturnType = Series, class... InputTypes>
ReturnType nn::Schedule::run(InputTypes&&... inputs) const;
```

## What It Does

Runs the chain the same way [`forward`](forward.md) does -- each stage's
output feeds the next -- but as the general form behind it: explicit about
the expected return type instead of assuming `Series`, and able to feed
more than one argument to the first stage at once (packed into a tuple).
Reach for `run` instead of `forward`/`operator()` whenever the chain's real
input or output type is not a plain `Series`, most commonly a chain built
with [`append_as`](append_as.md)/[`insert_as`](insert_as.md) stages.

## Parameters

`typename ReturnType` (explicit or defaulted to `Series`)
: The type the *last* stage must declare as its output. Checked against the
  actual runtime type the chain produces after every stage has run.

`InputTypes&&... inputs`
: One argument: passed to the first stage as-is (its declared input type
  must match). More than one argument: packed into a single
  `std::tuple<std::decay_t<InputTypes>...>`, which becomes the first
  stage's expected input type -- see Example 2.

## Returns

`ReturnType` -- the last stage's output, `any_cast` to `ReturnType`.

## Errors

- No stages -- throws.
- The packed input's type does not match the first stage's declared input
  type, or any later stage's actual runtime input does not match its own
  declared input type -- throws, naming the offending stage.
- The chain's actual final output type does not match the requested
  `ReturnType` -- throws.

## Practical Examples

Three examples: the single-input form (identical to `forward`), packing two
inputs into a tuple for the first stage, and requesting a `ReturnType` the
chain does not actually produce.

### Example 1

`run<Series>(x)` with one input is exactly what `forward(x)` calls.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  // forward(x) is exactly run<Series>(x).
  const pluss::Series y = net.run<pluss::Series>(x);
  std::cout << "run<Series>(x) shape: " << y.sizes()[0] << "x" << y.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
run<Series>(x) shape: 1x2
```

### Example 2

Passing two arguments packs them into `std::tuple<Series, Series>` as the
first stage's input -- here, a stage that combines them by addition before
the chain continues with an ordinary `Series -> Series` layer.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  // Passing more than one argument to run() packs them into a
  // std::tuple<...> as the first stage's input type -- here, a stage that
  // combines two Series by addition.
  struct Combine {
    Series forward(const std::tuple<Series, Series>& in) const {
      return std::get<0>(in).add(std::get<1>(in));
    }
  };

  nn::Schedule net;
  net.append_as<std::tuple<Series, Series>, Series>("combine", Combine{});
  net.append("head", nn::Fusion(4, 2));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  const Series a = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 4}, f64);
  const Series b = pluss::make_series({0.5, 0.5, 0.5, 0.5}, {1, 4}, f64);

  const Series y = net.run<Series>(a, b);  // two arguments -> combine sees std::tuple<Series, Series>
  std::cout << "run<Series>(a, b) shape: " << y.sizes()[0] << "x" << y.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
run<Series>(a, b) shape: 1x2
```

### Example 3

Requesting `std::tuple<Series, Series>` from a chain that actually produces
a plain `Series` -- every stage runs (the mismatch is only known once the
chain finishes), and the final `any_cast` throws.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Schedule net;
  net.append("only", nn::Fusion(3, 2));  // produces a plain Series

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  const Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  try {
    // Requesting a tuple return from a chain that produces a plain Series.
    auto y = net.run<std::tuple<Series, Series>>(x);
    (void)y;
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule::run: requested return type NSt3__15tupleIJN5pluss5codem6SeriesES3_EEE but chain produced N5pluss5codem6SeriesE
```

## See Also

- [`forward`](forward.md) -- the `Series -> Series`, single-input shorthand.
- [`append_as`](append_as.md) -- builds the heterogeneous chains `run` is meant
  to drive.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reserve.html">
    <span class="label">Previous</span>
    <span class="title">reserve</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
