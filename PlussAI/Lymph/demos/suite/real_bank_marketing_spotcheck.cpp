#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.delimiter = ';';
  csv.ncases = 4000;
  const DataFrame raw = DataFrame::load_csv((root / "data/bank-full.csv").string(), csv);

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 32;
  enc.include_unknown_bucket = true;

  lymph::Payload data = lymph::payload::make(raw)
      .features({"age", "job", "marital", "education", "default", "balance", "housing", "loan", "contact",
                 "day", "month", "duration", "campaign", "pdays", "previous", "poutcome"})
      .label("y")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow design = lymph::workflow::make(
      lymph::Policy{},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"age", "balance", "day", "duration", "campaign", "pdays", "previous"}, {
              {"scale", lymph::stage::standard_scaler()},
          }),
          lymph::workflow::branch("cat", {"job", "marital", "education", "default", "housing", "loan", "contact", "month", "poutcome"}, {
              {"encode", lymph::stage::one_hot_encoder(enc)},
          }),
      });

  lymph::Builder models = lymph::builder::build({
      {"logistic", lymph::model::logistic_regression({.regularization_strength = 0.02})},
      {"forest", lymph::model::random_forest_classifier({.learners = 40, .subsample = 0.8})},
      {"boost", lymph::model::gradient_boost_classifier({.learners = 50, .learning_rate = 0.05})},
  });

  lymph::metrics::CrossValidationOptions cv{};
  cv.plan = lymph::CVPlanKind::StratifiedCrossVal;
  cv.folds = 3;
  cv.shuffle = true;
  cv.random_state = 20260811ULL;
  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::RocAuc, lymph::MetricKind::F1, lymph::MetricKind::BalancedAccuracy};

  lymph::spotcheck::Result ranked = lymph::spotcheck::suite(models)
      .strategy("plain", lymph::TrainOptions{})
      .run(design, data, cv, assess, lymph::MetricKind::RocAuc,
           lymph::TuneObjectivePolicy::Maximize, {.fit_top = 2, .fit_bottom = 1});

  std::cout << "bank shortlist: " << ranked.top(2).size() << "\n";
  std::cout << "bank bottom: " << ranked.bottom(1).size() << "\n";
  return 0;
}
