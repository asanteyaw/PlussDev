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
  csv.names = {"sex", "length", "diameter", "height", "whole", "shucked", "viscera", "shell", "rings"};
  const DataFrame raw = DataFrame::load_csv((root / "data/abalone.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 8;
  enc.include_unknown_bucket = true;

  lymph::Payload data = lymph::payload::make(raw)
      .features({"sex", "length", "diameter", "height", "whole", "shucked", "viscera", "shell"})
      .label("rings")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("cat", {"sex"}, {{"encode", lymph::stage::one_hot_encoder(enc)}}),
          lymph::workflow::branch("num", {"length", "diameter", "height", "whole", "shucked", "viscera", "shell"}, {
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::model::random_forest_regressor({.learners = 75, .subsample = 0.85}));

  wf.learn_(data);
  std::cout << "abalone r2: " << wf.assess(data).r2.item<double>() << "\n";
  return 0;
}
