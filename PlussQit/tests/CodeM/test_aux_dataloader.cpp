#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Series make_x() {
  std::vector<float> data;
  data.reserve(20);
  for (int i = 0; i < 20; ++i) data.push_back(static_cast<float>(i));
  return pluss::make_series(data, {10, 2}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
}

Series make_y() {
  std::vector<float> data;
  data.reserve(10);
  for (int i = 0; i < 10; ++i) data.push_back((i % 2) ? 1.0f : 0.0f);
  return pluss::make_series(data, {10, 1}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
}

} // namespace

TEST_CASE("aux dataloader batches map-style Series fields", "[aux][dataloader]") {
  const Series x = make_x();
  const Series y = make_y();

  pluss::loader::DataLoader loader({x, y}, pluss::loader::DataLoaderOptions{
                                       .batch_size = 4,
                                       .shuffle = false,
                                       .drop_last = false,
                                   });

  REQUIRE(loader.sample_count() == 10);
  REQUIRE(loader.batch_count() == 3);

  auto first = loader.batch_at(0);
  REQUIRE(first.size() == 2);
  REQUIRE(first.x().sizes()[0] == 4);
  REQUIRE(first.x().sizes()[1] == 2);
  REQUIRE(first.y().sizes()[0] == 4);
  REQUIRE(first.y().sizes()[1] == 1);
  const auto x0 = first.x().to_vec<float>();
  REQUIRE(x0.size() == 8);
  REQUIRE(x0[0] == Approx(0.0f));
  REQUIRE(x0[1] == Approx(1.0f));
  REQUIRE(x0[6] == Approx(6.0f));
  REQUIRE(x0[7] == Approx(7.0f));

  std::size_t seen = 0;
  for (auto batch : loader) {
    seen += static_cast<std::size_t>(batch.x().sizes()[0]);
  }
  REQUIRE(seen == 10);
}

TEST_CASE("aux dataloader supports deterministic shuffling and drop_last", "[aux][dataloader]") {
  const Series x = make_x();
  const Series y = make_y();

  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.shuffle = true;
  options.drop_last = true;
  options.seed = 99ULL;

  pluss::loader::DataLoader a({x, y}, options);
  pluss::loader::DataLoader b({x, y}, options);

  REQUIRE(a.batch_count() == 2);
  REQUIRE(b.batch_count() == 2);

  const auto first_a = (*a.begin()).x().to_vec<float>();
  const auto first_b = (*b.begin()).x().to_vec<float>();
  REQUIRE(first_a == first_b);
}

TEST_CASE("aux split_fields and split_loaders provide train val test partitioning", "[aux][dataloader]") {
  const Series x = make_x();
  const Series y = make_y();

  const auto split = pluss::loader::split_fields(
      {x, y},
      pluss::loader::DataSplitOptions{
          .train_ratio = 0.6,
          .val_ratio = 0.2,
          .shuffle = false,
      });

  REQUIRE(split.train_fields.size() == 2);
  REQUIRE(split.val_fields.size() == 2);
  REQUIRE(split.test_fields.size() == 2);
  REQUIRE(split.train_fields[0].sizes()[0] == 6);
  REQUIRE(split.val_fields[0].sizes()[0] == 2);
  REQUIRE(split.test_fields[0].sizes()[0] == 2);

  auto loaders = pluss::loader::split_loaders(
      {x, y},
      pluss::loader::DataSplitOptions{
          .train_ratio = 0.6,
          .val_ratio = 0.2,
          .shuffle = true,
          .seed = 7ULL,
      },
      pluss::loader::DataLoaderOptions{
          .batch_size = 4,
          .shuffle = true,
          .seed = 11ULL,
      },
      pluss::loader::DataLoaderOptions{
          .batch_size = 2,
      },
      pluss::loader::DataLoaderOptions{
          .batch_size = 2,
      });

  REQUIRE(loaders.train.sample_count() == 6);
  REQUIRE(loaders.val.sample_count() == 2);
  REQUIRE(loaders.test.sample_count() == 2);
}

