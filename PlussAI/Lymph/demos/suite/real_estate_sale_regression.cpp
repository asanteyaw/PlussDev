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
  const DataFrame raw = DataFrame::load_csv((root / "data/Real_Estate_Sales_2001-2020_GL.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 64;
  enc.include_unknown_bucket = true;

  lymph::Payload data = lymph::payload::make(raw)
      .features({"List Year", "Town", "Assessed Value", "Sales Ratio", "Property Type", "Residential Type"})
      .label("Sale Amount")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      lymph::Policy{.drop_columns = {"Serial Number", "Address", "Assessor Remarks", "OPM remarks", "Location"},
                    .reject_missing = {"Sale Amount"}},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("cat", {"Town", "Property Type", "Residential Type"}, {{"encode", lymph::stage::one_hot_encoder(enc)}}),
          lymph::workflow::branch("num", {"List Year", "Assessed Value", "Sales Ratio"}, {
              {"fill", lymph::stage::simple_imputer()},
              {"scale", lymph::stage::robust_scaler()},
          }),
      },
      lymph::models::gradient_boost_regressor({.learners = 80, .learning_rate = 0.04}));

  wf.learn_(data);
  std::cout << "real-estate mae: " << wf.assess(data).mae.item<double>() << "\n";
  return 0;
}
