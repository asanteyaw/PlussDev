# `pluss::ai::nlp::runtime::EmbeddingOptions`

The configuration of an [`EmbeddingTable`](EmbeddingTable.md): how wide each vector is, whether positions
are added to the token vectors, how the position vectors are produced, and the numeric type and seed the
table is initialized with. The struct is plain data built with designated initializers.

## Declaration

```cpp
struct pluss::ai::nlp::runtime::EmbeddingOptions {
  std::int64_t width = 128;
  bool use_positions = true;
  bool learned_positions = true;
  std::int64_t max_positions = 4096;
  pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};
```

## Fields

### `width`

`std::int64_t`, default `128`. The length of every vector the table produces. It must be positive. It is
also the input width an encoder expects, so the encoder's model dimension has to match it.

### `use_positions`

`bool`, default `true`. When `true`, a position vector is added to each token vector, so the same word at
two places in a sequence gets two different representations. Turn it off for bag-of-words style models that
should ignore order.

### `learned_positions`

`bool`, default `true`. Chooses how position vectors are made. Learned positions start random and train
with the model. Sinusoidal positions are fixed functions of the position, so they need no training and
extend to any position the table was sized for.

### `max_positions`

`std::int64_t`, default `4096`. The longest sequence the table can position. A batch longer than this is
rejected when it reaches the table, rather than silently reusing position vectors. It must be positive when
positions are used.

### `cfg`

`pluss::Config`, default single precision on the CPU. The numeric type and device of the table's weights.
Choose a different precision or device here, and the table's tensors follow it.

### `seed`

`std::uint64_t`, default `0`. The seed for the random initialization. Two tables with the same seed,
vocabulary size, and width start with the same weights, which makes an initialization reproducible.

## Guidance

Set `width` to the model dimension of the encoder that will read the table, and set `max_positions` to the
longest batch the application will see. The defaults are sized for general use; a small model with short
inputs can use a much smaller width and position budget to save memory.

Keep the seed and the options fixed across the training and inference runs of the same model. The state of a
saved table records them, so a restored table reproduces the original's vectors.

## See Also

- [`EmbeddingTable`](EmbeddingTable.md) -- uses these options.
- [`TransformerTextEncoder`](TransformerTextEncoder.md) -- its model dimension must match `width`.

## Practical Examples

These examples show the options' effect on the table's shape, the positions, and the reproducibility of the
initialization.

### Example 1

A table with the default width and the shape it gives a batch.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const pluss::Series out = table.forward(batch);
  std::cout << "shape = " << out.sizes()[0] << " x " << out.sizes()[1] << " x " << out.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
shape = 2 x 2 x 128
```

### Example 2

Positions on and off. Without positions the table has no position weights, and the program prints that
distinction.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  for (const bool positions : {true, false}) {
    nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4, .use_positions = positions}};
    table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
    std::cout << "use_positions = " << positions << ", position weights defined = " << table.position_weights().defined() << "\n";
  }
  return 0;
}
```

Output

```text
use_positions = 1, position weights defined = 1
use_positions = 0, position weights defined = 0
```

### Example 3

Reproducible initialization. Two tables with the same seed start with identical weights, and a different seed
gives different ones; the program compares the first weight of each.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  const std::int64_t vocab = static_cast<std::int64_t>(indexer.vocabulary().size());
  nlp::runtime::EmbeddingTable a{nlp::runtime::EmbeddingOptions{.width = 4, .seed = 7}};
  nlp::runtime::EmbeddingTable b{nlp::runtime::EmbeddingOptions{.width = 4, .seed = 7}};
  nlp::runtime::EmbeddingTable c{nlp::runtime::EmbeddingOptions{.width = 4, .seed = 8}};
  a.reset(vocab, indexer.pad_id());
  b.reset(vocab, indexer.pad_id());
  c.reset(vocab, indexer.pad_id());
  const double first_a = a.weights().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>()[4];
  const double first_b = b.weights().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>()[4];
  const double first_c = c.weights().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>()[4];
  std::cout << "same seed equal = " << (first_a == first_b) << ", other seed equal = " << (first_a == first_c) << "\n";
  return 0;
}
```

Output

```text
same seed equal = 1, other seed equal = 0
```

## Changelog

- **0.1** -- initial documented surface.
