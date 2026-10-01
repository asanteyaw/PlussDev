#include <pluss/ai/ml.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-8) {
  return std::abs(a - b) <= tol;
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

bool approx_vec(const std::vector<double>& got, const std::vector<double>& want, double tol = 1e-8) {
  if (got.size() != want.size()) return false;
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (!approx(got[i], want[i], tol)) return false;
  }
  return true;
}

bool run_nodej_balance_checks(std::ostream* err = nullptr) {
  const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0}, {4}, f64());

  auto mix_or = class_mix(y);
  if (!mix_or.ok()) {
    if (err) *err << "class_mix should succeed on finite labels\n";
    return false;
  }
  const ClassMix& mix = mix_or.value();
  if (mix.sample_count != 4 || mix.class_count != 2 || !approx(mix.imbalance_ratio, 3.0)) {
    if (err) *err << "class_mix should expose sample count, class count, and imbalance ratio\n";
    return false;
  }
  if (!approx_vec(as_f64(mix.labels), {0.0, 1.0}) || !approx_vec(as_f64(mix.counts), {3.0, 1.0})) {
    if (err) *err << "class_mix should return sorted labels and class counts\n";
    return false;
  }

  TrainOptions balanced{};
  balanced.class_weight = ClassWeightMode::Balanced;
  auto balanced_or = resolve_weight_plan(y, pluss::series::undefined(), balanced);
  if (!balanced_or.ok()) {
    if (err) *err << "resolve_weight_plan should accept balanced class weighting\n";
    return false;
  }
  if (!approx_vec(as_f64(balanced_or.value().class_weights), {2.0 / 3.0, 2.0})
      || !approx_vec(as_f64(balanced_or.value().weights), {2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0, 2.0})) {
    if (err) *err << "balanced class weighting should follow inverse class frequency\n";
    return false;
  }

  TrainOptions custom{};
  custom.class_weight = ClassWeightMode::Custom;
  custom.class_weights = pluss::make_series(std::vector<double>{1.0, 4.0}, {2}, f64());
  auto custom_or = resolve_training_weights(y, pluss::series::undefined(), custom);
  if (!custom_or.ok()) {
    if (err) *err << "resolve_training_weights should accept custom class weights\n";
    return false;
  }
  if (!approx_vec(as_f64(custom_or.value()), {4.0 / 7.0, 4.0 / 7.0, 4.0 / 7.0, 16.0 / 7.0})) {
    if (err) *err << "custom class weights should map by sorted class labels and normalize mean weight\n";
    return false;
  }

  const Series batch_w = pluss::make_series(std::vector<double>{1.0, 2.0, 1.0, 1.0}, {4}, f64());
  TrainOptions stacked{};
  stacked.sample_weights = pluss::make_series(std::vector<double>{1.0, 1.0, 2.0, 1.0}, {4}, f64());
  auto stacked_or = resolve_training_weights(y, batch_w, stacked);
  if (!stacked_or.ok()) {
    if (err) *err << "resolve_training_weights should combine batch and train sample weights\n";
    return false;
  }
  if (!approx_vec(as_f64(stacked_or.value()), {2.0 / 3.0, 4.0 / 3.0, 4.0 / 3.0, 2.0 / 3.0})) {
    if (err) *err << "sample weights should multiply and normalize to mean one\n";
    return false;
  }

  const Series bad_w = pluss::make_series(std::vector<double>{1.0, 0.0, 1.0, 1.0}, {4}, f64());
  auto bad_or = resolve_training_weights(y, bad_w, TrainOptions{});
  if (bad_or.ok()) {
    if (err) *err << "resolve_training_weights should reject non-positive sample weights\n";
    return false;
  }

  BalanceTarget equalize{};
  equalize.kind = BalanceTargetKind::Equalize;
  auto plan_or = plan_balance(y, equalize, RandomState{.seed = 11, .deterministic = true});
  if (!plan_or.ok()) {
    if (err) *err << "plan_balance should produce an equalized row-index plan\n";
    return false;
  }
  if (plan_or.value().indices.numel() != 6 || !plan_or.value().uses_replacement) {
    if (err) *err << "equalized plan should oversample minority rows to majority count\n";
    return false;
  }
  if (!approx_vec(as_f64(plan_or.value().target_counts), {3.0, 3.0})
      || !approx_vec(as_f64(plan_or.value().after.counts), {3.0, 3.0})) {
    if (err) *err << "equalized plan should report balanced after-counts\n";
    return false;
  }

  const Series x = pluss::make_series(std::vector<double>{0.0, 10.0,
                                                          1.0, 11.0,
                                                          2.0, 12.0,
                                                          3.0, 13.0},
                                      {4, 2},
                                      f64());
  const LabeledBatch batch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
      .sample_weights = batch_w,
  };
  auto balanced_batch_or = balance_rows(batch, equalize, RandomState{.seed = 11, .deterministic = true});
  if (!balanced_batch_or.ok()) {
    if (err) *err << "balance_rows should materialize an equalized labeled batch\n";
    return false;
  }
  auto balanced_mix_or = class_mix(balanced_batch_or.value().y);
  if (!balanced_mix_or.ok() || !approx_vec(as_f64(balanced_mix_or.value().counts), {3.0, 3.0})) {
    if (err) *err << "balance_rows should materialize labels with equal class counts\n";
    return false;
  }
  if (balanced_batch_or.value().features.sample_count() != 6 || balanced_batch_or.value().sample_weights.numel() != 6) {
    if (err) *err << "balance_rows should slice features and sample weights consistently\n";
    return false;
  }

  BalanceTarget ratio{};
  ratio.kind = BalanceTargetKind::Ratio;
  ratio.ratio = 1.0;
  auto ratio_or = plan_balance(y, ratio, RandomState{.seed = 7, .deterministic = true});
  if (!ratio_or.ok() || !approx_vec(as_f64(ratio_or.value().after.counts), {3.0, 3.0})) {
    if (err) *err << "ratio target should lift the minority class to requested binary ratio\n";
    return false;
  }

  BalanceTarget counts{};
  counts.kind = BalanceTargetKind::Counts;
  counts.counts = pluss::make_series(std::vector<double>{2.0, 2.0}, {2}, f64());
  auto counts_batch_or = balance_rows(batch, counts, RandomState{.seed = 19, .deterministic = true});
  if (!counts_batch_or.ok()) {
    if (err) *err << "balance_rows should support explicit per-class counts\n";
    return false;
  }
  auto counts_mix_or = class_mix(counts_batch_or.value().y);
  if (!counts_mix_or.ok() || !approx_vec(as_f64(counts_mix_or.value().counts), {2.0, 2.0})) {
    if (err) *err << "explicit counts target should under/over sample to requested counts\n";
    return false;
  }

  const auto lymph_plan =
      pluss::ai::lymph::plan_balance(y, equalize, RandomState{.seed = 11, .deterministic = true});
  if (!approx_vec(as_f64(lymph_plan.after.counts), {3.0, 3.0})) {
    if (err) *err << "lymph plan_balance wrapper should expose NodeJ balance planning\n";
    return false;
  }

  const Series blend_x = pluss::make_series(std::vector<double>{0.0,
                                                                1.0,
                                                                2.0,
                                                                10.0,
                                                                14.0},
                                            {5, 1},
                                            f64());
  const Series blend_y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0}, {5}, f64());
  const LabeledBatch blend_batch{
      .features = FeatureBatch{.x = blend_x, .layout = ObservationLayout::Rows},
      .y = blend_y,
  };
  BalanceTarget blend_target{};
  blend_target.kind = BalanceTargetKind::Equalize;
  blend_target.growth = BalanceGrowthKind::BlendNeighbors;
  blend_target.neighbors = 1;
  auto labels_only_blend_or = plan_balance(blend_y, blend_target, RandomState{.seed = 23, .deterministic = true});
  if (labels_only_blend_or.ok() || labels_only_blend_or.status().code != StatusCode::InvalidArgument) {
    if (err) *err << "labels-only BlendNeighbors planning should reject instead of falling back to replacement\n";
    return false;
  }
  auto blend_plan_or = plan_balance(blend_batch, blend_target, RandomState{.seed = 23, .deterministic = true});
  if (!blend_plan_or.ok()) {
    if (err) *err << "batch-aware plan_balance should support BlendNeighbors growth\n";
    return false;
  }
  if (!blend_plan_or.value().uses_synthesis || blend_plan_or.value().uses_replacement ||
      blend_plan_or.value().synthetic_base_indices.numel() != 1) {
    if (err) *err << "BlendNeighbors should describe synthetic row instructions for minority growth\n";
    return false;
  }
  const auto lymph_blend_plan =
      pluss::ai::lymph::plan_balance(blend_batch, blend_target, RandomState{.seed = 23, .deterministic = true});
  if (!lymph_blend_plan.uses_synthesis || lymph_blend_plan.uses_replacement ||
      lymph_blend_plan.synthetic_base_indices.numel() != 1) {
    if (err) *err << "lymph BlendNeighbors metadata should match NodeJ synthesis/replacement semantics\n";
    return false;
  }
  auto blend_batch_or = balance_rows(blend_batch, blend_target, RandomState{.seed = 23, .deterministic = true});
  if (!blend_batch_or.ok()) {
    if (err) *err << "balance_rows should materialize BlendNeighbors rows\n";
    return false;
  }
  auto blend_mix_or = class_mix(blend_batch_or.value().y);
  if (!blend_mix_or.ok() || !approx_vec(as_f64(blend_mix_or.value().counts), {3.0, 3.0})) {
    if (err) *err << "BlendNeighbors materialization should equalize class counts\n";
    return false;
  }
  const auto blend_values = as_f64(blend_batch_or.value().features.x);
  if (blend_values.size() != 6 || !(blend_values.back() >= 10.0 && blend_values.back() <= 14.0)) {
    if (err) *err << "BlendNeighbors synthetic feature should lie between same-class neighbors\n";
    return false;
  }

  const Series clean_x = pluss::make_series(std::vector<double>{0.0,
                                                                0.04,
                                                                0.20,
                                                                5.0},
                                            {4, 1},
                                            f64());
  const Series clean_y = pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 0.0}, {4}, f64());
  const LabeledBatch clean_batch{
      .features = FeatureBatch{.x = clean_x, .layout = ObservationLayout::Rows},
      .y = clean_y,
  };

  BalanceTarget boundary_clean{};
  boundary_clean.clean = BalanceCleanKind::BoundaryPairs;
  auto boundary_plan_or = plan_balance(clean_batch, boundary_clean, RandomState{.seed = 31, .deterministic = true});
  if (!boundary_plan_or.ok()) {
    if (err) *err << "BoundaryPairs cleaning plan should succeed\n";
    return false;
  }
  if (!boundary_plan_or.value().uses_cleaning || boundary_plan_or.value().removed_indices.numel() != 1
      || !approx_vec(as_f64(boundary_plan_or.value().removed_indices), {0.0})
      || !approx_vec(as_f64(boundary_plan_or.value().after.counts), {2.0, 1.0})) {
    if (err) *err << "BoundaryPairs should remove the majority side of a reciprocal boundary pair\n";
    return false;
  }

  BalanceTarget clean_equalize{};
  clean_equalize.kind = BalanceTargetKind::Equalize;
  clean_equalize.clean = BalanceCleanKind::BoundaryPairs;
  auto clean_balanced_or = balance_rows(clean_batch, clean_equalize, RandomState{.seed = 37, .deterministic = true});
  if (!clean_balanced_or.ok()) {
    if (err) *err << "balance_rows should combine BoundaryPairs cleaning with Equalize\n";
    return false;
  }
  auto clean_balanced_mix_or = class_mix(clean_balanced_or.value().y);
  if (!clean_balanced_mix_or.ok() || !approx_vec(as_f64(clean_balanced_mix_or.value().counts), {2.0, 2.0})) {
    if (err) *err << "cleaning plus equalization should rebalance from the cleaned base rows\n";
    return false;
  }

  const Series agree_x = pluss::make_series(std::vector<double>{0.00,
                                                                0.03,
                                                                0.06,
                                                                4.0,
                                                                5.0},
                                            {5, 1},
                                            f64());
  const Series agree_y = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0, 0.0}, {5}, f64());
  const LabeledBatch agree_batch{
      .features = FeatureBatch{.x = agree_x, .layout = ObservationLayout::Rows},
      .y = agree_y,
  };
  BalanceTarget agreement_clean{};
  agreement_clean.clean = BalanceCleanKind::NeighborAgreement;
  agreement_clean.clean_neighbors = 2;
  agreement_clean.clean_vote = BalanceCleanVote::MajorityDisagreement;
  auto agreement_plan_or = plan_balance(agree_batch, agreement_clean, RandomState{.seed = 41, .deterministic = true});
  if (!agreement_plan_or.ok()) {
    if (err) *err << "NeighborAgreement cleaning plan should succeed: "
                  << agreement_plan_or.status().message << "\n";
    return false;
  }
  if (!agreement_plan_or.value().uses_cleaning || agreement_plan_or.value().removed_indices.numel() != 1
      || !approx_vec(as_f64(agreement_plan_or.value().removed_indices), {0.0})
      || !approx_vec(as_f64(agreement_plan_or.value().after.counts), {2.0, 2.0})) {
    if (err) *err << "NeighborAgreement should remove targetable rows whose neighbor vote disagrees\n";
    return false;
  }

  const auto lymph_clean_plan =
      pluss::ai::lymph::plan_balance(clean_batch, boundary_clean, RandomState{.seed = 31, .deterministic = true});
  if (!lymph_clean_plan.uses_cleaning || lymph_clean_plan.removed_indices.numel() != 1) {
    if (err) *err << "lymph plan_balance wrapper should expose batch cleaning plans\n";
    return false;
  }

  ModelSpec neighbor_spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Neighbor,
      .options = NeighborFamilyOptions{
          .k = 1,
          .standardize = false,
      },
  };
  auto classifier_or = make_classifier(neighbor_spec);
  if (!classifier_or.ok()) {
    if (err) *err << "make_classifier should construct neighbor classifier\n";
    return false;
  }
  TrainOptions integrated_train{};
  integrated_train.balance.kind = BalanceTargetKind::Equalize;
  integrated_train.random.seed = 53;
  integrated_train.random.deterministic = true;
  Status integrated_status = classifier_or.value()->learn(batch, integrated_train);
  if (!integrated_status.ok()) {
    if (err) *err << "classifier learn should accept TrainOptions.balance: "
                  << integrated_status.message << "\n";
    return false;
  }
  auto classifier_state_or = classifier_or.value()->dump_state();
  if (!classifier_state_or.ok()) {
    if (err) *err << "classifier dump_state should succeed after balanced learn\n";
    return false;
  }
  const auto& classifier_state = classifier_state_or.value();
  const auto classifier_meta = classifier_state.size() > 3 ? as_f64(classifier_state[3]) : std::vector<double>{};
  if (classifier_meta.size() < 2 || !approx(classifier_meta[1], 6.0)) {
    if (err) *err << "classifier learn should fit on the equalized row count; got "
                  << (classifier_meta.size() < 2 ? -1.0 : classifier_meta[1]) << "\n";
    return false;
  }

  pluss::ai::nodej::online::OnlineLinearOptions online_options{};
  online_options.family.objective = LinearObjective::Logistic;
  auto online_or = pluss::ai::nodej::online::make_online_linear_classifier(online_options);
  if (!online_or.ok()) {
    if (err) *err << "make_online_linear_classifier should succeed\n";
    return false;
  }
  auto online_model = online_or.move_value();
  TrainOptions online_train{};
  online_train.max_iterations = 3;
  online_train.balance.kind = BalanceTargetKind::Equalize;
  online_train.random.seed = 59;
  online_train.random.deterministic = true;
  Status online_status = online_model.learn(batch, online_train);
  if (!online_status.ok()) {
    if (err) *err << "online classifier learn should accept TrainOptions.balance: "
                  << online_status.message << "\n";
    return false;
  }
  if (online_model.seen_samples() != 18) {
    if (err) *err << "online classifier bootstrap should consume the equalized row count\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej balance core", "[nodej][balance]") {
  if (!run_nodej_balance_checks(&std::cerr)) {
    FAIL("nodej balance checks failed");
  }
}
#else
int main() {
  if (!run_nodej_balance_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej balance checks passed\n";
  return 0;
}
#endif
