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
  csv.names = {"sepal_length", "sepal_width", "petal_length", "petal_width", "species"};
  const DataFrame raw = DataFrame::load_csv((root / "data/iris.csv").string(), csv);

  lymph::Payload data = lymph::payload::make(raw)
      .features({"sepal_length", "sepal_width", "petal_length", "petal_width"})
      .label("species")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"sepal_length", "sepal_width", "petal_length", "petal_width"}, {
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::model::random_forest_classifier({.learners = 51, .subsample = 1.0}));

  wf.learn_(data);
  const lymph::MetricReport report = wf.assess(data);
  const lymph::ConfusionTable table = lymph::reports::confusion_table(wf, data);

  std::cout << "iris accuracy: " << report.accuracy.item<double>() << "\n";
  std::cout << "iris confusion cells: " << table.counts.numel() << "\n";
  return 0;
}
