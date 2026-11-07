#pragma once

#include <CodeM/core/Errors.h>
#include <CodeM/core/ScalarType.h>
#include <CodeM/core/Series.h>
#include <pluss/codem/series_factory.h>
#include <pluss/config.h>
#include <pluss/tils/kit/sampler.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace pluss::loader {

struct DataBatch {
  std::vector<Series> fields{};

  [[nodiscard]] bool empty() const noexcept { return fields.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return fields.size(); }

  Series& at(std::size_t index) {
    PLUSS_CHECK(index < fields.size(), "pluss::DataBatch::at: index out of range");
    return fields[index];
  }

  const Series& at(std::size_t index) const {
    PLUSS_CHECK(index < fields.size(), "pluss::DataBatch::at: index out of range");
    return fields[index];
  }

  Series& operator[](std::size_t index) { return at(index); }
  const Series& operator[](std::size_t index) const { return at(index); }

  Series& x() {
    PLUSS_CHECK(!fields.empty(), "pluss::DataBatch::x: batch has no fields");
    return fields.front();
  }

  const Series& x() const {
    PLUSS_CHECK(!fields.empty(), "pluss::DataBatch::x: batch has no fields");
    return fields.front();
  }

  [[nodiscard]] bool has_y() const noexcept { return fields.size() >= 2; }

  Series& y() {
    PLUSS_CHECK(fields.size() >= 2, "pluss::DataBatch::y: batch does not contain a second field");
    return fields[1];
  }

  const Series& y() const {
    PLUSS_CHECK(fields.size() >= 2, "pluss::DataBatch::y: batch does not contain a second field");
    return fields[1];
  }
};

class Dataset {
public:
  virtual ~Dataset() = default;
  [[nodiscard]] virtual std::int64_t size() const = 0;
  [[nodiscard]] virtual DataBatch get(std::int64_t index) const = 0;

  // Map-style materialization hook used by DataLoader for vectorized batching.
  [[nodiscard]] virtual std::vector<Series> materialize_fields() const { return {}; }
  [[nodiscard]] virtual std::int64_t batch_dim() const noexcept { return 0; }
};

class SeriesDataset final : public Dataset {
public:
  SeriesDataset() = default;

  explicit SeriesDataset(std::vector<Series> fields, std::int64_t batch_dim = 0)
      : fields_(std::move(fields)), batch_dim_(batch_dim) {
    validate_();
  }

  SeriesDataset(std::initializer_list<Series> fields, std::int64_t batch_dim = 0)
      : SeriesDataset(std::vector<Series>(fields), batch_dim) {}

  [[nodiscard]] std::int64_t size() const override { return sample_count_; }
  [[nodiscard]] std::int64_t batch_dim() const noexcept override { return batch_dim_; }

  [[nodiscard]] const std::vector<Series>& fields() const noexcept { return fields_; }
  [[nodiscard]] std::vector<Series> materialize_fields() const override { return fields_; }

  [[nodiscard]] DataBatch get(std::int64_t index) const override {
    PLUSS_CHECK(index >= 0 && index < sample_count_, "pluss::SeriesDataset::get: index out of range");
    return slice_(index, 1);
  }

private:
  static std::int64_t normalize_dim_(std::int64_t dim, std::int64_t rank) {
    std::int64_t out = dim;
    if (out < 0) out += rank;
    return out;
  }

  void validate_() {
    PLUSS_CHECK(!fields_.empty(), "pluss::SeriesDataset: expected at least one field");
    dims_.clear();
    dims_.reserve(fields_.size());

    const auto check_field = [&](const Series& s, std::size_t idx) {
      PLUSS_CHECK(s.defined(), "pluss::SeriesDataset: field[" + std::to_string(idx) + "] must be defined");
      PLUSS_CHECK(s.dim() >= 1, "pluss::SeriesDataset: field[" + std::to_string(idx) + "] must have rank >= 1");
    };

    check_field(fields_.front(), 0);
    const auto d0 = normalize_dim_(batch_dim_, fields_.front().dim());
    PLUSS_CHECK(
        d0 >= 0 && d0 < fields_.front().dim(),
        "pluss::SeriesDataset: batch_dim out of range for field[0]");
    sample_count_ = fields_.front().sizes()[d0];
    dims_.push_back(d0);

    for (std::size_t i = 1; i < fields_.size(); ++i) {
      check_field(fields_[i], i);
      const auto d = normalize_dim_(batch_dim_, fields_[i].dim());
      PLUSS_CHECK(
          d >= 0 && d < fields_[i].dim(),
          "pluss::SeriesDataset: batch_dim out of range for field[" + std::to_string(i) + "]");
      PLUSS_CHECK(
          fields_[i].sizes()[d] == sample_count_,
          "pluss::SeriesDataset: all fields must share identical sample count on batch_dim");
      dims_.push_back(d);
    }
  }

  [[nodiscard]] DataBatch slice_(std::int64_t start, std::int64_t count) const {
    PLUSS_CHECK(start >= 0 && count >= 0 && start + count <= sample_count_, "pluss::SeriesDataset: invalid slice");
    std::vector<std::int64_t> ids(static_cast<std::size_t>(count));
    std::iota(ids.begin(), ids.end(), start);

    DataBatch out;
    out.fields.reserve(fields_.size());
    for (std::size_t i = 0; i < fields_.size(); ++i) {
      const auto& s = fields_[i];
      const auto cfg = ::pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(s.platform());
      const auto idx = make_series(ids, {count}, cfg);
      out.fields.push_back(s.index_select(dims_[i], idx));
    }
    return out;
  }

