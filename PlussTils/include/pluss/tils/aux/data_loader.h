#pragma once

#include <CodeM/core/Errors.h>
#include <CodeM/core/ScalarType.h>
#include <CodeM/core/Series.h>
#include <pluss/codem/series_factory.h>
#include <pluss/config.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace pluss::aux {

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
      const auto cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(s.platform());
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
      PLUSS_CHECK(owner_ != nullptr, "pluss::DataLoader::iterator: invalid iterator dereference");
      return owner_->batch_at(index_);
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
      : DataLoader(fields_from_dataset_(dataset), with_dataset_dim_(std::move(options), dataset.batch_dim())) {}

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
    if (sample_count_ <= 0 || options_.batch_size <= 0) return 0;
    const std::int64_t full = sample_count_ / options_.batch_size;
    const std::int64_t rem = sample_count_ % options_.batch_size;
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
    if (reshuffle && options_.shuffle) {
      shuffle_order_();
    }
    return *this;
  }

  DataLoader& reset(bool reshuffle = true) {
    ++epoch_;
    cursor_ = 0;
    if (reshuffle && options_.shuffle) {
      shuffle_order_();
    }
    return *this;
  }

  std::optional<batch_type> next() {
    if (cursor_ >= batch_count()) return std::nullopt;
    const auto out = batch_at(cursor_);
    ++cursor_;
    return out;
  }

  batch_type batch_at(std::size_t batch_index) const {
    PLUSS_CHECK(batch_index < batch_count(), "pluss::DataLoader::batch_at: batch index out of range");
    const std::int64_t start = static_cast<std::int64_t>(batch_index) * options_.batch_size;
    std::int64_t count = options_.batch_size;
    if (!options_.drop_last) {
      count = std::min<std::int64_t>(count, sample_count_ - start);
    }
    return make_batch_(start, count);
  }

  iterator begin() {
    if (options_.reshuffle_each_epoch) {
      reset(/*reshuffle=*/options_.shuffle);
    } else {
      cursor_ = 0;
    }
    return iterator(this, 0);
  }

  iterator end() { return iterator(this, batch_count()); }

private:
  static std::vector<Series> fields_from_dataset_(const Dataset& dataset) {
    auto fields = dataset.materialize_fields();
    PLUSS_CHECK(
        !fields.empty(),
        "pluss::DataLoader(dataset): dataset must provide map-style fields via materialize_fields()");
    return fields;
  }

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
    PLUSS_CHECK(!fields_.empty(), "pluss::DataLoader: expected at least one Series field");
    PLUSS_CHECK(options_.batch_size > 0, "pluss::DataLoader: batch_size must be > 0");

    dims_.clear();
    dims_.reserve(fields_.size());

    const auto check_field = [&](const Series& s, std::size_t idx) {
      PLUSS_CHECK(s.defined(), "pluss::DataLoader: field[" + std::to_string(idx) + "] must be defined");
      PLUSS_CHECK(s.dim() >= 1, "pluss::DataLoader: field[" + std::to_string(idx) + "] must have rank >= 1");
    };

    check_field(fields_.front(), 0);
    const std::int64_t d0 = normalize_dim_(options_.batch_dim, fields_.front().dim());
    PLUSS_CHECK(
        d0 >= 0 && d0 < fields_.front().dim(),
        "pluss::DataLoader: batch_dim out of range for field[0]");
    sample_count_ = fields_.front().sizes()[d0];
    PLUSS_CHECK(sample_count_ >= 0, "pluss::DataLoader: invalid sample dimension");
    dims_.push_back(d0);

    for (std::size_t i = 1; i < fields_.size(); ++i) {
      check_field(fields_[i], i);
      const std::int64_t d = normalize_dim_(options_.batch_dim, fields_[i].dim());
      PLUSS_CHECK(
          d >= 0 && d < fields_[i].dim(),
          "pluss::DataLoader: batch_dim out of range for field[" + std::to_string(i) + "]");
      PLUSS_CHECK(
          fields_[i].sizes()[d] == sample_count_,
          "pluss::DataLoader: all fields must share identical sample count on batch_dim");
      dims_.push_back(d);
    }

    order_.resize(static_cast<std::size_t>(sample_count_));
    std::iota(order_.begin(), order_.end(), static_cast<std::int64_t>(0));
    cursor_ = 0;
    epoch_ = 0;
    if (options_.shuffle) {
      shuffle_order_();
    }
  }

  void shuffle_order_() {
    std::mt19937_64 rng(options_.seed + epoch_);
    std::shuffle(order_.begin(), order_.end(), rng);
  }

  batch_type make_batch_(std::int64_t start, std::int64_t count) const {
    PLUSS_CHECK(start >= 0, "pluss::DataLoader: internal batch start must be >= 0");
    PLUSS_CHECK(count >= 0, "pluss::DataLoader: internal batch count must be >= 0");
    PLUSS_CHECK(start + count <= sample_count_, "pluss::DataLoader: internal batch span out of bounds");

    std::vector<std::int64_t> ids(static_cast<std::size_t>(count));
    std::copy_n(order_.begin() + start, count, ids.begin());

    batch_type out;
    out.fields.reserve(fields_.size());
    for (std::size_t i = 0; i < fields_.size(); ++i) {
      const auto& s = fields_[i];
      const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(s.platform());
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
      return options_.collate_fn(std::move(out));
    }
    return out;
  }

  std::vector<Series> fields_{};
  DataLoaderOptions options_{};
  std::vector<std::int64_t> dims_{};
  std::vector<std::int64_t> order_{};
  std::int64_t sample_count_{0};
  std::uint64_t epoch_{0};
  std::size_t cursor_{0};
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
    const auto cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(s.platform());
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

