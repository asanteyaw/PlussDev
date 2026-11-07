# `pluss::loader::DataLoaderOptions`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

## What It Does

`DataLoaderOptions` configures everything about how a `DataLoader` turns a dataset into batches. The fields fall into five groups: how samples are grouped into batches (`batch_size`, `drop_last`, `batch_dim`), how they are ordered (`shuffle`, `seed`, `reshuffle_each_epoch`, `sampler`), what happens to a batch before it is delivered (`target_platform`, `target_dtype`, `collate_fn`), background computation (`num_workers`, `prefetch_factor`, `persistent_workers`, `pin_memory`, `timeout`, `in_order`, `worker_init_fn`), and `batch_dim` doubles as which axis of the input fields holds the samples.

`sampler` and `shuffle` are mutually exclusive: setting both is refused. `prefetch_factor` and `persistent_workers` require `num_workers > 0`; setting either without workers is refused. `pin_memory` is accepted and applied, but is a documented no-op today, because the underlying tensor engine's own pin operation does not yet change a Series's pinned state.

## Fields

| Field | Meaning | Default |
|---|---|---|
| **Batching** | | |
| `batch_size` | Samples per batch. | `32` |
| `drop_last` | Whether a shorter final batch is dropped instead of delivered. | `false` |
| `batch_dim` | Which axis of the input fields holds the samples. | `0` |
| **Ordering** | | |
| `shuffle` | Whether samples are visited in a shuffled order. Mutually exclusive with `sampler`. | `false` |
| `seed` | The shuffle seed; combined with the epoch to vary per epoch. | `0` |
| `reshuffle_each_epoch` | Whether `begin()` advances to the next epoch and reshuffles automatically. | `true` |
| `sampler` | A custom [`Sampler`](loader-sampler.md) controlling visit order, instead of plain `shuffle`. | none |
| **Delivery** | | |
| `target_platform` | Platform every delivered batch is moved to, if set. | unset (source platform kept) |
| `target_dtype` | Scalar type every delivered batch is converted to, if set. | unset (source dtype kept) |
| `collate_fn` | A function that receives each assembled batch and may transform it before delivery. | none (batch delivered as assembled) |
| **Background workers** | | |
| `num_workers` | Background worker thread count; `0` runs synchronously on the calling thread. | `0` |
| `prefetch_factor` | Batches to compute ahead per worker. Requires `num_workers > 0`. | unset |
| `persistent_workers` | Whether worker threads stay alive between epochs instead of being joined and respawned. Requires `num_workers > 0`. | `false` |
| `pin_memory` | Requests pinned-memory batches. Currently a documented no-op. | `false` |
| `timeout` | Seconds to wait for a worker-produced batch before giving up. `0` waits indefinitely. | `0.0` |
| `in_order` | Whether batches must arrive in strict index order, versus as soon as a worker finishes. | `true` |
| `worker_init_fn` | Called once per worker thread, with that worker's index, when workers start. | none |

## Errors

A non-positive `batch_size`, `shuffle` and `sampler` set together, or `prefetch_factor`/`persistent_workers` set without `num_workers`, are each refused when the loader is built or reconfigured.

## Practical Examples

### Example 1

Example 1 sets batch_size and reads the resulting batch plan.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;

  // Seven samples with two fields. Field x holds a row of two values per sample;
  // field y holds one label per sample. Both fields share the same batch dimension.
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, {7, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0, 1, 0}, {7}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  loader::DataLoader batches_source({x, y}, options);

  std::cout << "samples: " << batches_source.sample_count() << '\n';
  std::cout << "batches: " << batches_source.batch_count() << '\n';

  // Sequential order visits samples 0..6 in turn. The last batch is smaller.
  for (std::size_t i = 0; i < batches_source.batch_count(); ++i) {
    const auto batch = batches_source.batch_at(i);
    std::cout << "batch " << i << " rows: " << batch.x().sizes()[0]
              << ", label sum: " << batch.y().sum().item<double>() << '\n';
  }
}
```

Output

```text
samples: 7
batches: 3
batch 0 rows: 3, label sum: 1
batch 1 rows: 3, label sum: 2
batch 2 rows: 1, label sum: 0
```

Seven samples in batches of three give three batches, the last one smaller.

### Example 2

Example 2 compares drop_last on and off.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6}, {7}, cfg);

  // Keeping the partial batch: seven samples in batches of three give 3, 3, 1.
  loader::DataLoaderOptions keep{};
  keep.batch_size = 3;
  keep.drop_last = false;
  loader::DataLoader kept({x}, keep);

  // Dropping it: the one leftover sample is never delivered.
  loader::DataLoaderOptions drop{};
  drop.batch_size = 3;
  drop.drop_last = true;
  loader::DataLoader dropped({x}, drop);

  std::cout << "kept batches: " << kept.batch_count() << '\n';
  std::cout << "dropped batches: " << dropped.batch_count() << '\n';
  std::cout << "last kept batch rows: " << kept.batch_at(kept.batch_count() - 1).x().sizes()[0] << '\n';
}
```