  std::vector<Series> fields_{};
  std::vector<std::int64_t> dims_{};
  std::int64_t batch_dim_{0};
  std::int64_t sample_count_{0};
};

namespace detail {

// materialize_fields() is the fast, vectorized path -- when a subclass
// provides it, use it directly. When it is left at its unimplemented
// (empty) default, fall back to the per-sample contract every subclass
// already has to implement: call get(i) once per sample and unite the
// results along batch_dim(), producing the exact fields the vectorized
// path would have returned. Slower (one get() call per sample instead
// of a single index_select), but makes every Dataset subclass usable
// with DataLoader/split_fields/temporal_split_fields out of the box,
// matching what get() is documented to be for.
inline std::vector<Series> materialize_via_get_(const Dataset& dataset) {
  const std::int64_t n = dataset.size();
  PLUSS_CHECK(n > 0, "pluss::loader::Dataset: dataset must have at least one sample");
  const std::int64_t dim = dataset.batch_dim();

  const DataBatch first = dataset.get(0);
  PLUSS_CHECK(
      !first.fields.empty(),
      "pluss::loader::Dataset: dataset::get(0) returned no fields "
      "(materialize_fields() and get() cannot both be empty)");

  std::vector<std::vector<Series>> per_field(first.fields.size());
  for (std::size_t f = 0; f < first.fields.size(); ++f) {
    per_field[f].reserve(static_cast<std::size_t>(n));
    per_field[f].push_back(first.fields[f]);
  }
  for (std::int64_t i = 1; i < n; ++i) {
    const DataBatch sample = dataset.get(i);
    PLUSS_CHECK(
        sample.fields.size() == first.fields.size(),
        "pluss::loader::Dataset: every dataset::get(index) call must return "
        "the same field count");
    for (std::size_t f = 0; f < sample.fields.size(); ++f) {
      per_field[f].push_back(sample.fields[f]);
    }
  }

  std::vector<Series> materialized;
  materialized.reserve(per_field.size());
  for (auto& samples : per_field) {
    // A bare scalar (rank 0) has no axis to unite along at all -- the
    // most natural minimal get() implementation (one scalar per
    // sample) returns exactly this. Promote to rank 1 first so
    // batch_dim() names a real axis, the same shape SeriesDataset's
    // own fields are already required to have (dim() >= 1).
    for (auto& s : samples) {
      if (s.dim() == 0) s = s.unsqueeze(0);
    }
    materialized.push_back(
        samples.front().unite(std::vector<Series>(samples.begin() + 1, samples.end()), dim));
  }
  return materialized;
}

inline std::vector<Series> materialize_dataset_fields_(const Dataset& dataset) {
  auto fields = dataset.materialize_fields();
  if (!fields.empty()) return fields;
  return materialize_via_get_(dataset);
}

} // namespace detail

struct DataLoaderOptions {
  std::int64_t batch_size{32};
  bool shuffle{false};
  bool drop_last{false};
  std::uint64_t seed{0};
  std::int64_t batch_dim{0};
  bool reshuffle_each_epoch{true};
  std::optional<Platform> target_platform{};
  std::optional<ScalarType> target_dtype{};
  std::function<DataBatch(DataBatch)> collate_fn{};

  // A custom index-sampling strategy, overriding `shuffle` when set (the
  // two are mutually exclusive, the same convention torch.utils.data uses --
  // see Sampler in <pluss/tils/kit/sampler.h>). Leave unset (the default)
  // to keep the plain `shuffle` bool's existing behavior unchanged.
  std::shared_ptr<Sampler> sampler{};

  // How many background threads compute batches ahead of consumption. `0`
  // (the default) computes every batch synchronously on the calling thread,
  // exactly as before this field existed -- every existing caller is
  // unaffected. `> 0` spawns that many worker threads (not OS processes --
  // C++ has no GIL to work around, so a thread pool avoids the
  // serialization/IPC cost torch's own multiprocessing workers pay).
  std::size_t num_workers{0};

  // How many batches each worker may compute ahead of the consumer, before
  // blocking. Only meaningful when `num_workers > 0`; setting it with
  // `num_workers == 0` throws, matching torch's own constraint. Defaults to
  // `2` when left unset and `num_workers > 0`.
  std::optional<std::int64_t> prefetch_factor{};

  // When `true`, worker threads started for one epoch are kept alive and
  // reused for the next epoch instead of being joined and respawned.
  // Requires `num_workers > 0`.
  bool persistent_workers{false};

  // When `true`, calls `.pin_memory()` on every field of every produced
  // batch. Currently a documented no-op in the underlying tensor engine
  // (CodeM's `pin_memory` op returns its input unchanged, and
  // `Series::is_pinned()` always reports `false`) -- wiring it through here
  // now means real pinning takes effect automatically the moment CodeM
  // implements it, with no DataLoader-side change needed.
  bool pin_memory{false};

  // Seconds to wait for a prefetched batch before throwing. `0.0` (the
  // default) waits indefinitely. Only meaningful when `num_workers > 0`.
  double timeout{0.0};

  // When `true` (the default), batches are delivered in strict ascending
  // index order even when `num_workers > 1` lets workers finish out of
  // order. When `false`, a batch is delivered as soon as any worker
  // finishes it, which can improve throughput at the cost of a
  // non-deterministic delivery order.
  bool in_order{true};

  // Called once per worker thread, from that thread, right after it starts
  // and before it computes its first batch -- given the worker's own id in
  // `[0, num_workers)`. Unlike torch's `worker_init_fn` (which exists partly
  // to work around per-process RNG state and picklability across a process
  // boundary), this is a plain thread-entry hook; use it for thread-local
  // setup a caller's own `collate_fn` might need.
  std::function<void(std::size_t)> worker_init_fn{};
};

