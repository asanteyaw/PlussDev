#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

static std::uint64_t fnv1a64_append_u64(std::uint64_t h, std::uint64_t v) {
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  for (int b = 0; b < 8; ++b) {
    const std::uint8_t byte = static_cast<std::uint8_t>((v >> (8 * b)) & 0xFFu);
    h ^= static_cast<std::uint64_t>(byte);
    h *= kPrime;
  }
  return h;
}

static std::uint64_t hash_i64_vec(const std::vector<std::int64_t>& v) {
  std::uint64_t h = 1469598103934665603ULL;
  h = fnv1a64_append_u64(h, static_cast<std::uint64_t>(v.size()));
  for (std::int64_t x : v) {
    h = fnv1a64_append_u64(h, static_cast<std::uint64_t>(x));
  }
  return h;
}

int main() {
  using namespace pluss;
  using namespace pluss::ai;

  const std::string csv = "/Users/yawasante/software/cxx/PlussDev/PlussMic/data/housing.csv";
  auto housing = table::DataFrame::load_csv(csv);

  housing.set("income_cat") = housing.features("median_income").bucketize(
      {0.0, 1.5, 3.0, 4.5, 6.0, std::numeric_limits<double>::infinity()},
      {1, 2, 3, 4, 5});

  auto split = lymph::make_holdout_indices(
      housing,
      lymph::HoldoutSplitOptions{
          .test_size = 0.2,
          .stratify_feature = "income_cat",
          .random_state = 42,
      });

  auto test_idx = split.test_indices.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  auto train_idx = split.train_indices.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  auto test_idx_sorted = test_idx;
  std::sort(test_idx_sorted.begin(), test_idx_sorted.end());

  std::cout << "HOLDOUT train=" << train_idx.size() << " test=" << test_idx.size() << "\n";
  std::cout << "HOLDOUT_TEST_HEAD";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, test_idx.size()); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << test_idx_sorted[i];
  }
  std::cout << "\n";
  std::cout << "HOLDOUT_HASH train=" << hash_i64_vec(train_idx)
            << " test=" << hash_i64_vec(test_idx) << "\n";
  std::cout << "HOLDOUT_TRAIN_ORDER_HEAD";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, train_idx.size()); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << train_idx[i];
  }
  std::cout << "\n";
  std::cout << "HOLDOUT_TEST_ORDER_HEAD";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, test_idx.size()); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << test_idx[i];
  }
  std::cout << "\n";

  auto train_df = housing.take(train_idx);
  auto null_rows = train_df.isnull().any().as_series().to(pluss::pCPU, ScalarType::Bool).to_vec<bool>();
  std::vector<std::int64_t> null_pos{};
  std::vector<std::int64_t> null_orig_idx{};
  for (std::size_t i = 0; i < null_rows.size() && null_pos.size() < 5; ++i) {
    if (!null_rows[i]) continue;
    null_pos.push_back(static_cast<std::int64_t>(i));
    null_orig_idx.push_back(train_idx[i]);
  }
  std::cout << "NULL_HEAD_POS";
  for (std::size_t i = 0; i < null_pos.size(); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << null_pos[i];
  }
  std::cout << "\n";
  std::cout << "NULL_HEAD_ORIG_INDEX";
  for (std::size_t i = 0; i < null_orig_idx.size(); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << null_orig_idx[i];
  }
  std::cout << "\n";

  auto corr_ranked = train_df.corr().arrange({"median_house_value"}, false).features({"median_house_value"});
  auto corr_vals = corr_ranked.features("median_house_value").as_series().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::cout << "CORR_MHV_DESC";
  for (std::size_t i = 0; i < corr_vals.size(); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << corr_vals[i];
  }
  std::cout << "\n";

  lymph::prep::OutlierIsolatorOptions out_opts{};
  out_opts.random.seed = 42;
  out_opts.random.deterministic = true;
  out_opts.estimators = 100;
  out_opts.max_samples_auto = true;
  out_opts.max_features_ratio = 1.0;
  out_opts.bootstrap = false;
  out_opts.contamination_auto = true;

  auto isolator = lymph::prep::outlier_isolator(out_opts);
  auto labels_thread = isolator.learn_infer(train_df.feataxis().series());
  auto scores_thread = isolator.score_samples(train_df.feataxis().series());

  auto labels = labels_thread.as_series().to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  auto scores = scores_thread.as_series().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

  std::int64_t inliers = 0;
  std::int64_t outliers = 0;
  for (std::int64_t y : labels) {
    if (y == 1) ++inliers;
    else if (y == -1) ++outliers;
  }

  std::cout << "OUTLIER_COUNTS inlier=" << inliers << " outlier=" << outliers << " total=" << labels.size() << "\n";
  std::cout << "OUTLIER_LABEL_HEAD";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, labels.size()); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << labels[i];
  }
  std::cout << "\n";
  std::cout << "OUTLIER_SCORE_HEAD";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, scores.size()); ++i) {
    std::cout << (i == 0 ? ' ' : ',') << scores[i];
  }
  std::cout << "\n";

  return 0;
}
