<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f0f718ae90e3 -->

# `pluss::ai::nn::Schedule::forward`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
Series nn::Schedule::forward(const Series& input) const;
Series nn::Schedule::operator()(const Series& input) const;
```

`operator()` is exactly `forward` -- a `Schedule` is callable directly, the
same convention every layer in this reference follows. Both are
`Series -> Series` shorthand for [`run<Series>(input)`](run.md); reach for
`run` directly when the chain's real input or output type is not a plain
`Series`.

## Parameters

`const Series& input`
: Fed to the first stage. Must match that stage's declared input type
  (`Series`, for every chain `forward`/`operator()` can run -- a chain whose
  first stage declares a different input type is only reachable through
  [`run`](run.md)).

## Returns

`Series` -- the last stage's output, after `input` has passed through every
stage in order.

## Errors

- The `Schedule` has no stages -- throws.
- Any stage's actual runtime input does not match its declared type (see
  [`run`](run.md)'s Errors) -- throws.
- The final stage's declared output type is not `Series` -- throws (use
  [`run<ReturnType>`](run.md) for a chain that ends in something else).

## Practical Examples

Three examples: a basic two-stage run, confirming `operator()` and
`forward` agree, and the empty-schedule error.

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

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  const pluss::Series y = net.forward(x);
  std::cout << "forward(x) shape: " << y.sizes()[0] << "x" << y.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
forward(x) shape: 1x2
```

### Example 2

`net(x)` and `net.forward(x)` produce identical values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  // operator() is exactly forward() -- Schedule is callable directly.
  const pluss::Series via_forward = net.forward(x);
  const pluss::Series via_call = net(x);

  std::cout << "same values: " << std::boolalpha
            << via_forward.allclose(via_call) << "\n";

  return 0;
}
```

Output

```text
same values: true
```

### Example 3

An empty `Schedule` has no first stage to feed -- `forward` throws rather
than returning the input unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule empty_net;  // no stages added

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3});

  try {
    empty_net.forward(x);
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule::run: cannot run an empty schedule
```

## See Also

- [`run`](run.md) -- the general form, for chains with a different input
  count or return type.
- [`then`](then.md) -- builds the chain `forward` runs.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extend.html">
    <span class="label">Previous</span>
    <span class="title">extend</span>
  </a>
  <a class="next-link" href="index_of.html">
    <span class="label">Next</span>
    <span class="title">index_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