struct TimeSeriesSplitOptions {
  std::int64_t n_splits{5};
  std::optional<std::int64_t> test_size{};
  std::optional<std::int64_t> max_train_size{};
  std::int64_t gap{0};
  std::int64_t batch_dim{0};
};

struct TimeSeriesIndexFold {
  std::vector<std::int64_t> train_indices{};
  std::vector<std::int64_t> test_indices{};
};

struct TimeSeriesIndexSplit {
  std::vector<TimeSeriesIndexFold> folds{};
};

struct TimeSeriesFieldFold {
  std::vector<Series> train_fields{};
  std::vector<Series> test_fields{};
};

struct TimeSeriesFieldSplit {
  std::vector<TimeSeriesFieldFold> folds{};
};

struct TimeSeriesLoaderFold {
  DataLoader train{};
  DataLoader test{};
};

struct TimeSeriesLoaderSplit {
  std::vector<TimeSeriesLoaderFold> folds{};
};

inline TimeSeriesIndexSplit time_series_split_indices(
    std::int64_t sample_count,
    TimeSeriesSplitOptions options = {}) {
  PLUSS_CHECK(sample_count >= 2, "pluss::time_series_split_indices: sample_count must be >= 2");
  PLUSS_CHECK(options.n_splits >= 2, "pluss::time_series_split_indices: n_splits must be >= 2");
  PLUSS_CHECK(options.gap >= 0, "pluss::time_series_split_indices: gap must be >= 0");
  if (options.test_size.has_value()) {
    PLUSS_CHECK(*options.test_size > 0, "pluss::time_series_split_indices: test_size must be > 0 when provided");
  }
  if (options.max_train_size.has_value()) {
    PLUSS_CHECK(
        *options.max_train_size > 0,
        "pluss::time_series_split_indices: max_train_size must be > 0 when provided");
  }

  const std::int64_t n_splits = options.n_splits;
  const std::int64_t n_folds = n_splits + 1;
  const std::int64_t test_size = options.test_size.value_or(sample_count / n_folds);

  PLUSS_CHECK(test_size > 0, "pluss::time_series_split_indices: inferred test_size must be > 0");
  PLUSS_CHECK(
      n_folds <= sample_count,
      "pluss::time_series_split_indices: number of folds cannot exceed sample_count");
  PLUSS_CHECK(
      sample_count - options.gap - (test_size * n_splits) > 0,
      "pluss::time_series_split_indices: too many splits for sample_count/test_size/gap");

  TimeSeriesIndexSplit out;
  out.folds.reserve(static_cast<std::size_t>(n_splits));

  for (std::int64_t test_start = sample_count - n_splits * test_size;
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

    out.folds.push_back(TimeSeriesIndexFold{
        .train_indices = std::move(train_ids),
        .test_indices = std::move(test_ids),
    });
  }

  return out;
}

