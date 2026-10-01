<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2275613c3563 -->

# `pluss::ai::nn::LSTM::options`

[Back to `pluss::ai::nn::LSTM`](../LSTM.md)

Only reachable via `->` (`net->options()`).

## Signature

```cpp
nn::LSTMOptions& nn::LSTM::options();
const nn::LSTMOptions& nn::LSTM::options() const;
```

## What It Does

Returns the [`LSTMOptions`](../LSTMOptions.md) this instance was
constructed with. The mutable overload allows reading back a field set at
construction; it does **not** retroactively resize or rebuild
[`params()`](params.md) if you mutate a geometry field (including
`proj_size`) through it after construction -- call `reset_parameters()`
yourself if the weight blocks need to match a changed option.

## Parameters

None.

## Returns

`LSTMOptions&` (or `const LSTMOptions&`) -- direct access to every field
this instance was constructed with; see [`LSTMOptions`](../LSTMOptions.md)
for what each one means.

## Errors

None.

## Practical Examples

Three examples: reading back the options a positional-constructor call
produced, including `proj_size`, confirming that mutating a geometry
field through the mutable overload does not retroactively resize
`params()`, and confirming the `LSTMOptions`-struct and positional
constructor forms produce identical options (including `proj_size`).

### Example 1

The positional constructor's arguments land exactly where you would
expect in `options()`, including `proj_size`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(4, 8, /*num_layers=*/2, /*bias=*/true, /*batch_first=*/true, /*dropout=*/0.0,
               /*bidirectional=*/false, /*split_bias=*/false, /*proj_size=*/3);

  std::cout << "input_size=" << net->options().input_size << "\n";
  std::cout << "hidden_size=" << net->options().hidden_size << "\n";
  std::cout << "num_layers=" << net->options().num_layers << "\n";
  std::cout << "proj_size=" << net->options().proj_size << "\n";
  std::cout << "batch_first=" << std::boolalpha << net->options().batch_first << "\n";

  return 0;
}
```

Output

```text
input_size=4
hidden_size=8
num_layers=2
proj_size=3
batch_first=true
```

### Example 2

Mutating `hidden_size` through `options()` after construction changes
what `options()` reports, but `params()` still holds the original,
now-mismatched shapes until `reset_parameters()` rebuilds them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(2, 4);
  std::cout << "before: options().hidden_size=" << net->options().hidden_size
            << ", params()[0].weight_hh cols=" << net->params()[0].weight_hh.sizes()[1] << "\n";

  net->options().hidden_size = 16;
  std::cout << "after mutating options() only: options().hidden_size=" << net->options().hidden_size
            << ", params()[0].weight_hh cols=" << net->params()[0].weight_hh.sizes()[1]
            << " -- unchanged, now mismatched\n";

  net->reset_parameters();
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

The `LSTMOptions`-struct constructor and the positional-argument
constructor are just two ways to reach the same state -- `options()`
reports identical values either way, including `proj_size`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTMOptions opts{};
  opts.input_size = 4;
  opts.hidden_size = 8;
  opts.num_layers = 2;
  opts.proj_size = 3;
  opts.batch_first = true;
  nn::LSTM via_options(opts);

  nn::LSTM via_positional(4, 8, /*num_layers=*/2, /*bias=*/true, /*batch_first=*/true,
                           /*dropout=*/0.0, /*bidirectional=*/false, /*split_bias=*/false,
                           /*proj_size=*/3);

  std::cout << "both constructors produce the same options:\n";
  std::cout << "  input_size: " << via_options->options().input_size << " == " << via_positional->options().input_size << "\n";
  std::cout << "  hidden_size: " << via_options->options().hidden_size << " == " << via_positional->options().hidden_size << "\n";
  std::cout << "  proj_size: " << via_options->options().proj_size << " == " << via_positional->options().proj_size << "\n";
  std::cout << "  batch_first: " << std::boolalpha << via_options->options().batch_first
            << " == " << via_positional->options().batch_first << "\n";

  return 0;
}
```

Output

```text
both constructors produce the same options:
  input_size: 4 == 4
  hidden_size: 8 == 8
  proj_size: 3 == 3
  batch_first: true == true
```

## See Also

- [`params`](params.md) -- the weight blocks whose shapes derive from
  these options, but are not automatically kept in sync with them.
- [`LSTMOptions`](../LSTMOptions.md) -- every field's own meaning.

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
