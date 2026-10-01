# `pluss::ai::nn::Embedding`

A learned lookup table: one row per category code, gathered by index.
`weight` has shape `[cardinality, hidden_size]`; `forward` takes integer
category codes of any shape and returns that same shape with `hidden_size`
appended as a trailing axis. This is the standard way a categorical
feature (a product ID, a token, a day-of-week code) becomes a dense vector
a network can actually compute with, instead of the network having to work
with the raw integer code directly.

## Signature

```cpp
explicit nn::Embedding::Embedding(nn::EmbeddingOptions options);

void nn::Embedding::reset_parameters();
```

## Construction

`Embedding` has exactly one constructor, and it requires an
[`EmbeddingOptions`](EmbeddingOptions.md) -- there is no no-argument
overload, because `cardinality`/`hidden_size` have no meaningful default
(`weight` cannot be allocated without them). At minimum, set both:

```cpp
nn::EmbeddingOptions opts{};
opts.cardinality = 1000;  // 1000 distinct categories
opts.hidden_size = 16;    // each category becomes a 16-dim vector
nn::Embedding emb(opts);
```

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned lookup table, shape `[cardinality, hidden_size]`. Registered
  as a parameter. Row `i` is category `i`'s embedding vector.

`options()` (mutable and `const` overloads) returns the
[`EmbeddingOptions`](EmbeddingOptions.md) this instance was constructed
with.

## Methods

`forward(index)` / `operator()(index)` -- `Series`
: Returns `weight().index_select(0, index.reshape({index.numel()}).to(Int64))`,
  reshaped back to `index`'s own shape with `hidden_size` appended. `index`
  may have any shape (a scalar, a flat batch, a `[batch, seq_len]` matrix
  of token codes) -- every element is looked up independently. Throws if
  any index falls outside `[0, cardinality)`.

`reset_parameters()` -- `void`
: Re-fills `weight()` in place with a fresh uniform draw in
  `[-1/sqrt(hidden_size), 1/sqrt(hidden_size)]`, using `options().seed`.
  Called once during construction; call it again to reinitialize an
  already-trained table. Never reassigns `weight()` itself, so registration
  and gradient tracking are undisturbed across any number of calls.

## Practical Examples

Five examples: a basic lookup verified row-by-row against `weight`, a
multi-dimensional index shape, confirming the initialization bound and
`reset_parameters()`'s reproducibility, the out-of-range error, and
confirming `Embedding` runs on the MPS backend.

### Example 1

Looking up three category codes and confirming each output row matches
the corresponding row of `weight` directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::EmbeddingOptions opts{};
  opts.cardinality = 5;
  opts.hidden_size = 3;
  opts.seed = 42;
  nn::Embedding emb(opts);

  std::cout << "weight shape: [" << emb->weight().sizes()[0] << ", " << emb->weight().sizes()[1] << "]\n";

  const pluss::Series index = pluss::make_series({0.0, 2.0, 4.0}, {3}).to(pluss::ScalarType::Int64);
  const pluss::Series y = emb.forward(index);

  std::cout << "index shape: [3], output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << "]\n";
  std::cout << "row 0 of output equals row 0 of weight: " << std::boolalpha
            << y.select(0, 0).allclose(emb->weight().select(0, 0)) << "\n";
  std::cout << "row 1 of output equals row 2 of weight: "
            << y.select(0, 1).allclose(emb->weight().select(0, 2)) << "\n";

  return 0;
}
```

Output

```text
weight shape: [5, 3]
index shape: [3], output shape: [3, 3]
row 0 of output equals row 0 of weight: true
row 1 of output equals row 2 of weight: true
```

### Example 2

A `[batch, seq_len]` index -- the conventional shape for a batch of
token/category sequences -- comes back as `[batch, seq_len, hidden_size]`.
`forward` never flattens the *result*, only the lookup itself; the
original shape (plus the trailing embedding axis) is always restored.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::EmbeddingOptions opts{};
  opts.cardinality = 10;
  opts.hidden_size = 4;
  opts.seed = 7;
  nn::Embedding emb(opts);

  // batch=2, seq_len=3 category codes
  const pluss::Series index = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3}).to(pluss::ScalarType::Int64);
  const pluss::Series y = emb.forward(index);

  std::cout << "index shape: [" << index.sizes()[0] << ", " << index.sizes()[1] << "]\n";
  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2]
            << "] -- index's own shape, plus hidden_size appended\n";

  return 0;
}
```