TEST_CASE("aux temporal_split provides reference-style expanding or rolling windows", "[aux][dataloader]") {
  const auto plan = pluss::loader::temporal_split_indices(
      10,
      pluss::loader::TemporalSplitOptions{
          .folds = 3,
          .test_size = 2,
          .max_train_size = 3,
          .gap = 0,
      });

  REQUIRE(plan.folds.size() == 3);
  REQUIRE(plan.folds[0].train_indices == std::vector<std::int64_t>{1, 2, 3});
  REQUIRE(plan.folds[0].test_indices == std::vector<std::int64_t>{4, 5});
  REQUIRE(plan.folds[1].train_indices == std::vector<std::int64_t>{3, 4, 5});
  REQUIRE(plan.folds[1].test_indices == std::vector<std::int64_t>{6, 7});
  REQUIRE(plan.folds[2].train_indices == std::vector<std::int64_t>{5, 6, 7});
  REQUIRE(plan.folds[2].test_indices == std::vector<std::int64_t>{8, 9});

  const Series x = make_x();
  const Series y = make_y();
  const auto split = pluss::loader::temporal_split_fields(
      {x, y},
      pluss::loader::TemporalSplitOptions{
          .folds = 3,
          .test_size = 2,
          .max_train_size = 3,
          .gap = 0,
      });

  REQUIRE(split.folds.size() == 3);
  REQUIRE(split.folds[0].train_fields.size() == 2);
  REQUIRE(split.folds[0].test_fields.size() == 2);
  REQUIRE(split.folds[0].train_fields[0].sizes()[0] == 3);
  REQUIRE(split.folds[0].test_fields[0].sizes()[0] == 2);
}

TEST_CASE("aux dataloader supports collate_fn and target dtype transfer", "[aux][dataloader]") {
  const Series x = make_x();
  const Series y = make_y();

  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.target_dtype = ScalarType::Float64;
  options.collate_fn = [](pluss::loader::DataBatch batch) {
    batch.fields.push_back(batch.x().mean(1, true));
    return batch;
  };

  pluss::loader::DataLoader loader({x, y}, options);
  const auto batch = loader.batch_at(0);
  REQUIRE(batch.size() == 3);
  REQUIRE(batch.x().scalar_type() == ScalarType::Float64);
  REQUIRE(batch.y().scalar_type() == ScalarType::Float64);
  REQUIRE(batch[2].scalar_type() == ScalarType::Float64);
}

TEST_CASE("aux seriesdataset supports row access and dataloader construction", "[aux][dataloader]") {
  const Series x = make_x();
  const Series y = make_y();

  pluss::loader::SeriesDataset dataset({x, y});
  REQUIRE(dataset.size() == 10);

  const auto row = dataset.get(3);
  REQUIRE(row.size() == 2);
  REQUIRE(row.x().sizes()[0] == 1);
  REQUIRE(row.y().sizes()[0] == 1);

  pluss::loader::DataLoader loader(dataset, pluss::loader::DataLoaderOptions{
                                        .batch_size = 5,
                                        .shuffle = false,
                                    });
  REQUIRE(loader.sample_count() == 10);
  REQUIRE(loader.batch_count() == 2);
}

namespace {

// A minimal Dataset: only get()/size() implemented, materialize_fields()
// left at its unimplemented (empty) default -- exercises DataLoader's
// per-sample get()-based fallback path directly, rather than the
// vectorized materialize_fields() one every other Dataset test above
// goes through.
class ScalarOnlyDataset final : public pluss::loader::Dataset {
 public:
  explicit ScalarOnlyDataset(std::vector<double> values) : values_(std::move(values)) {}

  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }

  pluss::loader::DataBatch get(std::int64_t index) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }

 private:
  std::vector<double> values_;
};

class EmptyDataset final : public pluss::loader::Dataset {
 public:
  std::int64_t size() const override { return 3; }
  pluss::loader::DataBatch get(std::int64_t) const override { return {}; }
};

} // namespace

