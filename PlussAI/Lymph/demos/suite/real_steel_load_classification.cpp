#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.ncases = 5000;
  const DataFrame raw = DataFrame::load_csv((root / "data/Steel_industry_data.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 16;
  enc.include_unknown_bucket = true;

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"Usage_kWh", "Lagging_Current_Reactive.Power_kVarh", "Leading_Current_Reactive_Power_kVarh",
                 "CO2(tCO2)", "Lagging_Current_Power_Factor", "Leading_Current_Power_Factor",
                 "NSM", "WeekStatus", "Day_of_week"})
      .label("Load_Type")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"Usage_kWh", "Lagging_Current_Reactive.Power_kVarh", "Leading_Current_Reactive_Power_kVarh",
                                          "CO2(tCO2)", "Lagging_Current_Power_Factor", "Leading_Current_Power_Factor", "NSM"}, {
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
          lymph::workflow::branch("cat", {"WeekStatus", "Day_of_week"}, {
              {"encode", lymph::prep::stage::one_hot_encoder(enc)},
          }),
      },
      lymph::models::gradient_boost_classifier({.learners = 70, .learning_rate = 0.05}));

  wf.learn_(data);
  std::cout << "steel balanced accuracy: " << wf.assess(data).balanced_accuracy.item<double>() << "\n";
  return 0;
}
