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
  const DataFrame raw = DataFrame::load_csv((root / "data/Warehouse_and_Retail_Sales.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 64;
  enc.include_unknown_bucket = true;

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"YEAR", "MONTH", "SUPPLIER", "ITEM TYPE", "RETAIL SALES", "RETAIL TRANSFERS"})
      .label("WAREHOUSE SALES")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("time", {"YEAR", "MONTH"}, {{"scale", lymph::prep::stage::standard_scaler()}}),
          lymph::workflow::branch("cat", {"SUPPLIER", "ITEM TYPE"}, {{"encode", lymph::prep::stage::one_hot_encoder(enc)}}),
          lymph::workflow::branch("sales", {"RETAIL SALES", "RETAIL TRANSFERS"}, {
              {"fill", lymph::prep::stage::simple_imputer()},
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
      },
      lymph::models::random_forest_regressor({.learners = 60, .subsample = 0.8}));

  wf.learn_(data);
  std::cout << "warehouse rmse: " << wf.assess(data).rmse.item<double>() << "\n";
  return 0;
}
