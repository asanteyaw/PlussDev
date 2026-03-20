#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cstdint>
#include <filesystem>
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

TEST_CASE("aux time_series_split provides sklearn-style expanding or rolling windows", "[aux][dataloader]") {
  const auto plan = pluss::loader::time_series_split_indices(
      10,
      pluss::loader::TimeSeriesSplitOptions{
          .n_splits = 3,
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
  const auto split = pluss::loader::time_series_split_fields(
      {x, y},
      pluss::loader::TimeSeriesSplitOptions{
          .n_splits = 3,
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

TEST_CASE("aux scalar helpers provide concise scalar extraction", "[aux][scalars]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series d = pluss::make_series(3.25, cfg);
  REQUIRE(pluss::as_f64(d) == Approx(3.25));

  const Series i = pluss::make_series(std::int64_t{42}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  REQUIRE(pluss::as_i64(i) == 42);
  REQUIRE(pluss::as_I64(i) == 42);
}

TEST_CASE("aux pathing supports generic named roots", "[aux][pathing]") {
  const auto custom = std::filesystem::temp_directory_path() / "pluss_aux_pathing_test";
  pluss::loader::set_path_root("data", custom);
  REQUIRE(pluss::loader::has_path_root("data"));
  REQUIRE(pluss::loader::data_file("sample.csv") == (custom / "sample.csv"));

  pluss::loader::clear_path_root("data");
  REQUIRE_FALSE(pluss::loader::has_path_root("data"));
}