class DataLoader {
public:
  using batch_type = DataBatch;

  class iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = batch_type;
    using difference_type = std::ptrdiff_t;

    iterator() = default;
    iterator(DataLoader* owner, std::size_t index) : owner_(owner), index_(index) {}

    batch_type operator*() const {
      PLUSS_CHECK(owner_ != nullptr, "pluss::loader::DataLoader::iterator: invalid iterator dereference");
      return owner_->fetch_(index_);
    }

    iterator& operator++() {
      ++index_;
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator& other) const {
      return owner_ == other.owner_ && index_ == other.index_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

  private:
    DataLoader* owner_{nullptr};
    std::size_t index_{0};
  };

  DataLoader() = default;

  explicit DataLoader(const Dataset& dataset, DataLoaderOptions options = {})
      : DataLoader(
            detail::materialize_dataset_fields_(dataset),
            with_dataset_dim_(std::move(options), dataset.batch_dim())) {}

  explicit DataLoader(const SeriesDataset& dataset, DataLoaderOptions options = {})
      : DataLoader(fields_from_series_dataset_(dataset), with_dataset_dim_(std::move(options), dataset.batch_dim())) {}

  explicit DataLoader(std::vector<Series> fields, DataLoaderOptions options = {})
      : fields_(std::move(fields)), options_(options) {
    validate_and_initialize_();
  }

  DataLoader(std::initializer_list<Series> fields, DataLoaderOptions options = {})
      : DataLoader(std::vector<Series>(fields), options) {}

  [[nodiscard]] bool empty() const noexcept { return sample_count_ == 0; }
  [[nodiscard]] std::size_t field_count() const noexcept { return fields_.size(); }
  [[nodiscard]] std::int64_t sample_count() const noexcept { return sample_count_; }
  [[nodiscard]] std::int64_t batch_size() const noexcept { return options_.batch_size; }
  [[nodiscard]] const DataLoaderOptions& options() const noexcept { return options_; }
  [[nodiscard]] std::uint64_t epoch() const noexcept { return epoch_; }

  [[nodiscard]] std::size_t batch_count() const noexcept {
    const std::int64_t total = static_cast<std::int64_t>(order_.size());
    if (total <= 0 || options_.batch_size <= 0) return 0;
    const std::int64_t full = total / options_.batch_size;
    const std::int64_t rem = total % options_.batch_size;
    if (rem == 0 || options_.drop_last) return static_cast<std::size_t>(full);
    return static_cast<std::size_t>(full + 1);
  }

  DataLoader& set_options(DataLoaderOptions options) {
    options_ = options;
    validate_and_initialize_();
    return *this;
  }

  DataLoader& set_epoch(std::uint64_t epoch, bool reshuffle = true) {
    epoch_ = epoch;
    cursor_ = 0;
    if (reshuffle && (options_.shuffle || options_.sampler)) {
      regenerate_order_();
    }
    return *this;
  }

  DataLoader& reset(bool reshuffle = true) {
    ++epoch_;
    cursor_ = 0;
    if (reshuffle && (options_.shuffle || options_.sampler)) {
      regenerate_order_();
    }
    return *this;
  }

  std::optional<batch_type> next() {
    if (cursor_ >= batch_count()) {
      if (options_.num_workers > 0 && !options_.persistent_workers) stop_prefetch_();
      return std::nullopt;
    }
    batch_type out = fetch_(cursor_);
    ++cursor_;
    return out;
  }

  batch_type batch_at(std::size_t batch_index) const {
    PLUSS_CHECK(batch_index < batch_count(), "pluss::loader::DataLoader::batch_at: batch index out of range");
    const std::int64_t total = static_cast<std::int64_t>(order_.size());
    const std::int64_t start = static_cast<std::int64_t>(batch_index) * options_.batch_size;
    std::int64_t count = options_.batch_size;
    if (!options_.drop_last) {
      count = std::min<std::int64_t>(count, total - start);
    }
    return make_batch_(start, count);
  }

  iterator begin() {
    if (options_.reshuffle_each_epoch) {
      reset(/*reshuffle=*/true);
    } else {
      cursor_ = 0;
    }
    if (options_.num_workers > 0) {
      if (!options_.persistent_workers) stop_prefetch_();
      start_prefetch_();
    }
    return iterator(this, 0);
  }

  iterator end() { return iterator(this, batch_count()); }

  ~DataLoader() { stop_prefetch_(); }

  // Not defaulted: a defaulted move would transfer `prefetch_` (a
  // unique_ptr) by nulling the source's pointer while that source's own
  // worker threads -- spawned with `this` captured as the source object's
  // address, which a move does not relocate -- might still be mid-loop and
  // about to dereference it. Stopping prefetch on the source first (through
  // its still-valid pointer, exactly as `stop_prefetch_()` now guarantees
  // for destruction) closes the same hazard here.
  DataLoader(DataLoader&& other) noexcept {
    // Stop `other`'s workers *before* touching any of its other members --
    // they read fields_/options_/dims_/order_ via `this` (== &other) too, so
    // moving those out first would race a still-running worker's reads of
    // the very members being moved from under it.
    other.stop_prefetch_();
    fields_ = std::move(other.fields_);
    options_ = std::move(other.options_);
    dims_ = std::move(other.dims_);
    order_ = std::move(other.order_);
    sample_count_ = other.sample_count_;
    epoch_ = other.epoch_;
    cursor_ = other.cursor_;
  }

