#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.ncases = 6000;
  const DataFrame raw = DataFrame::load_csv((root / "data/US_Accidents_Dec20.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 64;
  enc.include_unknown_bucket = true;

  lymph::Policy gate{
      .drop_columns = {"ID", "Description", "Street", "Zipcode", "Weather_Timestamp"},
      .reject_missing = {"Severity"},
      .allow_missing = {"Temperature(F)", "Humidity(%)", "Visibility(mi)", "Weather_Condition", "Side", "State", "Sunrise_Sunset"},
  };
  lymph::Payload data = lymph::payload::make(raw)
      .features({"Distance(mi)", "Temperature(F)", "Humidity(%)", "Visibility(mi)",
                 "Weather_Condition", "Side", "State", "Sunrise_Sunset"})
      .label("Severity")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      gate,
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"Distance(mi)", "Temperature(F)", "Humidity(%)", "Visibility(mi)"}, {
              {"fill", lymph::stage::simple_imputer()},
              {"scale", lymph::stage::standard_scaler()},
          }),
          lymph::workflow::branch("cat", {"Weather_Condition", "Side", "State", "Sunrise_Sunset"}, {
              {"encode", lymph::stage::one_hot_encoder(enc)},
          }),
      },
      lymph::model::random_forest_classifier({.learners = 60, .subsample = 0.85}));

  wf.learn_(data);
  std::cout << "accident severity balanced accuracy: " << wf.assess(data).balanced_accuracy.item<double>() << "\n";
  return 0;
}
