#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  const DataFrame raw = DataFrame::load_csv((root / "data/employee_salaries.csv").string());

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 64;
  enc.include_unknown_bucket = true;

  lymph::Policy gate = lymph::policy({
      .drop_columns = {"FIRST_NAME", "LAST_NAME", "Combo Name", "DATE_STARTED", "ORIG_START", "DATE_TERM"},
      .reject_missing = {" GROSS_PAY "},
  });
  lymph::Payload data = lymph::payload::make(raw)
      .features({"DEPARTMENT", "JOB_TITLE", "ELECTED_OFFICIAL", "SEX", "ETHNICITY", "PAY_STATUS",
                 " ANNUAL_SALARY ", " REGULAR_PAY ", " OVERTIME_PAY ", " INCENTIVE_PAY "})
      .label(" GROSS_PAY ")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      gate,
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("cat", {"DEPARTMENT", "JOB_TITLE", "ELECTED_OFFICIAL", "SEX", "ETHNICITY", "PAY_STATUS"}, {
              {"encode", lymph::stage::one_hot_encoder(enc)},
          }),
          lymph::workflow::branch("pay", {" ANNUAL_SALARY ", " REGULAR_PAY ", " OVERTIME_PAY ", " INCENTIVE_PAY "}, {
              {"fill", lymph::stage::simple_imputer()},
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::models::random_forest_regressor({.learners = 80, .subsample = 0.85}));

  wf.learn_(data);
  std::cout << "employee salary r2: " << wf.assess(data).r2.item<double>() << "\n";
  return 0;
}