  DataLoader& operator=(DataLoader&& other) noexcept {
    if (this != &other) {
      stop_prefetch_();
      other.stop_prefetch_();
      fields_ = std::move(other.fields_);
      options_ = std::move(other.options_);
      dims_ = std::move(other.dims_);
      order_ = std::move(other.order_);
      sample_count_ = other.sample_count_;
      epoch_ = other.epoch_;
      cursor_ = other.cursor_;
    }
    return *this;
  }

  DataLoader(const DataLoader&) = delete;
  DataLoader& operator=(const DataLoader&) = delete;

private:

  static std::vector<Series> fields_from_series_dataset_(const SeriesDataset& dataset) {
    return dataset.fields();
  }

  static DataLoaderOptions with_dataset_dim_(DataLoaderOptions options, std::int64_t dataset_batch_dim) {
    options.batch_dim = dataset_batch_dim;
    return options;
  }

  static std::int64_t normalize_dim_(std::int64_t dim, std::int64_t rank) {
    std::int64_t out = dim;
    if (out < 0) out += rank;
    return out;
  }

  void validate_and_initialize_() {
    stop_prefetch_();

    PLUSS_CHECK(!fields_.empty(), "pluss::loader::DataLoader: expected at least one Series field");
    PLUSS_CHECK(options_.batch_size > 0, "pluss::loader::DataLoader: batch_size must be > 0");
    PLUSS_CHECK(
        !(options_.sampler && options_.shuffle),
        "pluss::loader::DataLoader: sampler and shuffle are mutually exclusive");
    PLUSS_CHECK(
        options_.num_workers > 0 || !options_.prefetch_factor.has_value(),
        "pluss::loader::DataLoader: prefetch_factor requires num_workers > 0");
    PLUSS_CHECK(
        !options_.prefetch_factor.has_value() || *options_.prefetch_factor > 0,
        "pluss::loader::DataLoader: prefetch_factor must be > 0 when set");
    PLUSS_CHECK(
        options_.num_workers > 0 || !options_.persistent_workers,
        "pluss::loader::DataLoader: persistent_workers requires num_workers > 0");
    PLUSS_CHECK(options_.timeout >= 0.0, "pluss::loader::DataLoader: timeout must be >= 0");

    dims_.clear();
    dims_.reserve(fields_.size());

    const auto check_field = [&](const Series& s, std::size_t idx) {
      PLUSS_CHECK(s.defined(), "pluss::loader::DataLoader: field[" + std::to_string(idx) + "] must be defined");
      PLUSS_CHECK(s.dim() >= 1, "pluss::loader::DataLoader: field[" + std::to_string(idx) + "] must have rank >= 1");
    };

    check_field(fields_.front(), 0);
    const std::int64_t d0 = normalize_dim_(options_.batch_dim, fields_.front().dim());
    PLUSS_CHECK(
        d0 >= 0 && d0 < fields_.front().dim(),
        "pluss::loader::DataLoader: batch_dim out of range for field[0]");
    sample_count_ = fields_.front().sizes()[d0];
    PLUSS_CHECK(sample_count_ >= 0, "pluss::loader::DataLoader: invalid sample dimension");
    dims_.push_back(d0);

    for (std::size_t i = 1; i < fields_.size(); ++i) {
      check_field(fields_[i], i);
      const std::int64_t d = normalize_dim_(options_.batch_dim, fields_[i].dim());
      PLUSS_CHECK(
          d >= 0 && d < fields_[i].dim(),
          "pluss::loader::DataLoader: batch_dim out of range for field[" + std::to_string(i) + "]");
      PLUSS_CHECK(
          fields_[i].sizes()[d] == sample_count_,
          "pluss::loader::DataLoader: all fields must share identical sample count on batch_dim");
      dims_.push_back(d);
    }

    cursor_ = 0;
    epoch_ = 0;
    regenerate_order_();
  }

  // Rebuilds `order_` for the current epoch: a plain 0..sample_count_-1
  // sequence, optionally shuffled (the original behavior, unchanged), or --
  // when `options_.sampler` is set -- whatever that Sampler produces
  // (which may legitimately be a different length than `sample_count_`,
  // e.g. a RandomSampler with an explicit `num_samples` override). Batch
  // math (`batch_count()`/`batch_at()`) is sized off `order_.size()`, never
  // `sample_count_`, directly for this reason.
  void regenerate_order_() {
    if (options_.sampler) {
      order_ = options_.sampler->sample(sample_count_, options_.seed + epoch_);
      for (const auto idx : order_) {
        PLUSS_CHECK(
            idx >= 0 && idx < sample_count_,
            "pluss::loader::DataLoader: sampler produced an index out of range");
      }
      return;
    }
    order_.resize(static_cast<std::size_t>(sample_count_));
    std::iota(order_.begin(), order_.end(), static_cast<std::int64_t>(0));
    if (options_.shuffle) {
      std::mt19937_64 rng(options_.seed + epoch_);
      std::shuffle(order_.begin(), order_.end(), rng);
    }
  }

  // Dispatches a single batch fetch to either the synchronous path
  // (`num_workers == 0`, unchanged from before this field existed) or the
  // background prefetch queue. The threaded path ignores `index` and
  // always delivers the next batch in strictly ascending call order --
  // safe because every caller (both `next()` and `iterator`) only ever
  // dereferences sequentially, never out of order or more than once per
  // position (the same single-pass contract `input_iterator_tag` implies).
  batch_type fetch_(std::size_t index) {
    if (options_.num_workers == 0) return batch_at(index);
    if (!prefetch_ || prefetch_->threads.empty()) start_prefetch_();
    return pull_prefetched_();
  }

