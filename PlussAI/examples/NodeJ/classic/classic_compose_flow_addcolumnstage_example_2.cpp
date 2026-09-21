// compose::Flow::add_column_stage example 2 -- two named stages in one flow,
// each independently addressable by stage_key.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_column_stage("scale_numeric", cls::TransformKind::StandardScaler,
                        cls::prep::StandardScalerOptions{}, {0, 1});
  flow.add_column_stage("encode_category", cls::TransformKind::OneHotEncoder,
                        cls::prep::OneHotEncoderOptions{}, {2});

  for (const cls::TransformSpec& spec : flow.transform_specs()) {
    std::cout << "stage_key = " << spec.stage_key << ", route size = " << spec.feature_route.size()
              << "\n";
  }

  return 0;
}
