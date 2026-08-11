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
  csv.names = {"pregnancies", "glucose", "pressure", "skin", "insulin", "bmi", "pedigree", "age", "outcome"};
  const DataFrame raw = DataFrame::load_csv((root / "data/diabetes.csv").string(), csv);

  lymph::Payload data = lymph::payload::make(raw)
      .features({"pregnancies", "glucose", "pressure", "skin", "insulin", "bmi", "pedigree", "age"})
      .label("outcome");

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"pregnancies", "glucose", "pressure", "skin", "insulin", "bmi", "pedigree", "age"}, {
              {"fill", lymph::stage::simple_imputer()},
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::models::logistic_regression({.regularization_strength = 0.01}));

  wf.learn_(data);
  std::cout << "diabetes f1: " << wf.assess(data).f1.item<double>() << "\n";
  std::cout << "diabetes labels: " << wf.infer(data).numel() << "\n";
  return 0;
}