  batch_type make_batch_(std::int64_t start, std::int64_t count) const {
    // Batch construction (index_select/dtype-cast/collate) is bookkeeping,
    // never a differentiable step -- and, when num_workers > 0, this runs
    // concurrently from multiple worker threads. The tape's own record/pause
    // state is thread_local (see CodeM/core/SeriesTape.cpp), so this guard
    // only ever affects the calling thread's own state, never races with it.
    const ::pluss::NoGradGuard no_grad;

    PLUSS_CHECK(start >= 0, "pluss::loader::DataLoader: internal batch start must be >= 0");
    PLUSS_CHECK(count >= 0, "pluss::loader::DataLoader: internal batch count must be >= 0");
    PLUSS_CHECK(
        start + count <= static_cast<std::int64_t>(order_.size()),
        "pluss::loader::DataLoader: internal batch span out of bounds");

    std::vector<std::int64_t> ids(static_cast<std::size_t>(count));
    std::copy_n(order_.begin() + start, count, ids.begin());

    batch_type out;
    out.fields.reserve(fields_.size());
    for (std::size_t i = 0; i < fields_.size(); ++i) {
      const auto& s = fields_[i];
      const ::pluss::Config icfg =
          ::pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(s.platform());
      const Series idx = make_series(ids, {count}, icfg);
      auto field = s.index_select(dims_[i], idx);
      if (options_.target_platform.has_value() || options_.target_dtype.has_value()) {
        const Platform platform = options_.target_platform.value_or(field.platform());
        const ScalarType dtype = options_.target_dtype.value_or(field.scalar_type());
        field = field.to(platform, dtype);
      }
      out.fields.push_back(std::move(field));
    }
    if (options_.collate_fn) {
      out = options_.collate_fn(std::move(out));
    }
    if (options_.pin_memory) {
      for (auto& field : out.fields) {
        if (field.defined()) field = field.pin_memory();
      }
    }
    return out;
  }

  // --- background prefetching (num_workers > 0) ---------------------

  struct Prefetch {
    std::vector<std::thread> threads{};
    std::mutex mu{};
    // Serializes every worker's actual tensor-engine call (`batch_at()` ->
    // `index_select`/`.to()`/etc). CodeM's own thread-safety for *concurrent*
    // Series construction across threads is not a documented guarantee this
    // codebase makes anywhere else, and empirically crashes under real
    // concurrent load (confirmed via this DataLoader's own test suite) --
    // so engine calls are serialized here rather than shipping a race.
    // Worker-to-worker parallelism for the tensor op itself is therefore
    // nil, but the pipeline-overlap benefit (a worker preparing the next
    // batch while the consumer thread is off doing something else with the
    // batch already delivered) is real and unaffected.
    std::mutex engine_mu{};
    std::condition_variable cv{};
    std::map<std::size_t, batch_type> completed{};
    std::size_t next_to_compute{0};
    std::size_t next_to_deliver{0};
    std::size_t total_batches{0};
    std::size_t capacity{1};
    bool stop{false};
    // Set by whichever worker thread first hits an exception computing a
    // batch (e.g. a bad value tripping a validation check, or a caller's
    // own `collate_fn` throwing). An uncaught exception escaping a
    // std::thread's entry function calls std::terminate(), so every worker
    // catches everything here and hands it to the consumer instead, which
    // rethrows it from `pull_prefetched_()` on the calling thread.
    std::exception_ptr pending_error{};

    // Guarantees a clean shutdown no matter how the owning DataLoader is
    // destroyed -- a joinable std::thread destroyed while still running
    // calls std::terminate(), so this must run before `threads` itself is
    // torn down (its own destructor, invoked immediately after this body).
    ~Prefetch() {
      {
        std::unique_lock<std::mutex> lock(mu);
        stop = true;
      }
      cv.notify_all();
      for (auto& t : threads) {
        if (t.joinable()) t.join();
      }
    }
  };

  void worker_loop_(std::size_t worker_id) {
    try {
      if (options_.worker_init_fn) options_.worker_init_fn(worker_id);
    } catch (...) {
      std::unique_lock<std::mutex> lock(prefetch_->mu);
      if (!prefetch_->pending_error) prefetch_->pending_error = std::current_exception();
      prefetch_->cv.notify_all();
      return;
    }
    while (true) {
      std::size_t my_index;
      {
        std::unique_lock<std::mutex> lock(prefetch_->mu);
        prefetch_->cv.wait(lock, [&] {
          return prefetch_->stop || prefetch_->pending_error ||
                 (prefetch_->next_to_compute < prefetch_->total_batches &&
                  (prefetch_->next_to_compute - prefetch_->next_to_deliver) < prefetch_->capacity);
        });
        if (prefetch_->stop || prefetch_->pending_error) return;
        my_index = prefetch_->next_to_compute++;
      }
      try {
        batch_type result;
        {
          std::unique_lock<std::mutex> engine_lock(prefetch_->engine_mu);
          result = batch_at(my_index);
        }
        std::unique_lock<std::mutex> lock(prefetch_->mu);
        prefetch_->completed.emplace(my_index, std::move(result));
      } catch (...) {
        std::unique_lock<std::mutex> lock(prefetch_->mu);
        if (!prefetch_->pending_error) prefetch_->pending_error = std::current_exception();
        prefetch_->cv.notify_all();
        return;
      }
      prefetch_->cv.notify_all();
    }
  }

