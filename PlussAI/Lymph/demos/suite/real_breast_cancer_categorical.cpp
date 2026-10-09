#include "pluss/ai/lymph/model.h"
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.header = false;
  csv.names = {"age", "menopause", "tumor_size", "inv_nodes", "node_caps", "deg_malig",
               "breast", "breast_quad", "irradiat", "class"};
  const DataFrame raw = DataFrame::load_csv((root / "data/breast_cancer.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 24;
  enc.include_unknown_bucket = true;

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"age", "menopause", "tumor_size", "inv_nodes", "node_caps", "deg_malig",
                 "breast", "breast_quad", "irradiat"})
      .label("class")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("cat", {"age", "menopause", "tumor_size", "inv_nodes", "node_caps",
                                          "breast", "breast_quad", "irradiat"}, {
              {"encode", lymph::prep::stage::one_hot_encoder(enc)},
          }),
          lymph::workflow::branch("ord", {"deg_malig"}, {{"scale", lymph::prep::stage::standard_scaler()}}),
      },
      lymph::models::decision_tree_classifier({.max_depth = 5, .min_samples_leaf = 4}));
      
  wf.learn_(data);
  std::cout << "breast-cancer accuracy: " << wf.assess(data).accuracy.item<double>() << "\n";
  return 0;
}
