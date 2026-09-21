#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler({}, {0});   // routed to column 0
  flow.add_one_hot_encoder();          // no route -- sees every column

  const cls::FlowGraphInfo info = flow.graph_info().value();
  for (const auto& stage : info.stages) {
    std::cout << "stage " << stage.index << ": kind=" << static_cast<int>(stage.kind)
              << " routed_feature_count=" << stage.routed_feature_count
              << " route_active=" << std::boolalpha << stage.route_active << "\n";
  }

  return 0;
}
