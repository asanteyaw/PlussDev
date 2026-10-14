# Loaders: A Guide

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [Loader API Reference](../api/loaders.md) · [Core Seeds Guide](core_seeds.md)

Training code and data pipelines share one chore. They take a collection of
samples and deliver it in batches, in an order that is sometimes random, often
repeated for several passes, and occasionally split into parts for training,
validation, and testing. Writing that by hand is easy to get subtly wrong. The
final batch is short, a shuffle is not reproducible, a split leaks a sample
into two parts, or a worker thread delivers batches out of order. The loader
family handles these chores, and each one is a separate decision you make on
purpose.

This guide walks through those decisions in order. It starts with plain batching and
moves to shuffling, sampling, custom datasets, splitting, parallel workers,
file paths, and dtype transfer. It ends with the configurations the loader
refuses, so you know what an error means before you meet it. Every program is
complete, and every printed line is copied from the program's real output.

Loader utilities are optional. A program can process containers directly, and
it can adopt the loader only where batching or splitting earns its place.

## The mental model

A loader reads a **source** and delivers **batches**. The source is either a
list of aligned fields or a dataset that produces one sample at a time. Every
field shares one **batch dimension**, which is the dimension that indexes
samples. Row `i` of field `x` and row `i` of field `y` always describe the same
sample, and a batch slices every field at the same positions.

Three things govern what a batch contains, and each is a separate setting:

- **Order:** the sequence in which samples are visited. Sequential by default,
  random when you ask for shuffling, or whatever a sampler you supply decides.
- **Boundaries:** the batch size, and whether a short final batch is kept or
  dropped.
- **Repetition:** the epoch. A new epoch can reshuffle, and the seed decides
  whether that reshuffle can be reproduced.

Keep those three apart when you debug. A surprising batch is almost always the
result of one setting, not of the loader as a whole.

## Example 1: Sequential batches and the short last one

The first program builds a loader over two aligned fields with seven samples and
a batch size of three. Sequential order visits samples zero through six in
turn. Seven samples in batches of three leave a final batch of one.

```cpp
#include <pluss/tils/aux/data_loader.h>
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

The loader reports seven samples and three batches. Batches zero and one each
have three rows, and batch two has the single leftover row. The label sums
check the alignment by hand. Samples zero through two have labels zero, one,
and zero, which sum to one. Samples three through five have labels one, zero,
and one, which sum to two. The last sample has label zero. Because `x` and `y`
are sliced together, the label sums always agree with the rows they came from.

## Example 2: Keep or drop the short batch

Some training loops need every batch to have the same size, because a fixed
shape is simpler to compile or to average over. The second program uses the
same seven samples twice. It keeps the short batch in one loader and drops it in
the other.

```cpp
#include <pluss/tils/aux/data_loader.h>
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

Keeping the partial batch gives three batches, and the last one has a single
row. Dropping it gives two full batches, and the seventh sample is never
delivered in that pass. Choose by asking what the loop does with a short batch.
A metric averaged per batch is biased by a one-row batch. A model with a fixed
input shape may not accept one at all. If you drop the batch, know that the
dropped sample is not seen in that pass, so a rotating order is a better choice
for a dataset that must be covered evenly.

## Example 3: A seeded shuffle that can be replayed

Shuffling makes training less sensitive to the order of the data. It also makes
a run harder to reproduce, unless the shuffle is seeded. The third program shows
the three properties that matter. A seed fixes the order, a shuffled pass still
covers every sample exactly once, and a new epoch reshuffles while returning to
an earlier epoch replays it.

```cpp
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// The order of samples for one full pass, read through the batches the loader delivers.
std::vector<double> one_pass(pluss::loader::DataLoader& loader) {
  std::vector<double> order;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).to_vec<double>();
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

Two loaders built from the same seed produce the same first pass. The sorted
pass equals the original set, so a shuffle reorders samples without dropping or
duplicating them. Moving to epoch one produces a different order, and moving
back to epoch zero reproduces the first pass exactly. The epoch, together with
the seed, is what makes a shuffled run reproducible from one process to the
next.

## Example 4: Samplers decide the order

A shuffle flag chooses between two fixed orders. A sampler gives you the order
itself. The fourth program uses two samplers. The sequential sampler reproduces
the plain order. The subset sampler visits only the indices you list, which is
how you train on one class, one group, or one split without copying data.

```cpp
#include <pluss/tils/aux/data_loader.h>
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
    const auto values = batch.x().reshape({batch.x().numel()}).to_vec<double>();
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

The sequential sampler visits every sample in index order, as the first line of
output shows. The subset sampler visits exactly the listed indices,
and the check sorts the picked samples before comparing, because the subset
order is random. The shuffle flag and a sampler cannot be combined. The loader
rejects the pair, because two strategies that both claim to decide the order
would disagree. Choose one: set `shuffle` for a plain random order, or supply a
sampler when you need a specific one.

## Example 5: A custom dataset and a collate function

