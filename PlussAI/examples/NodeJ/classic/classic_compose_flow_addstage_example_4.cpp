#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  std::vector<cls::Status> results{};
  results.push_back(flow.add_standard_scaler());
  results.push_back(flow.add_minmax_scaler());
  results.push_back(flow.add_robust_scaler());
  results.push_back(flow.add_maxabs_scaler());
  results.push_back(flow.add_simple_imputer());
  results.push_back(flow.add_neighbor_imputer());
  results.push_back(flow.add_iterative_imputer());
  results.push_back(flow.add_one_hot_encoder());
  results.push_back(flow.add_ordinal_encoder());
  results.push_back(flow.add_label_encoder());
  results.push_back(flow.add_frequency_encoder());
  results.push_back(flow.add_basen_encoder());
  results.push_back(flow.add_hashing_encoder());
  results.push_back(flow.add_target_encoder());
  results.push_back(flow.add_woe_encoder());
  results.push_back(flow.add_vector_normalizer());
  results.push_back(flow.add_quantile_map());
  results.push_back(flow.add_power_map());
  results.push_back(flow.add_threshold_binarizer());
  results.push_back(flow.add_polynomial_lift());

  bool all_ok = true;
  for (const cls::Status& s : results) all_ok = all_ok && s.ok();
  std::cout << "all 20 ok() = " << std::boolalpha << all_ok << "\n";
  std::cout << "results.size() = " << results.size() << "\n";
  std::cout << "stage_count()  = " << flow.stage_count() << "\n";

  return 0;
}
