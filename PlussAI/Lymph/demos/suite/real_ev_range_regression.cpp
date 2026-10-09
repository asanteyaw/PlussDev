#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.ncases = 8000;
  const DataFrame raw = DataFrame::load_csv((root / "data/Electric_Vehicle_Population_Data.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 96;
  enc.include_unknown_bucket = true;

  lymph::workflow::Policy gate{
      .drop_columns = {"VIN (1-10)", "DOL Vehicle ID", "Vehicle Location", "Electric Utility", "2020 Census Tract"},
      .reject_missing = {"Electric Range"},
  };
  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"Model Year", "Make", "Model", "Electric Vehicle Type",
                 "Clean Alternative Fuel Vehicle (CAFV) Eligibility", "Base MSRP"})
      .label("Electric Range")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      gate,
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("cat", {"Make", "Model", "Electric Vehicle Type",
                                          "Clean Alternative Fuel Vehicle (CAFV) Eligibility"}, {
              {"encode", lymph::prep::stage::one_hot_encoder(enc)},
          }),
          lymph::workflow::branch("num", {"Model Year", "Base MSRP"}, {
              {"fill", lymph::prep::stage::simple_imputer()},
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
      },
      lymph::models::gradient_boost_regressor({.learners = 80, .learning_rate = 0.05}));

  wf.learn_(data);
  std::cout << "ev range mae: " << wf.assess(data).mae.item<double>() << "\n";
  return 0;
}