Not every source is a table of fields. A dataset can compute each sample when it
is requested, which is useful when the stored form is large or the sample is
derived. The fifth program defines a small dataset whose sample `i` holds the
square of `i` and a label of `i` modulo two. The loader stacks samples into
batches, and a collate function then reshapes each batch before it is delivered.

```cpp
#include <pluss/tils/aux/data_loader.h>
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

The dataset has five samples, so a batch size of two gives three batches. The
last batch holds sample four alone, with squared value sixteen. The collate
function keeps only the labels, so each delivered batch has a single field. The
label sum of the first batch is one, because samples zero and one carry labels
zero and one. A collate function is the place to pad, stack, or filter a batch.
It runs once per batch, so keep it cheap relative to the work the batch feeds.

## Example 6: Splits that partition without leaking

Training, validation, and test parts are only useful when they never share a
sample. The sixth program splits ten samples sixty, twenty, and twenty percent,
with a seed. It then checks three properties. The three parts together hold each
row exactly once, the two fields stay aligned within each part, and the same
seed gives the same split.

```cpp
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);
  std::vector<double> tens(values);
  for (auto& v : tens) v *= 10.0;
  const pluss::Series y = pluss::make_series(tens, {10}, cfg);

  // Sixty percent train, twenty percent validation. The remaining twenty percent is test.
  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.seed = 7;
  const loader::DataSplit split = loader::split_fields({x, y}, options);

  std::cout << "train rows: " << split.train_fields[0].sizes()[0] << '\n';
  std::cout << "val rows: " << split.val_fields[0].sizes()[0] << '\n';
  std::cout << "test rows: " << split.test_fields[0].sizes()[0] << '\n';

  // The three splits partition the input: every row appears exactly once.
  std::vector<double> all;
  for (const auto* part : {&split.train_fields[0], &split.val_fields[0], &split.test_fields[0]}) {
    const auto v = part->reshape({part->numel()}).to_vec<double>();
    all.insert(all.end(), v.begin(), v.end());
  }
  std::sort(all.begin(), all.end());
  std::cout << "partition covers all rows once: " << (all == values) << '\n';

  // Fields stay aligned: y was built as ten times x, so every train row keeps that relation.
  const auto train_x = split.train_fields[0].reshape({split.train_fields[0].numel()}).to_vec<double>();
  const auto train_y = split.train_fields[1].reshape({split.train_fields[1].numel()}).to_vec<double>();
  bool aligned = train_x.size() == train_y.size();
  for (std::size_t i = 0; aligned && i < train_x.size(); ++i) aligned = train_y[i] == 10.0 * train_x[i];
  std::cout << "fields stay aligned: " << aligned << '\n';

  // The same seed gives the same split.
  const loader::DataSplit again = loader::split_fields({x, y}, options);
  std::cout << "same seed, same split: "
            << (again.train_fields[0].reshape({again.train_fields[0].numel()}).to_vec<double>() == train_x) << '\n';
}
```

Output

```text
train rows: 6
val rows: 2
test rows: 2
partition covers all rows once: true
fields stay aligned: true
same seed, same split: true
```

The parts hold six, two, and two rows, and sorting their union returns the
original ten values, so no row is missing and none appears twice. Each row of
the training part still satisfies the relation between its two fields, which
means the split moved whole samples and never separated a field from its row. A
split that is reproducible is one you can audit, so keep the seed with the run
that used it.

## Example 7: Background workers without reordering

A loader can compute batches on background threads while the training step runs.
The seventh program compares a synchronous loader with a loader that uses two
workers and a prefetch depth of two. Delivery is in order by default, so the
parallel loader hands over the same batches in the same sequence.

```cpp
#include <pluss/tils/aux/data_loader.h>
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
    identical = reference.batch_at(i).x().to_vec<double>() == prefetched.batch_at(i).x().to_vec<double>();
  }
  std::cout << "same batches in order: " << identical << '\n';

  // Iterating the prefetching loader yields the same order too.
  std::vector<double> streamed;
  for (auto batch : prefetched) {
    const auto v = batch.x().reshape({batch.x().numel()}).to_vec<double>();
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

The parallel loader yields eight batches, and each batch equals the synchronous
batch at the same position. Iterating the parallel loader streams the samples in
the original order. The worker initialization hook ran once per worker, which is
how you would set up thread-local state. Turn off in-order delivery only when
throughput matters more than the sequence, because then the batch order depends
on which worker finishes first.

## Example 8: Paths under a root

Loaders often read files, and a file path that escapes its project is a bug
waiting to happen. The eighth program sets a private root, resolves a file path
under it, creates a directory that a cache needs, and refuses a path that climbs
out of the root. The program removes its own directory when it finishes.

```cpp
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>
#include <pluss/tils/aux/pathing.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  namespace loader = pluss::loader;

  // A private root for this example, so nothing outside it is touched.
  const fs::path root = fs::temp_directory_path() / "pluss_loader_paths_example";
  fs::remove_all(root);
  fs::create_directories(root);
  loader::set_root_path(root);

  // A file path resolves under the root. It does not create the file.
  const loader::PathRef train = loader::file_path("data/train.csv");
  // The root is canonicalized, so compare against the loader's own root.
  const fs::path canonical_root = loader::root_path();
  std::cout << "train under root: " << (fs::path(train).lexically_relative(canonical_root).string() == "data/train.csv") << '\n';
  std::cout << "train exists yet: " << fs::exists(train) << '\n';

  // A directory with create_dirs makes the directory now.
  loader::PathOptions create{};
  create.create_dirs = true;
  const loader::DirRef cache = loader::file_dir("cache/features", create);
  std::cout << "cache directory created: " << fs::is_directory(fs::path(cache)) << '\n';
  std::cout << "cache is under root: " << (fs::path(cache).lexically_relative(canonical_root).string() == "cache/features") << '\n';

  // A path that climbs out of the root is refused.
  bool refused = false;
  try {
    (void)loader::file_path("../escape.csv");
  } catch (const std::exception&) {
    refused = true;
  }
  std::cout << "escape refused: " << refused << '\n';

  loader::reset_root_path();
  fs::remove_all(root);
}
```

Output

```text
train under root: true
train exists yet: false
cache directory created: true
cache is under root: true
escape refused: true
```

A file path resolves under the root without being created, so the first check
shows that the path exists only as a name until something writes it. A directory
requested with `create_dirs` is created at once, and it sits under the root. A
path such as `../escape.csv` is refused. The root is canonicalized, so when you
compare resolved paths, compare against `root_path()` rather than against the
path you passed in. On macOS, a temporary directory is a symbolic link, and the
two forms differ.

## Example 9: Converting batches on the way out

Sources are stored in whatever dtype the data arrived in. A training step may
need another one. The ninth program keeps a float32 source and shows that a
loader leaves batches in the source dtype by default. Setting a target dtype
converts every batch on delivery and leaves the source untouched.

```cpp
#include <pluss/tils/aux/data_loader.h>
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
            << (converted.batch_at(1).x().to_vec<double>() == std::vector<double>{2.5, 3.5}) << '\n';
}
```

Output

```text
source float32: true
unchanged batches float32: true
converted batches float64: true
values survive conversion: true
```

The source stays float32, and unchanged batches stay float32. With a target dtype
of float64, every batch is converted, and the values survive the conversion for
the samples checked. The conversion happens as each batch is delivered, so the stored source keeps
its original dtype.

## Example 10: Configurations the loader refuses

The loader is strict about a few configurations, because each would silently
mean something other than what the caller intended. The tenth program triggers
the refusals one at a time and prints whether each was rejected. The program
prints only the outcome, not the library's wording.

```cpp
#include <pluss/tils/aux/data_loader.h>
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

A prefetch depth without workers is rejected, because prefetching is a property
of the workers, and setting it without any would look like a setting that did
something. Reading a batch index equal to the batch count is rejected, since
batch indices start at zero. A zero batch size is rejected, because no batch can
hold zero samples.

## Decisions

Choose the order first, then the boundary, then the repetition.

| You want | Set | Notes |
|---|---|---|
| The plain order | nothing, or `sampler = SequentialSampler` | Visits samples in index order |
| A random order you can replay | `shuffle = true` and `seed` | Same seed, same order, every epoch |
| A specific subset | `sampler = SubsetRandomSampler(indices)` | Visits only the listed indices |
| Equal batch sizes | `drop_last = true` | Drops the short final batch |
| Background batches | `num_workers > 0`, `prefetch_factor` | Requires workers; in-order by default |
| Batches in another dtype | `target_dtype` | Converts each batch on delivery |
| Slate-local setup per worker | `worker_init_fn` | Runs once per worker thread |

Two of these choices hide the most trouble, so decide them deliberately. First,
keep the seed with the run. A shuffled loader is reproducible only when its seed
and epoch are both recorded. Second, decide what a short final batch means for
your loop before you train, because keeping or dropping it changes both the
number of steps and the samples seen per pass.

## Pitfalls

**Shuffling without a seed.** Each run then sees a different order, and a
regression between two runs cannot be attributed to the code. Example 3 shows
the fix: record the seed and the epoch.

**Assuming a dropped batch is covered elsewhere.** Example 2 shows that
`drop_last` removes the leftover samples from that pass. Check the coverage
you need across passes rather than assuming it.

**Combining a shuffle with a sampler.** Example 4 shows the rejection. Set one
strategy, not both.

**Reading past the last batch.** Example 10 shows that batch indices run from
zero to the batch count minus one.

**Comparing paths without canonicalizing.** Example 8 shows that a resolved path
uses the canonical root. Compare against `root_path()`.

**Dropping the order guarantee for speed.** Example 7 shows that in-order
delivery is the default, and turning it off makes the batch order depend on
which worker finishes first.

## Where next

- [Loader API Reference](../api/loaders.md) -- the canonical `pluss::loader`
  surface, one entry per type and operation.
- [Deterministic Seeds: A Guide](core_seeds.md) -- the seed family that
  `shuffle` and `DataSplitOptions::seed` draw from.
- [Core Utilities Manual](core.md) -- the shelf that the seed family belongs to.