inline TimeSeriesFieldSplit time_series_split_fields(
    const std::vector<Series>& fields,
    TimeSeriesSplitOptions options = {}) {
  std::int64_t sample_count = 0;
  const auto dims = detail::split_dims_for_fields_(fields, options.batch_dim, &sample_count);
  auto index_plan = time_series_split_indices(sample_count, options);

  TimeSeriesFieldSplit out;
  out.folds.reserve(index_plan.folds.size());

  for (auto& fold : index_plan.folds) {
    out.folds.push_back(TimeSeriesFieldFold{
        .train_fields = detail::index_fields_(fields, dims, fold.train_indices),
        .test_fields = detail::index_fields_(fields, dims, fold.test_indices),
    });
  }

  return out;
}

inline TimeSeriesFieldSplit time_series_split_fields(
    std::initializer_list<Series> fields,
    TimeSeriesSplitOptions options = {}) {
  return time_series_split_fields(std::vector<Series>(fields), options);
}

inline TimeSeriesFieldSplit time_series_split_fields(
    const SeriesDataset& dataset,
    TimeSeriesSplitOptions options = {}) {
  options.batch_dim = dataset.batch_dim();
  return time_series_split_fields(dataset.fields(), options);
}

inline TimeSeriesFieldSplit time_series_split_fields(
    const Dataset& dataset,
    TimeSeriesSplitOptions options = {}) {
  options.batch_dim = dataset.batch_dim();
  return time_series_split_fields(dataset.materialize_fields(), options);
}

inline TimeSeriesLoaderSplit time_series_split_loaders(
    const std::vector<Series>& fields,
    TimeSeriesSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  auto split = time_series_split_fields(fields, split_options);
  train_options.batch_dim = split_options.batch_dim;
  test_options.batch_dim = split_options.batch_dim;

  TimeSeriesLoaderSplit out;
  out.folds.reserve(split.folds.size());
  for (auto& fold : split.folds) {
    out.folds.push_back(TimeSeriesLoaderFold{
        .train = DataLoader(std::move(fold.train_fields), train_options),
        .test = DataLoader(std::move(fold.test_fields), test_options),
    });
  }
  return out;
}

inline TimeSeriesLoaderSplit time_series_split_loaders(
    std::initializer_list<Series> fields,
    TimeSeriesSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  return time_series_split_loaders(std::vector<Series>(fields), split_options, train_options, test_options);
}

inline TimeSeriesLoaderSplit time_series_split_loaders(
    const SeriesDataset& dataset,
    TimeSeriesSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  split_options.batch_dim = dataset.batch_dim();
  return time_series_split_loaders(dataset.fields(), split_options, train_options, test_options);
}

inline TimeSeriesLoaderSplit time_series_split_loaders(
    const Dataset& dataset,
    TimeSeriesSplitOptions split_options = {},
    DataLoaderOptions train_options = {},
    DataLoaderOptions test_options = {}) {
  split_options.batch_dim = dataset.batch_dim();
  return time_series_split_loaders(dataset.materialize_fields(), split_options, train_options, test_options);
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
  return split_fields(dataset.materialize_fields(), options);
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
  return split_loaders(dataset.materialize_fields(), split_options, train_options, val_options, test_options);
}

} // namespace pluss::aux

namespace pluss::loader {
using aux::Dataset;
using aux::SeriesDataset;
using aux::DataBatch;
using aux::DataLoader;
using aux::DataLoaderOptions;
using aux::DataLoaderSplit;
using aux::DataSplit;
using aux::DataSplitOptions;
using aux::TimeSeriesSplitOptions;
using aux::TimeSeriesIndexFold;
using aux::TimeSeriesIndexSplit;
using aux::TimeSeriesFieldFold;
using aux::TimeSeriesFieldSplit;
using aux::TimeSeriesLoaderFold;
using aux::TimeSeriesLoaderSplit;
using aux::split_fields;
using aux::split_loaders;
using aux::time_series_split_indices;
using aux::time_series_split_fields;
using aux::time_series_split_loaders;
} // namespace pluss::loader
