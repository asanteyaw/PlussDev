#include <pluss/ml.h>

#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::SplitOptions;
using pluss::ai::nodej::prep::CategoryOverflowPolicy;
using pluss::ai::nodej::prep::FrequencyEncoder;
using pluss::ai::nodej::prep::FrequencyEncoderOptions;
using pluss::ai::nodej::prep::MinMaxScaler;
using pluss::ai::nodej::prep::OneHotEncoder;
using pluss::ai::nodej::prep::OneHotEncoderOptions;
using pluss::ai::nodej::prep::RobustScaler;
using pluss::ai::nodej::prep::SimpleImputer;
using pluss::ai::nodej::prep::SimpleImputerOptions;
using pluss::ai::nodej::prep::SimpleImputerStrategy;
using pluss::ai::nodej::prep::StandardScaler;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void print_shape(const std::string& name, const Series& x) {
  std::cout << name << " shape=[";
  const auto sizes = x.sizes();
  for (std::size_t i = 0; i < sizes.size(); ++i) {
    std::cout << sizes[i] << (i + 1 == sizes.size() ? "" : ", ");
  }
  std::cout << "]\n";
}

template <class T>
bool require_ok(const pluss::ai::nodej::StatusOr<T>& out, const std::string& label) {
  if (out.ok()) return true;
  std::cerr << label << " failed: " << out.status().message << "\n";
  return false;
}

bool require_status(const pluss::ai::nodej::Status& st, const std::string& label) {
  if (st.ok()) return true;
  std::cerr << label << " failed: " << st.message << "\n";
  return false;
}

} // namespace

int main() {
  std::cout << "NodeJ split + prep demo\n";

  const Series x_raw = pluss::make_series(std::vector<double>{
                                              1.0,  2.0, std::numeric_limits<double>::quiet_NaN(),
                                              2.0,  2.5, 1.0,
                                              3.0,  std::numeric_limits<double>::quiet_NaN(), 0.0,
                                              4.0,  3.5, 1.0,
                                              5.0,  4.0, 0.0,
                                              6.0,  4.5, 1.0},
                                          {6, 3},
                                          f64());
  const FeatureBatch dense{.x = x_raw, .layout = ObservationLayout::Rows};
  const Series labels = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());

  SplitOptions split{};
  split.holdout_fraction = 0.33;
  split.random.seed = 7;

  auto holdout_or = pluss::ai::nodej::holdout_split(dense.sample_count(), split);
  if (!require_ok(holdout_or, "holdout_split")) return 1;
  std::cout << "holdout train idx count=" << holdout_or.value().train_indices.numel()
            << " test idx count=" << holdout_or.value().test_indices.numel() << "\n";

  auto strat_or = pluss::ai::nodej::strat_holdout_split(labels, split);
  if (!require_ok(strat_or, "strat_holdout_split")) return 1;
  std::cout << "strat holdout train idx count=" << strat_or.value().train_indices.numel()
            << " test idx count=" << strat_or.value().test_indices.numel() << "\n";

  split.folds = 3;
  auto fold_or = pluss::ai::nodej::fold_plan(dense.sample_count(), split);
  if (!require_ok(fold_or, "fold_plan")) return 1;
  std::cout << "kfold folds=" << fold_or.value().folds.size() << "\n";

  SimpleImputer imputer(SimpleImputerOptions{
      .strategy = SimpleImputerStrategy::Median,
      .constant_value = 0.0,
      .empty_feature_fallback = 0.0,
  });
  auto imp_or = imputer.learn_map(dense);
  if (!require_ok(imp_or, "SimpleImputer.learn_map")) return 1;
  const Series x_imputed = imp_or.move_value();
  print_shape("imputed", x_imputed);

  StandardScaler zscore{};
  auto z_or = zscore.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(z_or, "StandardScaler.learn_map")) return 1;
  const Series x_z = z_or.move_value();
  print_shape("standardized", x_z);

  MinMaxScaler minmax{};
  auto mm_or = minmax.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(mm_or, "MinMaxScaler.learn_map")) return 1;
  const Series x_mm = mm_or.move_value();
  print_shape("minmax", x_mm);

  RobustScaler robust{};
  auto rb_or = robust.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(rb_or, "RobustScaler.learn_map")) return 1;
  const Series x_rb = rb_or.move_value();
  print_shape("robust", x_rb);

  const Series x_query = pluss::make_series(std::vector<double>{
                                                7.0, 5.0, 1.0,
                                                8.0, 5.5, 0.0},
                                            {2, 3},
                                            f64());
  auto mapped_or = zscore.map(FeatureBatch{.x = x_query, .layout = ObservationLayout::Rows});
  if (!require_ok(mapped_or, "StandardScaler.map")) return 1;
  print_shape("standardized query", mapped_or.value());

  const Series categories = pluss::make_series(std::vector<double>{
                                                   0.0, 1.0,
                                                   1.0, 1.0,
                                                   2.0, 0.0,
                                                   1.0, 2.0,
                                                   0.0, 2.0,
                                                   2.0, 1.0},
                                               {6, 2},
                                               f64());
  const FeatureBatch cat_batch{.x = categories, .layout = ObservationLayout::Rows};

  OneHotEncoder one_hot(OneHotEncoderOptions{
      .max_categories = 8,
      .overflow = CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
  });
  auto one_hot_or = one_hot.learn_map(cat_batch);
  if (!require_ok(one_hot_or, "OneHotEncoder.learn_map")) return 1;
  print_shape("one-hot", one_hot_or.value());

  FrequencyEncoder freq(FrequencyEncoderOptions{
      .max_categories = 8,
      .overflow = CategoryOverflowPolicy::KeepMostFrequent,
      .unknown_frequency = 0.0,
  });
  auto freq_or = freq.learn_map(cat_batch);
  if (!require_ok(freq_or, "FrequencyEncoder.learn_map")) return 1;
  print_shape("frequency-encoded", freq_or.value());

  std::cout << "NodeJ split + prep demo completed.\n";
  return 0;
}