  void start_prefetch_() {
    if (!prefetch_) prefetch_ = std::make_unique<Prefetch>();
    {
      std::unique_lock<std::mutex> lock(prefetch_->mu);
      prefetch_->completed.clear();
      prefetch_->next_to_compute = 0;
      prefetch_->next_to_deliver = 0;
      prefetch_->total_batches = batch_count();
      const std::int64_t pf = options_.prefetch_factor.value_or(2);
      prefetch_->capacity = std::max<std::size_t>(1, options_.num_workers * static_cast<std::size_t>(pf));
      prefetch_->stop = false;
    }
    if (prefetch_->threads.empty()) {
      prefetch_->threads.reserve(options_.num_workers);
      for (std::size_t w = 0; w < options_.num_workers; ++w) {
        prefetch_->threads.emplace_back([this, w] { worker_loop_(w); });
      }
    }
    prefetch_->cv.notify_all();
  }

  void stop_prefetch_() {
    if (!prefetch_) return;
    // Signal stop and join *before* resetting the pointer. unique_ptr::reset()
    // nulls this->prefetch_ before Prefetch's destructor runs, and worker
    // threads reach the Prefetch object through this->prefetch_ (not a
    // stable captured raw pointer) -- so if a worker were still between
    // statements in worker_loop_() when reset() nulled the member, its next
    // `prefetch_->...` would dereference nullptr. Joining here first,
    // through the still-valid pointer, guarantees no worker thread is still
    // running by the time the pointer itself is torn down.
    {
      std::unique_lock<std::mutex> lock(prefetch_->mu);
      prefetch_->stop = true;
    }
    prefetch_->cv.notify_all();
    for (auto& t : prefetch_->threads) {
      if (t.joinable()) t.join();
    }
    prefetch_.reset();
  }

  batch_type pull_prefetched_() {
    std::unique_lock<std::mutex> lock(prefetch_->mu);
    const auto ready = [&] {
      return prefetch_->stop || prefetch_->pending_error ||
             (options_.in_order ? prefetch_->completed.count(prefetch_->next_to_deliver) > 0
                                : !prefetch_->completed.empty());
    };
    if (options_.timeout > 0.0) {
      const bool got = prefetch_->cv.wait_for(lock, std::chrono::duration<double>(options_.timeout), ready);
      PLUSS_CHECK(got, "pluss::loader::DataLoader: timed out waiting for a prefetched batch");
    } else {
      prefetch_->cv.wait(lock, ready);
    }
    if (prefetch_->pending_error) {
      const auto err = prefetch_->pending_error;
      lock.unlock();
      std::rethrow_exception(err);
    }
    PLUSS_CHECK(!prefetch_->stop, "pluss::loader::DataLoader: prefetch stopped while a batch was still pending");

    const std::size_t key = options_.in_order ? prefetch_->next_to_deliver : prefetch_->completed.begin()->first;
    auto it = prefetch_->completed.find(key);
    batch_type out = std::move(it->second);
    prefetch_->completed.erase(it);
    ++prefetch_->next_to_deliver;
    lock.unlock();
    prefetch_->cv.notify_all();
    return out;
  }

  std::vector<Series> fields_{};
  DataLoaderOptions options_{};
  std::vector<std::int64_t> dims_{};
  std::vector<std::int64_t> order_{};
  std::int64_t sample_count_{0};
  std::uint64_t epoch_{0};
  std::size_t cursor_{0};
  std::unique_ptr<Prefetch> prefetch_{};
};

struct DataSplitOptions {
  double train_ratio{0.8};
  double val_ratio{0.1};
  bool shuffle{true};
  std::uint64_t seed{0};
  std::int64_t batch_dim{0};
};

struct DataSplit {
  std::vector<Series> train_fields{};
  std::vector<Series> val_fields{};
  std::vector<Series> test_fields{};
};

namespace detail {

inline std::int64_t normalize_split_dim_(std::int64_t dim, std::int64_t rank) {
  std::int64_t out = dim;
  if (out < 0) out += rank;
  return out;
}

inline std::vector<std::int64_t> split_dims_for_fields_(
    const std::vector<Series>& fields,
    std::int64_t batch_dim,
    std::int64_t* sample_count) {
  PLUSS_CHECK(!fields.empty(), "pluss::split_fields: expected at least one Series field");
  std::vector<std::int64_t> dims;
  dims.reserve(fields.size());

  PLUSS_CHECK(fields.front().defined(), "pluss::split_fields: field[0] must be defined");
  PLUSS_CHECK(fields.front().dim() >= 1, "pluss::split_fields: field[0] must have rank >= 1");
  const auto d0 = normalize_split_dim_(batch_dim, fields.front().dim());
  PLUSS_CHECK(d0 >= 0 && d0 < fields.front().dim(), "pluss::split_fields: batch_dim out of range for field[0]");
  const auto n = fields.front().sizes()[d0];
  dims.push_back(d0);

  for (std::size_t i = 1; i < fields.size(); ++i) {
    const auto& s = fields[i];
    PLUSS_CHECK(s.defined(), "pluss::split_fields: field[" + std::to_string(i) + "] must be defined");
    PLUSS_CHECK(s.dim() >= 1, "pluss::split_fields: field[" + std::to_string(i) + "] must have rank >= 1");
    const auto d = normalize_split_dim_(batch_dim, s.dim());
    PLUSS_CHECK(
        d >= 0 && d < s.dim(),
        "pluss::split_fields: batch_dim out of range for field[" + std::to_string(i) + "]");
    PLUSS_CHECK(
        s.sizes()[d] == n,
        "pluss::split_fields: all fields must share identical sample count on batch_dim");
    dims.push_back(d);
  }

  *sample_count = n;
  return dims;
}

inline std::vector<std::int64_t> take_ids_(
    const std::vector<std::int64_t>& ids,
    std::int64_t offset,
    std::int64_t count) {
  std::vector<std::int64_t> out(static_cast<std::size_t>(count));
  std::copy_n(ids.begin() + offset, count, out.begin());
  return out;
}

inline std::vector<Series> index_fields_(
    const std::vector<Series>& fields,
    const std::vector<std::int64_t>& dims,
    const std::vector<std::int64_t>& ids) {
  std::vector<Series> out;
  out.reserve(fields.size());
  for (std::size_t i = 0; i < fields.size(); ++i) {
    const auto& s = fields[i];
    const auto cfg =
        ::pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(s.platform());
    const auto idx = make_series(ids, {static_cast<std::int64_t>(ids.size())}, cfg);
    out.push_back(s.index_select(dims[i], idx));
  }
  return out;
}

} // namespace detail

