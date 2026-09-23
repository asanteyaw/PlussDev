#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // cluster 0 (label 0): 0..7; a stray label-1 point at -0.5 sits just
  // outside it, closest to row "0.0"; cluster 1 (label 1): 100,101,102,
  // far enough away to never affect anything in cluster 0.
  const pluss::Series x = pluss::make_series(
      {-0.5, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 100.0, 101.0, 102.0},
      {12}, f64).reshape({12, 1});
  const pluss::Series y = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget any_vote{};
  any_vote.clean = cls::BalanceCleanKind::NeighborAgreement;
  any_vote.clean_neighbors = 2;
  any_vote.clean_vote = cls::BalanceCleanVote::AnyDisagreement;
  any_vote.protect_small_classes = false;
  const cls::BalancePlan any_plan = cls::plan_balance(batch, any_vote).value();

  cls::BalanceTarget majority_vote = any_vote;
  majority_vote.clean_vote = cls::BalanceCleanVote::MajorityDisagreement;
  const cls::BalancePlan majority_plan = cls::plan_balance(batch, majority_vote).value();

  std::cout << "AnyDisagreement removed_indices      = " << any_plan.removed_indices << "\n";
  std::cout << "MajorityDisagreement removed_indices = " << majority_plan.removed_indices << "\n";

  return 0;
}