TEST_CASE(
    "aux dataloader falls back to get() when materialize_fields() is left unimplemented",
    "[aux][dataloader]") {
  ScalarOnlyDataset dataset({10.0, 20.0, 30.0, 40.0, 50.0});
  REQUIRE(dataset.materialize_fields().empty());

  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 2;
  pluss::loader::DataLoader loader(dataset, options);

  REQUIRE(loader.sample_count() == 5);
  REQUIRE(loader.batch_count() == 3);

  std::vector<double> seen;
  for (auto batch : loader) {
    const auto values = batch.x().to_vec<double>();
    seen.insert(seen.end(), values.begin(), values.end());
  }
  REQUIRE(seen == std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0});
}

TEST_CASE(
    "aux dataloader still throws a clear error when both materialize_fields() and get() are empty",
    "[aux][dataloader]") {
  EmptyDataset dataset;
  REQUIRE_THROWS(pluss::loader::DataLoader(dataset));
}

namespace {

// 20 rows this time (not 10) so num_workers up to 4 each get real work.
Series make_big_x() {
  std::vector<float> data;
  data.reserve(40);
  for (int i = 0; i < 40; ++i) data.push_back(static_cast<float>(i));
  return pluss::make_series(data, {20, 2}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
}

std::vector<float> collect_first_column(pluss::loader::DataLoader& loader) {
  std::vector<float> seen;
  for (auto batch : loader) {
    const auto col = batch.x().to_vec<float>();
    for (std::size_t i = 0; i < col.size(); i += 2) seen.push_back(col[i]);
  }
  return seen;
}

} // namespace

TEST_CASE("aux dataloader num_workers>0 matches the synchronous result exactly", "[aux][dataloader][threads]") {
  const Series x = make_big_x();

  pluss::loader::DataLoaderOptions sync_opts{};
  sync_opts.batch_size = 3;
  sync_opts.shuffle = true;
  sync_opts.seed = 42ULL;

  pluss::loader::DataLoaderOptions threaded_opts = sync_opts;
  threaded_opts.num_workers = 4;

  pluss::loader::DataLoader sync_loader({x}, sync_opts);
  pluss::loader::DataLoader threaded_loader({x}, threaded_opts);

  const auto sync_seen = collect_first_column(sync_loader);
  const auto threaded_seen = collect_first_column(threaded_loader);

  REQUIRE(sync_seen.size() == 20);
  REQUIRE(threaded_seen == sync_seen);
}

TEST_CASE("aux dataloader num_workers>0 is repeatable across many runs (no data race)", "[aux][dataloader][threads]") {
  const Series x = make_big_x();

  pluss::loader::DataLoaderOptions opts{};
  opts.batch_size = 3;
  opts.shuffle = true;
  opts.seed = 7ULL;
  opts.num_workers = 4;
  opts.prefetch_factor = 3;

  std::vector<float> baseline;
  for (int run = 0; run < 20; ++run) {
    pluss::loader::DataLoader loader({x}, opts);
    const auto seen = collect_first_column(loader);
    REQUIRE(seen.size() == 20);
    if (run == 0) {
      baseline = seen;
    } else {
      REQUIRE(seen == baseline);
    }
  }
}

TEST_CASE("aux dataloader persistent_workers reuses threads across epochs", "[aux][dataloader][threads]") {
  const Series x = make_big_x();

  pluss::loader::DataLoaderOptions opts{};
  opts.batch_size = 4;
  opts.shuffle = false;
  opts.num_workers = 2;
  opts.persistent_workers = true;

  pluss::loader::DataLoader loader({x}, opts);

  const auto epoch1 = collect_first_column(loader);
  const auto epoch2 = collect_first_column(loader);
  const auto epoch3 = collect_first_column(loader);

  REQUIRE(epoch1.size() == 20);
  REQUIRE(epoch2.size() == 20);
  REQUIRE(epoch3.size() == 20);
  // shuffle=false -- every epoch visits the same order
  REQUIRE(epoch1 == epoch2);
  REQUIRE(epoch2 == epoch3);
}

TEST_CASE("aux dataloader worker_init_fn is called once per worker with a distinct id", "[aux][dataloader][threads]") {
  const Series x = make_big_x();

  std::mutex mu;
  std::vector<std::size_t> seen_ids;

  pluss::loader::DataLoaderOptions opts{};
  opts.batch_size = 3;
  opts.num_workers = 3;
  opts.worker_init_fn = [&](std::size_t worker_id) {
    std::lock_guard<std::mutex> lock(mu);
    seen_ids.push_back(worker_id);
  };

  pluss::loader::DataLoader loader({x}, opts);
  (void)collect_first_column(loader);

  const std::set<std::size_t> unique_ids(seen_ids.begin(), seen_ids.end());
  REQUIRE(seen_ids.size() == 3);
  REQUIRE(unique_ids == std::set<std::size_t>{0, 1, 2});
}

TEST_CASE("aux dataloader in_order=false still delivers every sample exactly once", "[aux][dataloader][threads]") {
  const Series x = make_big_x();

  pluss::loader::DataLoaderOptions opts{};
  opts.batch_size = 3;
  opts.num_workers = 4;
  opts.in_order = false;

  pluss::loader::DataLoader loader({x}, opts);
  auto seen = collect_first_column(loader);
  std::sort(seen.begin(), seen.end());

  std::vector<float> expected;
  for (int i = 0; i < 20; ++i) expected.push_back(static_cast<float>(i * 2));
  REQUIRE(seen == expected);
}

TEST_CASE("aux dataloader pin_memory does not alter batch values", "[aux][dataloader]") {
  const Series x = make_x();
  const Series y = make_y();

  pluss::loader::DataLoaderOptions without_pin{};
  without_pin.batch_size = 4;
  without_pin.shuffle = false;

  pluss::loader::DataLoaderOptions with_pin = without_pin;
  with_pin.pin_memory = true;

  pluss::loader::DataLoader a({x, y}, without_pin);
  pluss::loader::DataLoader b({x, y}, with_pin);

  REQUIRE(a.batch_at(0).x().to_vec<float>() == b.batch_at(0).x().to_vec<float>());
}

TEST_CASE("aux dataloader rejects invalid num_workers/prefetch/persistent combinations", "[aux][dataloader]") {
  const Series x = make_x();

  pluss::loader::DataLoaderOptions prefetch_without_workers{};
  prefetch_without_workers.prefetch_factor = 2;
  REQUIRE_THROWS(pluss::loader::DataLoader({x}, prefetch_without_workers));

  pluss::loader::DataLoaderOptions persistent_without_workers{};
  persistent_without_workers.persistent_workers = true;
  REQUIRE_THROWS(pluss::loader::DataLoader({x}, persistent_without_workers));

  pluss::loader::DataLoaderOptions negative_timeout{};
  negative_timeout.timeout = -1.0;
  REQUIRE_THROWS(pluss::loader::DataLoader({x}, negative_timeout));

  pluss::loader::DataLoaderOptions sampler_and_shuffle{};
  sampler_and_shuffle.shuffle = true;
  sampler_and_shuffle.sampler = std::make_shared<pluss::loader::SequentialSampler>();
  REQUIRE_THROWS(pluss::loader::DataLoader({x}, sampler_and_shuffle));
}

TEST_CASE("aux SequentialSampler and RandomSampler match torch-style semantics", "[aux][dataloader][sampler]") {
  pluss::loader::SequentialSampler seq;
  const auto seq_order = seq.sample(5, /*epoch_seed=*/0);
  REQUIRE(seq_order == std::vector<std::int64_t>{0, 1, 2, 3, 4});

  pluss::loader::RandomSampler rand_no_replacement(/*replacement=*/false);
  const auto rand_order = rand_no_replacement.sample(5, /*epoch_seed=*/123);
  auto sorted_order = rand_order;
  std::sort(sorted_order.begin(), sorted_order.end());
  REQUIRE(sorted_order == std::vector<std::int64_t>{0, 1, 2, 3, 4});

  // same seed -> same order; different seed -> (overwhelmingly likely) different order
  const auto rand_order_again = rand_no_replacement.sample(5, /*epoch_seed=*/123);
  REQUIRE(rand_order == rand_order_again);
  const auto rand_order_other_seed = rand_no_replacement.sample(5, /*epoch_seed=*/456);
  REQUIRE(rand_order != rand_order_other_seed);

  pluss::loader::RandomSampler rand_with_replacement(/*replacement=*/true, /*num_samples=*/10);
  const auto replacement_order = rand_with_replacement.sample(5, /*epoch_seed=*/9);
  REQUIRE(replacement_order.size() == 10);
  for (const auto idx : replacement_order) {
    REQUIRE(idx >= 0);
    REQUIRE(idx < 5);
  }
}

TEST_CASE("aux SubsetRandomSampler only draws from the given subset", "[aux][dataloader][sampler]") {
  pluss::loader::SubsetRandomSampler sampler(std::vector<std::int64_t>{2, 4, 6});
  const auto order = sampler.sample(/*size=*/10, /*epoch_seed=*/1);
  auto sorted_order = order;
  std::sort(sorted_order.begin(), sorted_order.end());
  REQUIRE(sorted_order == std::vector<std::int64_t>{2, 4, 6});
}

TEST_CASE("aux WeightedRandomSampler without replacement never repeats an index", "[aux][dataloader][sampler]") {
  pluss::loader::WeightedRandomSampler sampler(
      std::vector<double>{0.1, 0.9, 0.4, 0.7, 3.0}, /*num_samples=*/5, /*replacement=*/false);
  const auto order = sampler.sample(/*size=*/5, /*epoch_seed=*/3);
  auto sorted_order = order;
  std::sort(sorted_order.begin(), sorted_order.end());
  REQUIRE(sorted_order == std::vector<std::int64_t>{0, 1, 2, 3, 4});
}

TEST_CASE("aux dataloader propagates a worker's exception to the consumer instead of crashing", "[aux][dataloader][threads]") {
  const Series x = make_big_x();

  pluss::loader::DataLoaderOptions opts{};
  opts.batch_size = 3;
  opts.num_workers = 2;
  opts.collate_fn = [](pluss::loader::DataBatch) -> pluss::loader::DataBatch {
    throw std::runtime_error("deliberate collate_fn failure for the propagation test");
  };

  pluss::loader::DataLoader loader({x}, opts);
  bool threw = false;
  try {
    for (auto batch : loader) {
      (void)batch;
    }
  } catch (const std::exception&) {
    threw = true;
  }
  REQUIRE(threw);
}

TEST_CASE("aux dataloader accepts a custom Sampler in place of shuffle", "[aux][dataloader][sampler]") {
  const Series x = make_big_x();

  pluss::loader::DataLoaderOptions opts{};
  opts.batch_size = 20;
  opts.sampler = std::make_shared<pluss::loader::SubsetRandomSampler>(std::vector<std::int64_t>{0, 1, 2, 3, 4});

  pluss::loader::DataLoader loader({x}, opts);
  REQUIRE(loader.batch_count() == 1);

  const auto batch = loader.batch_at(0);
  REQUIRE(batch.x().sizes()[0] == 5);
}

TEST_CASE("aux scalar helpers provide concise scalar extraction", "[aux][scalars]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series d = pluss::make_series(3.25, cfg);
  REQUIRE(pluss::tils::as_f64(d) == Approx(3.25));

  const Series i = pluss::make_series(std::int64_t{42}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  REQUIRE(pluss::tils::as_i64(i) == 42);

  const auto exact_i64 = std::int64_t{9007199254740993LL};
  const auto exact_u64 = std::uint64_t{9007199254740993ULL};
  REQUIRE(pluss::tils::as_i64(pluss::tils::as_i64(exact_i64, Config{})) == exact_i64);
  REQUIRE(pluss::tils::as_u64(pluss::tils::as_u64(exact_u64, Config{})) == exact_u64);
}

TEST_CASE("aux pathing supports lean root+file API", "[aux][pathing]") {
  const auto custom = std::filesystem::temp_directory_path() / "pluss_aux_pathing_test";
  pluss::loader::set_root_path(custom);
  std::error_code ec_got;
  std::error_code ec_expected;
  const auto got_root = std::filesystem::weakly_canonical(pluss::loader::root_path(), ec_got);
  const auto expected_root = std::filesystem::weakly_canonical(custom, ec_expected);
  REQUIRE_FALSE(ec_got);
  REQUIRE_FALSE(ec_expected);
  REQUIRE(got_root == expected_root);
  REQUIRE(std::filesystem::path(pluss::loader::file_path("sample.csv").string()) ==
          (expected_root / "sample.csv"));

  const auto model_dir = pluss::loader::file_dir("trained_models", pluss::loader::PathOptions{
                                                                       .create_dirs = true,
                                                                   });
  REQUIRE(std::filesystem::exists(std::filesystem::path(model_dir.string())));
  REQUIRE(std::filesystem::path(model_dir.file("model.bin").string()) ==
          (expected_root / "trained_models" / "model.bin"));
  REQUIRE_THROWS(pluss::loader::file_path("../escape.txt").string());
}

TEST_CASE("aux pathing supports batch file path helpers", "[aux][pathing]") {
  const auto custom = std::filesystem::temp_directory_path() / "pluss_aux_pathing_batch_test";
  pluss::loader::set_root_path(custom);
  std::error_code ec;
  const auto expected_root = std::filesystem::weakly_canonical(custom, ec);
  REQUIRE_FALSE(ec);

  const auto paths = pluss::loader::file_paths({
      "all_data/housing.csv",
      "some_other_data/prices.csv",
      "all_other_data/fossils.xlsx",
  });
  REQUIRE(paths.size() == 3);
  REQUIRE(std::filesystem::path(paths[0].string()) == (expected_root / "all_data" / "housing.csv"));
  REQUIRE(std::filesystem::path(paths[1].string()) == (expected_root / "some_other_data" / "prices.csv"));
  REQUIRE(std::filesystem::path(paths[2].string()) == (expected_root / "all_other_data" / "fossils.xlsx"));

  const auto named = pluss::loader::file_path_map({
      {"housing", "all_data/housing.csv"},
      {"prices", "some_other_data/prices.csv"},
      {"fossils", "all_other_data/fossils.xlsx"},
  });
  REQUIRE(named.size() == 3);
  REQUIRE(std::filesystem::path(named.at("housing").string()) ==
          (expected_root / "all_data" / "housing.csv"));
  REQUIRE(std::filesystem::path(named.at("prices").string()) ==
          (expected_root / "some_other_data" / "prices.csv"));
  REQUIRE(std::filesystem::path(named.at("fossils").string()) ==
          (expected_root / "all_other_data" / "fossils.xlsx"));

  REQUIRE_THROWS(pluss::loader::file_path_map({
      {"dup", "a.csv"},
      {"dup", "b.csv"},
  }));
}

TEST_CASE("aux URL context supports internet-rooted file helpers", "[aux][pathing][url]") {
  const auto remote = pluss::loader::UrlContext::from_root(
      "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/");

  REQUIRE(remote.root() == "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main");
  REQUIRE(remote.file_path("example5.csv").string() ==
          "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/example5.csv");
  REQUIRE(remote.file_path("/unit1/example6.csv").string() ==
          "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/unit1/example6.csv");

  const auto urls = remote.file_paths({
      "example5.csv",
      "unit1/example6.csv",
  });
  REQUIRE(urls.size() == 2);
  REQUIRE(urls.at(0).string() ==
          "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/example5.csv");
  REQUIRE(urls.at(1).string() ==
          "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/unit1/example6.csv");

  const auto named = remote.file_path_map({
      {"housing", "example5.csv"},
      {"other", "unit1/example6.csv"},
  });
  REQUIRE(named.size() == 2);
  REQUIRE(named.at("housing").string() ==
          "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/example5.csv");
  REQUIRE(named.at("other").string() ==
          "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/unit1/example6.csv");

  REQUIRE_THROWS(remote.file_path("../escape.csv"));
  REQUIRE_THROWS(remote.file_path_map({
      {"dup", "a.csv"},
      {"dup", "b.csv"},
  }));
}