inline DataSplit split_fields(const std::vector<Series>& fields, DataSplitOptions options = {}) {
  PLUSS_CHECK(
      options.train_ratio >= 0.0 && options.train_ratio <= 1.0,
      "pluss::split_fields: train_ratio must be within [0, 1]");
  PLUSS_CHECK(
      options.val_ratio >= 0.0 && options.val_ratio <= 1.0,
      "pluss::split_fields: val_ratio must be within [0, 1]");
  PLUSS_CHECK(
      (options.train_ratio + options.val_ratio) <= 1.0,
      "pluss::split_fields: train_ratio + val_ratio must be <= 1.0");

  std::int64_t sample_count = 0;
  const auto dims = detail::split_dims_for_fields_(fields, options.batch_dim, &sample_count);
  PLUSS_CHECK(sample_count >= 0, "pluss::split_fields: invalid sample count");

  const auto to_count = [&](double ratio) {
    return static_cast<std::int64_t>(std::floor(ratio * static_cast<double>(sample_count)));
  };

  const std::int64_t train_count = to_count(options.train_ratio);
  const std::int64_t val_count = std::min<std::int64_t>(to_count(options.val_ratio), sample_count - train_count);
  const std::int64_t test_count = sample_count - train_count - val_count;

  std::vector<std::int64_t> order(static_cast<std::size_t>(sample_count));
  std::iota(order.begin(), order.end(), static_cast<std::int64_t>(0));
  if (options.shuffle) {
    std::mt19937_64 rng(options.seed);
    std::shuffle(order.begin(), order.end(), rng);
  }

  const auto train_ids = detail::take_ids_(order, 0, train_count);
  const auto val_ids = detail::take_ids_(order, train_count, val_count);
  const auto test_ids = detail::take_ids_(order, train_count + val_count, test_count);

  DataSplit out;
  out.train_fields = detail::index_fields_(fields, dims, train_ids);
  out.val_fields = detail::index_fields_(fields, dims, val_ids);
  out.test_fields = detail::index_fields_(fields, dims, test_ids);
  return out;
}

inline DataSplit split_fields(std::initializer_list<Series> fields, DataSplitOptions options = {}) {
  return split_fields(std::vector<Series>(fields), options);
}

struct DataLoaderSplit {
  DataLoader train;
  DataLoader val;
  DataLoader test;
};

struct TemporalSplitOptions {
  std::int64_t folds{5};
  std::optional<std::int64_t> test_size{};
  std::optional<std::int64_t> max_train_size{};
  std::int64_t gap{0};
  std::int64_t batch_dim{0};
};

struct TemporalIndexFold {
  std::vector<std::int64_t> train_indices{};
  std::vector<std::int64_t> test_indices{};
};

struct TemporalIndexSplit {
  std::vector<TemporalIndexFold> folds{};
};

struct TemporalFieldFold {
  std::vector<Series> train_fields{};
  std::vector<Series> test_fields{};
};

struct TemporalFieldSplit {
  std::vector<TemporalFieldFold> folds{};
};

struct TemporalLoaderFold {
  DataLoader train{};
  DataLoader test{};
};

struct TemporalLoaderSplit {
  std::vector<TemporalLoaderFold> folds{};
};

inline TemporalIndexSplit temporal_split_indices(
    std::int64_t sample_count,
    TemporalSplitOptions options = {}) {
  PLUSS_CHECK(sample_count >= 2, "pluss::temporal_split_indices: sample_count must be >= 2");
  PLUSS_CHECK(options.folds >= 2, "pluss::temporal_split_indices: folds must be >= 2");
  PLUSS_CHECK(options.gap >= 0, "pluss::temporal_split_indices: gap must be >= 0");
  if (options.test_size.has_value()) {
    PLUSS_CHECK(*options.test_size > 0, "pluss::temporal_split_indices: test_size must be > 0 when provided");
  }
  if (options.max_train_size.has_value()) {
    PLUSS_CHECK(
        *options.max_train_size > 0,
        "pluss::temporal_split_indices: max_train_size must be > 0 when provided");
  }

  const std::int64_t folds = options.folds;
  const std::int64_t n_folds = folds + 1;
  const std::int64_t test_size = options.test_size.value_or(sample_count / n_folds);

  PLUSS_CHECK(test_size > 0, "pluss::temporal_split_indices: inferred test_size must be > 0");
  PLUSS_CHECK(
      n_folds <= sample_count,
      "pluss::temporal_split_indices: number of folds cannot exceed sample_count");
  PLUSS_CHECK(
      sample_count - options.gap - (test_size * folds) > 0,
      "pluss::temporal_split_indices: too many splits for sample_count/test_size/gap");

  TemporalIndexSplit out;
  out.folds.reserve(static_cast<std::size_t>(folds));

  for (std::int64_t test_start = sample_count - folds * test_size;
       test_start < sample_count;
       test_start += test_size) {
    const std::int64_t train_end = test_start - options.gap;
    std::int64_t train_start = 0;
    if (options.max_train_size.has_value() && *options.max_train_size < train_end) {
      train_start = train_end - *options.max_train_size;
    }

    std::vector<std::int64_t> train_ids(static_cast<std::size_t>(train_end - train_start));
    std::iota(train_ids.begin(), train_ids.end(), train_start);

    std::vector<std::int64_t> test_ids(static_cast<std::size_t>(test_size));
    std::iota(test_ids.begin(), test_ids.end(), test_start);

    out.folds.push_back(TemporalIndexFold{
        .train_indices = std::move(train_ids),
        .test_indices = std::move(test_ids),
    });
  }

  return out;
}