Output

```text
kept batches: 3
dropped batches: 2
last kept batch rows: 1
```

Keeping the partial batch adds one more batch than dropping it, and the kept one is smaller.

### Example 3

Example 3 shuffles with a seed, and shows the shuffle is reproducible per epoch.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// The order of samples for one full pass, read through the batches the loader delivers.
std::vector<double> one_pass(pluss::loader::DataLoader& loader) {
  std::vector<double> order;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).pull<double>();
    order.insert(order.end(), values.begin(), values.end());
  }
  return order;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, {8}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.shuffle = true;
  options.seed = 42;

  // Two loaders built from the same seed produce the same first pass.
  loader::DataLoader first(std::vector<pluss::Series>{x}, options);
  loader::DataLoader second(std::vector<pluss::Series>{x}, options);
  const auto first_order = one_pass(first);
  std::cout << "same seed, same order: " << (first_order == one_pass(second)) << '\n';

  // A shuffled pass still visits every sample exactly once.
  auto sorted = first_order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every sample once: " << (sorted == std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}) << '\n';

  // Moving to a new epoch reshuffles by default. Returning to epoch zero replays it.
  first.set_epoch(1);
  const auto epoch_one = one_pass(first);
  std::cout << "epoch one differs from epoch zero: " << (epoch_one != first_order) << '\n';
  first.set_epoch(0);
  std::cout << "epoch zero replays: " << (one_pass(first) == first_order) << '\n';
}
```

Output

```text
same seed, same order: true
every sample once: true
epoch one differs from epoch zero: true
epoch zero replays: true
```

The same seed gives the same order, every sample is visited once, and a later epoch shuffles differently but epoch zero can be replayed.

### Example 4

Example 4 sets a custom sampler, and shows it is rejected together with shuffle.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void print_values(const std::vector<double>& values) {
  for (std::size_t i = 0; i < values.size(); ++i) std::cout << (i ? ", " : "") << values[i];
}

std::vector<double> visited(pluss::loader::DataLoader& loader) {
  std::vector<double> out;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).pull<double>();
    out.insert(out.end(), values.begin(), values.end());
  }
  return out;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, {8}, cfg);

  // A sequential sampler visits every sample in index order.
  loader::DataLoaderOptions sequential{};
  sequential.batch_size = 4;
  sequential.sampler = std::make_shared<loader::SequentialSampler>();
  loader::DataLoader in_order({x}, sequential);
  std::cout << "sequential: ";
  print_values(visited(in_order));
  std::cout << '\n';

  // A subset sampler visits only the listed indices, in a random order.
  loader::DataLoaderOptions subset{};
  subset.batch_size = 2;
  subset.sampler = std::make_shared<loader::SubsetRandomSampler>(std::vector<std::int64_t>{4, 5, 6});
  loader::DataLoader only_subset({x}, subset);
  auto picked = visited(only_subset);
  std::sort(picked.begin(), picked.end());
  std::cout << "subset visits exactly 4, 5, 6: " << (picked == std::vector<double>{4, 5, 6}) << '\n';

  // A sampler and the shuffle flag are mutually exclusive. The loader rejects the pair.
  loader::DataLoaderOptions both{};
  both.shuffle = true;
  both.sampler = std::make_shared<loader::SequentialSampler>();
  bool rejected = false;
  try {
    loader::DataLoader conflicting({x}, both);
  } catch (const std::exception&) {
    rejected = true;
  }
  std::cout << "shuffle and sampler together rejected: " << rejected << '\n';
}
```

Output

```text
sequential: 0, 1, 2, 3, 4, 5, 6, 7
subset visits exactly 4, 5, 6: true
shuffle and sampler together rejected: true
```

A sequential sampler visits every index in order, a subset sampler visits only its indices, and setting both shuffle and a sampler is refused.

### Example 5

Example 5 sets collate_fn to transform every assembled batch.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

