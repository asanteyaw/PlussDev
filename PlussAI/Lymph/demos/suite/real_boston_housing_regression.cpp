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
  csv.names = {"crim", "zn", "indus", "chas", "nox", "rm", "age", "dis", "rad", "tax", "ptratio", "b", "lstat", "medv"};
  const DataFrame raw = DataFrame::load_csv((root / "data/boston_housing.csv").string(), csv);

  lymph::Payload data = lymph::payload::make(raw)
      .features({"crim", "zn", "indus", "chas", "nox", "rm", "age", "dis", "rad", "tax", "ptratio", "b", "lstat"})
      .label("medv");

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"crim", "zn", "indus", "chas", "nox", "rm", "age", "dis", "rad", "tax", "ptratio", "b", "lstat"}, {
              {"fill", lymph::stage::simple_imputer()},
              {"shape", lymph::stage::power_map()},
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::models::gradient_boost_regressor({.learners = 80, .learning_rate = 0.04}));

  wf.learn_(data);
  std::cout << "boston rmse: " << wf.assess(data).rmse.item<double>() << "\n";
  return 0;
}
