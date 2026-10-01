<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 166ed6f1d764 -->

# `pluss::ai::nn::RNN::options`

[Back to `pluss::ai::nn::RNN`](../RNN.md)

Only reachable via `->` (`net->options()`).

## Signature

```cpp
nn::RNNOptions& nn::RNN::options();
const nn::RNNOptions& nn::RNN::options() const;
```

## What It Does

Returns the [`RNNOptions`](../RNNOptions.md) this instance was
constructed with -- the same values [`forward`](forward.md) reads to
decide the layout convention, nonlinearity, and bias layout, and
[`params()`](params.md)'s block shapes were derived from at construction
time. The mutable overload allows *reading back* a field that was set at
construction (`num_layers`, `bidirectional`, and so on); it does **not**
retroactively resize or otherwise rebuild `params()` if you mutate a
geometry field through it after construction -- call
`reset_parameters()` yourself if the weight blocks need to match a
changed option.

## Parameters

None.

## Returns

`RNNOptions&` (or `const RNNOptions&`) -- direct access to every field
this instance was constructed with; see [`RNNOptions`](../RNNOptions.md)
for what each one means.

## Errors

None.

## Practical Examples

Three examples: reading back the options a positional-constructor call
produced, confirming that mutating a geometry field through the mutable
overload does not retroactively resize `params()`, and confirming the
`RNNOptions`-struct and positional constructor forms produce identical
options.

### Example 1

The positional constructor's arguments land exactly where you would
expect in `options()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(4, 8, /*num_layers=*/2, "relu", /*bias=*/true, /*batch_first=*/true);

  std::cout << "input_size=" << net->options().input_size << "\n";
  std::cout << "hidden_size=" << net->options().hidden_size << "\n";
  std::cout << "num_layers=" << net->options().num_layers << "\n";
  std::cout << "nonlinearity=" << net->options().nonlinearity << "\n";
  std::cout << "batch_first=" << std::boolalpha << net->options().batch_first << "\n";

  return 0;
}
```

Output

```text
input_size=4
hidden_size=8
num_layers=2
nonlinearity=relu
batch_first=true
```

### Example 2

Mutating `hidden_size` through `options()` after construction changes
what `options()` reports, but `params()` still holds the original,
now-mismatched shapes -- `reset_parameters()` is what actually rebuilds
the weight blocks to match.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(2, 4);
  std::cout << "before: options().hidden_size=" << net->options().hidden_size
            << ", params()[0].weight_hh cols=" << net->params()[0].weight_hh.sizes()[1] << "\n";

  net->options().hidden_size = 16;  // changes options(), not params()
  std::cout << "after mutating options() only: options().hidden_size=" << net->options().hidden_size
            << ", params()[0].weight_hh cols=" << net->params()[0].weight_hh.sizes()[1]
            << " -- unchanged, now mismatched\n";

  net->reset_parameters();  // rebuilds params() to match the new hidden_size
  std::cout << "after reset_parameters(): params()[0].weight_hh cols="
            << net->params()[0].weight_hh.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
before: options().hidden_size=4, params()[0].weight_hh cols=4
after mutating options() only: options().hidden_size=16, params()[0].weight_hh cols=4 -- unchanged, now mismatched
after reset_parameters(): params()[0].weight_hh cols=16
```

### Example 3

The `RNNOptions`-struct constructor and the positional-argument
constructor are just two ways to reach the same state -- `options()`
reports identical values either way.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNNOptions opts{};
  opts.input_size = 4;
  opts.hidden_size = 8;
  opts.num_layers = 2;
  opts.nonlinearity = "relu";
  opts.batch_first = true;
  nn::RNN via_options(opts);

  nn::RNN via_positional(4, 8, 2, "relu", /*bias=*/true, /*batch_first=*/true);

  std::cout << "both constructors produce the same options:\n";
  std::cout << "  input_size: " << via_options->options().input_size << " == " << via_positional->options().input_size << "\n";
  std::cout << "  nonlinearity: " << via_options->options().nonlinearity << " == " << via_positional->options().nonlinearity << "\n";
  std::cout << "  batch_first: " << std::boolalpha << via_options->options().batch_first
            << " == " << via_positional->options().batch_first << "\n";

  return 0;
}
```

Output

```text
both constructors produce the same options:
  input_size: 4 == 4
  nonlinearity: relu == relu
  batch_first: true == true
```

## See Also

- [`params`](params.md) -- the weight blocks whose shapes derive from
  these options, but are not automatically kept in sync with them.
- [`RNNOptions`](../RNNOptions.md) -- every field's own meaning.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward.html">
    <span class="label">Previous</span>
    <span class="title">forward</span>
  </a>
  <a class="next-link" href="params.html">
    <span class="label">Next</span>
    <span class="title">params</span>
  </a>
</div>
<!-- apiref:nav:end -->