// A map-style dataset that computes each sample on demand instead of storing a table.
// Sample i has feature i and label i modulo 2.
class SquaresDataset final : public pluss::loader::Dataset {
public:
  explicit SquaresDataset(std::int64_t count) : count_(count) {}

  [[nodiscard]] std::int64_t size() const override { return count_; }

  [[nodiscard]] pluss::loader::DataBatch get(std::int64_t index) const override {
    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
    pluss::loader::DataBatch sample{};
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index * index)}, {1}, cfg));
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index % 2)}, {1}, cfg));
    return sample;
  }

private:
  std::int64_t count_;
};

} // namespace

int main() {
  namespace loader = pluss::loader;
  const SquaresDataset dataset(5);

  // Without a collate function, the loader stacks samples into batches.
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader plain(dataset, options);
  std::cout << "samples: " << plain.sample_count() << '\n';
  std::cout << "batches: " << plain.batch_count() << '\n';
  std::cout << "last batch squares: " << plain.batch_at(2).x().sum().item<double>() << '\n';

  // A collate function receives each assembled batch and may transform it. Here it
  // keeps only the labels, so every delivered batch has one field.
  loader::DataLoaderOptions collated = options;
  collated.collate_fn = [](loader::DataBatch batch) {
    loader::DataBatch out{};
    out.fields.push_back(batch.y());
    return out;
  };
  loader::DataLoader labels(dataset, collated);
  std::cout << "collated fields per batch: " << labels.batch_at(0).size() << '\n';
  std::cout << "label sum in batch 0: " << labels.batch_at(0).x().sum().item<double>() << '\n';
}
```

Output

```text
samples: 5
batches: 3
last batch squares: 16
collated fields per batch: 1
label sum in batch 0: 1
```

The collated loader delivers only the field the function kept, instead of every field the dataset produced.

### Example 6

Example 6 runs with background workers, a prefetch depth, and a worker_init_fn.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <atomic>
#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(64);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {64}, cfg);

  // The synchronous reference: every batch is computed on the calling thread.
  loader::DataLoaderOptions synchronous{};
  synchronous.batch_size = 8;
  loader::DataLoader reference({x}, synchronous);

  // Two background workers compute batches ahead. In-order delivery is the default,
  // so the batches arrive in the same sequence as the reference.
  std::atomic<int> started{0};
  loader::DataLoaderOptions parallel = synchronous;
  parallel.num_workers = 2;
  parallel.prefetch_factor = 2;
  parallel.worker_init_fn = [&started](std::size_t) { started.fetch_add(1); };
  loader::DataLoader prefetched({x}, parallel);

  std::cout << "batches: " << prefetched.batch_count() << '\n';
  bool identical = reference.batch_count() == prefetched.batch_count();
  for (std::size_t i = 0; identical && i < reference.batch_count(); ++i) {
    identical = reference.batch_at(i).x().host<double>() == prefetched.batch_at(i).x().host<double>();
  }
  std::cout << "same batches in order: " << identical << '\n';

  // Iterating the prefetching loader yields the same order too.
  std::vector<double> streamed;
  for (auto batch : prefetched) {
    const auto v = batch.x().reshape({batch.x().numel()}).pull<double>();
    streamed.insert(streamed.end(), v.begin(), v.end());
  }
  std::cout << "streamed order matches: " << (streamed == values) << '\n';
  std::cout << "workers started: " << started.load() << '\n';
}
```

Output

```text
batches: 8
same batches in order: true
streamed order matches: true
workers started: 2
```

The prefetching loader delivers the same batches, in the same order, as a synchronous reference, and the init function ran once per worker.

### Example 7

