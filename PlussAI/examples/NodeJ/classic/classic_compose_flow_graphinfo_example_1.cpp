#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  auto snapshot = [&](const char* label) {
    const cls::FlowGraphInfo info = flow.graph_info().value();
    std::cout << label << ": stages=" << info.stages.size()
              << " has_model=" << std::boolalpha << info.has_model
              << " trained=" << info.trained << "\n";
  };

  snapshot("empty          ");
  flow.add_standard_scaler();
  snapshot("stage added    ");

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  flow.learn_map(batch);  // no model attached -- this alone reaches trained()
  snapshot("stage learned  ");

  return 0;
}