Output

```text
index shape: [2, 3]
output shape: [2, 3, 4] -- index's own shape, plus hidden_size appended
```

### Example 3

The initial (and every `reset_parameters()`-reset) weight is drawn
uniformly from `[-1/sqrt(hidden_size), 1/sqrt(hidden_size)]`, and the same
`seed` always reproduces the exact same values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::EmbeddingOptions opts{};
  opts.cardinality = 100;
  opts.hidden_size = 16;
  opts.seed = 3;
  nn::Embedding emb(opts);

  const double bound = 1.0 / std::sqrt(static_cast<double>(opts.hidden_size));
  const pluss::Series w = emb->weight();
  const pluss::Series flat = w.reshape({w.numel()});
  std::cout << "bound = 1/sqrt(hidden_size) = " << bound << "\n";
  std::cout << "min >= -bound: " << std::boolalpha << (flat.amin(0, false).item<double>() >= -bound) << "\n";
  std::cout << "max <= bound: " << (flat.amax(0, false).item<double>() <= bound) << "\n";

  const pluss::Series before = w.clone();
  emb->reset_parameters();  // same seed -- re-fills with the exact same values
  std::cout << "reset_parameters() with the same seed reproduces the same weights: "
            << before.allclose(emb->weight()) << "\n";

  return 0;
}
```

Output

```text
bound = 1/sqrt(hidden_size) = 0.25
min >= -bound: true
max <= bound: true
reset_parameters() with the same seed reproduces the same weights: true
```

### Example 4

An index outside `[0, cardinality)` throws cleanly rather than reading out
of bounds.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::EmbeddingOptions opts{};
  opts.cardinality = 5;
  opts.hidden_size = 3;
  nn::Embedding emb(opts);

  const pluss::Series index = pluss::make_series({0.0, 10.0}, {2}).to(pluss::ScalarType::Int64);
  try {
    const pluss::Series y = emb.forward(index);
    std::cout << "(did not throw) " << y << "\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: gather: index out of range
```

### Example 5

`Embedding` runs on the MPS backend the same way it does on CPU -- moving
the module with `.to(pMPS)` moves `weight`, and the index tensor must be
moved to the same platform before calling `forward`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::EmbeddingOptions opts{};
  opts.cardinality = 5;
  opts.hidden_size = 3;
  nn::Embedding emb(opts);
  emb.to(pluss::pMPS);

  std::cout << "weight platform after .to(pMPS): " << static_cast<int>(emb->weight().platform()) << "\n";

  const Series index = pluss::make_series({0.0, 1.0}, {2}).to(pluss::ScalarType::Int64).to(pluss::pMPS);
  try {
    const Series y = emb.forward(index);
    std::cout << "embedding lookup on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "embedding lookup on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight platform after .to(pMPS): 2
embedding lookup on MPS: OK, platform=2
```

## Changelog

- **0.1** — the learned embedding table, `cardinality`/`hidden_size`/`cfg`/`seed`
  options.

## See Also

- [`EmbeddingOptions`](EmbeddingOptions.md) -- the options struct.
- [`LayerNorm`](../normalization/LayerNorm.md) -- another parameterized
  layer with no no-argument constructor, for comparison of how a required
  shape argument shows up in the options struct.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

EmbeddingOptions
```
<!-- apiref:toctree:end -->