Example 7 sets a target dtype so every batch is converted on the way out.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;

  // The source stores Float32 values. Nothing about the source changes.
  const auto f32 = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<float>{0.5F, 1.5F, 2.5F, 3.5F}, {4}, f32);

  // Without a target, each batch keeps the source dtype.
  loader::DataLoaderOptions keep{};
  keep.batch_size = 2;
  loader::DataLoader unchanged({x}, keep);
  std::cout << "source float32: " << (x.scalar_type() == pluss::ScalarType::Float32) << '\n';
  std::cout << "unchanged batches float32: "
            << (unchanged.batch_at(0).x().scalar_type() == pluss::ScalarType::Float32) << '\n';

  // With a target dtype, every delivered batch is converted on the way out.
  loader::DataLoaderOptions widen = keep;
  widen.target_dtype = pluss::ScalarType::Float64;
  loader::DataLoader converted({x}, widen);
  std::cout << "converted batches float64: "
            << (converted.batch_at(0).x().scalar_type() == pluss::ScalarType::Float64) << '\n';
  std::cout << "values survive conversion: "
            << (converted.batch_at(1).x().host<double>() == std::vector<double>{2.5, 3.5}) << '\n';
}
```

Output

```text
source float32: true
unchanged batches float32: true
converted batches float64: true
values survive conversion: true
```

Batches from the unconfigured loader keep the source dtype, and batches from the configured one are converted, with their values intact.

### Example 8

Example 8 triggers three refusals together: prefetch_factor without workers, a batch index past the end, and a zero batch size.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

// Returns true if the callable throws. The message itself is not printed, so this
// example's output does not depend on library wording.
template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);

  // Prefetching depth is a worker setting. Without workers it is an error, not a no-op.
  loader::DataLoaderOptions prefetch_without_workers{};
  prefetch_without_workers.prefetch_factor = 4;
  std::cout << "prefetch without workers rejected: "
            << throws([&] { loader::DataLoader bad({x}, prefetch_without_workers); }) << '\n';

  // Reading past the last batch is an error. Batch indices start at zero.
  loader::DataLoader two_batches({x}, [] {
    loader::DataLoaderOptions o{};
    o.batch_size = 2;
    return o;
  }());
  std::cout << "batch count: " << two_batches.batch_count() << '\n';
  std::cout << "reading batch two rejected: "
            << throws([&] { (void)two_batches.batch_at(two_batches.batch_count()); }) << '\n';

  // A zero batch size cannot describe any batch.
  loader::DataLoaderOptions zero{};
  zero.batch_size = 0;
  std::cout << "zero batch size rejected: "
            << throws([&] { loader::DataLoader bad({x}, zero); }) << '\n';
}
```

Output

```text
prefetch without workers rejected: true
batch count: 2
reading batch two rejected: true
zero batch size rejected: true
```

Each misconfiguration is refused rather than silently accepted.

### Example 9

Example 9 sets pin_memory and checks the documented no-op.

```cpp
// pin_memory is accepted and calls pin_memory() on every field of every batch, but the header
// itself documents this as a no-op today: the underlying engine's pin_memory() returns its input
// unchanged, and is_pinned() always reports false. This example confirms that is still the case.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.pin_memory = true;
  loader::DataLoader data({x}, options);
  std::cout << "batch is reported pinned: " << data.batch_at(0).x().is_pinned() << '\n';
  return 0;
}
```

Output

```text
batch is reported pinned: false
```

The batch still reports as not pinned, matching the field's own documented current behavior.

### Example 10

Example 10 sets in_order to false with several workers.

```cpp
// in_order set to false allows a worker to deliver a batch as soon as it finishes, rather than
// waiting for strict index order. Every batch still arrives exactly once; only the arrival order
// may differ from a run with in_order left at its default of true.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(40);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {40}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.num_workers = 3;
  options.in_order = false;
  loader::DataLoader data({x}, options);

  std::vector<double> seen;
  for (auto batch : data) {
    const auto v = batch.x().host<double>();
    seen.insert(seen.end(), v.begin(), v.end());
  }
  std::sort(seen.begin(), seen.end());
  std::cout << "every value delivered exactly once: " << (seen == values) << '\n';
  return 0;
}
```

Output

```text
every value delivered exactly once: true
```

Every value is still delivered exactly once; only the arrival order is allowed to differ from the default.

### Example 11

Example 11 sets persistent_workers and iterates two epochs in a row.

```cpp
// persistent_workers keeps worker threads alive between epochs instead of joining and respawning
// them. Both epochs still deliver every batch; the option changes how the threads are managed, not
// what is delivered.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  options.num_workers = 2;
  options.persistent_workers = true;
  loader::DataLoader data({x}, options);

  std::size_t first_epoch = 0;
  for (auto batch : data) first_epoch += batch.x().sizes()[0];
  std::size_t second_epoch = 0;
  for (auto batch : data) second_epoch += batch.x().sizes()[0];
  std::cout << "first epoch rows: " << first_epoch << '\n';
  std::cout << "second epoch rows: " << second_epoch << '\n';
  return 0;
}
```

Output

```text
first epoch rows: 6
second epoch rows: 6
```

Both epochs deliver every row, whether or not the worker threads were kept alive between them.

## See Also

- [`DataLoader`](loader-data-loader.md)
- [`Sampler`](loader-sampler.md)