inline TemporalFieldSplit temporal_split_fields(
    const std::vector<Series>& fields,
    TemporalSplitOptions options = {}) {
  std::int64_t sample_count = 0;
  const auto dims = detail::split_dims_for_fields_(fields, options.batch_dim, &sample_count);
  auto index_plan = temporal_split_indices(sample_count, options);

  TemporalFieldSplit out;
  out.folds.reserve(index_plan.folds.size());

  for (auto& fold : index_plan.folds) {
    out.folds.push_back(TemporalFieldFold{
        .train_fields = detail::index_fields_(fields, dims, fold.train_indices),
        .test_fields = detail::index_fields_(fields, dims, fold.test_indices),
    });
  }

  return out;
}

inline TemporalFieldSplit temporal_split_fields(
    std::initializer_list<Series> fields,
    TemporalSplitOptions options = {}) {
  return temporal_split_fields(std::vector<Series>(fields), options);
}

inline TemporalFieldSplit temporal_split_fields(
    const SeriesDataset& dataset,
    TemporalSplitOptions options = {}) {
  options.batch_dim = dataset.batch_dim();
  return temporal_split_fields(dataset.fields(), options);
}

inline TemporalFieldSplit temporal_split_fields(
    const Dataset& dataset,
    TemporalSplitOptions options = {}) {
  options.batch_dim = dataset.batch_dim();
  return temporal_split_fields(detail::materialize_dataset_fields_(dataset), options);
}

inline TemporalLoaderSplit temporal_split_loaders(
    const std::vector<Series>& fields,
    TemporalSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  auto split = temporal_split_fields(fields, split_options);
  train_options.batch_dim = split_options.batch_dim;
  test_options.batch_dim = split_options.batch_dim;

  TemporalLoaderSplit out;
  out.folds.reserve(split.folds.size());
  for (auto& fold : split.folds) {
    out.folds.push_back(TemporalLoaderFold{
        .train = DataLoader(std::move(fold.train_fields), train_options),
        .test = DataLoader(std::move(fold.test_fields), test_options),
    });
  }
  return out;
}

inline TemporalLoaderSplit temporal_split_loaders(
    std::initializer_list<Series> fields,
    TemporalSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  return temporal_split_loaders(std::vector<Series>(fields), split_options, train_options, test_options);
}

inline TemporalLoaderSplit temporal_split_loaders(
    const SeriesDataset& dataset,
    TemporalSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  split_options.batch_dim = dataset.batch_dim();
  return temporal_split_loaders(dataset.fields(), split_options, train_options, test_options);
}

inline TemporalLoaderSplit temporal_split_loaders(
    const Dataset& dataset,
    TemporalSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  split_options.batch_dim = dataset.batch_dim();
  return temporal_split_loaders(
      detail::materialize_dataset_fields_(dataset), split_options, train_options, test_options);
}

inline DataLoaderSplit split_loaders(
    const std::vector<Series>& fields,
    DataSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions val_options = {},
    DataLoaderOptions test_options = {}) {
  auto split = split_fields(fields, split_options);
  train_options.batch_dim = split_options.batch_dim;
  val_options.batch_dim = split_options.batch_dim;
  test_options.batch_dim = split_options.batch_dim;
  return DataLoaderSplit{
      .train = DataLoader(std::move(split.train_fields), train_options),
      .val = DataLoader(std::move(split.val_fields), val_options),
      .test = DataLoader(std::move(split.test_fields), test_options)};
}

inline DataLoaderSplit split_loaders(
    std::initializer_list<Series> fields,
    DataSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions val_options = {},
    DataLoaderOptions test_options = {}) {
  return split_loaders(std::vector<Series>(fields), split_options, train_options, val_options, test_options);
}

inline DataSplit split_fields(const SeriesDataset& dataset, DataSplitOptions options = {}) {
  options.batch_dim = dataset.batch_dim();
  return split_fields(dataset.fields(), options);
}

inline DataSplit split_fields(const Dataset& dataset, DataSplitOptions options = {}) {
  options.batch_dim = dataset.batch_dim();
  return split_fields(detail::materialize_dataset_fields_(dataset), options);
}

inline DataLoaderSplit split_loaders(
    const SeriesDataset& dataset,
    DataSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions val_options = {},
    DataLoaderOptions test_options = {}) {
  split_options.batch_dim = dataset.batch_dim();
  return split_loaders(dataset.fields(), split_options, train_options, val_options, test_options);
}

inline DataLoaderSplit split_loaders(
    const Dataset& dataset,
    DataSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions val_options = {},
    DataLoaderOptions test_options = {}) {
  split_options.batch_dim = dataset.batch_dim();
  return split_loaders(
      detail::materialize_dataset_fields_(dataset), split_options, train_options, val_options, test_options);
}

} // namespace pluss::loader
