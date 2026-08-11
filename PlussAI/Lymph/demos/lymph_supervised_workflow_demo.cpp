#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"loan_id", Thread(pluss::make_series(std::vector<double>{101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series(std::vector<double>{44, 52, 31, 80, 38, 92, 67, 73}, {8}, f64))},
      {"debt", Thread(pluss::make_series(std::vector<double>{18, 11, 26, 9, 29, 7, 13, 10}, {8}, f64))},
      {"age", Thread(pluss::make_series(std::vector<double>{25, 33, 42, 51, 29, 55, 46, 39}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"bad", Thread(pluss::make_series(std::vector<double>{0, 0, 1, 0, 1, 0, 0, 0}, {8}, f64))},
  });

  const lymph::Policy cleanup = lymph::policy({.drop_columns = {"loan_id"}});

  lymph::DataFrameBatchOptions data{};
  data.feature_names = {"income", "debt", "age", "region"};
  data.label_feature = "bad";
  data.note_policy = lymph::DataNotePolicy::Factorize;

  lymph::Model forest = lymph::models::random_forest_classifier({.learners = 15, .subsample = 1.0});
  lymph::ModelPrep prep = forest.prepare();
  const lymph::ColumnRouter categorical = prep.names({"region"});
  prep.fill({"income", "debt", "age"})
      .standardize(prep.remaining(categorical))
      .encode(categorical, {.max_categories = 16, .include_unknown_bucket = true});

  lymph::Workflow deployed = lymph::workflow::make(forest, cleanup);
  lymph::TrainOptions train{};
  train.random.seed = 20260810ULL;
  train.random.deterministic = true;
  deployed.learn_(raw, data, train);

  const DataFrame incoming({
      {"loan_id", Thread(pluss::make_series(std::vector<double>{201, 202, 203}, {3}, f64))},
      {"income", Thread(pluss::make_series(std::vector<double>{49, 88, 35}, {3}, f64))},
      {"debt", Thread(pluss::make_series(std::vector<double>{14, 8, 31}, {3}, f64))},
      {"age", Thread(pluss::make_series(std::vector<double>{36, 58, 27}, {3}, f64))},
      {"region", Thread(pluss::make_note({"west", "south", "north"}))},
  });

  const Series labels = deployed.infer(incoming, data);
  const lymph::MetricReport score = deployed.assess(raw, data);
  const lymph::ConfusionTable table = deployed.model().reports().confusion_table(raw, data);

  const std::filesystem::path bundle =
      std::filesystem::temp_directory_path() / "plussai_lymph_supervised_workflow_demo.bundle";
  deployed.save(bundle.string());
  lymph::Workflow loaded = lymph::workflow::load(bundle.string());
  const Series roundtrip_labels = loaded.infer(incoming, data);
  std::filesystem::remove(bundle);
  std::filesystem::remove(bundle.string() + ".policy");

  std::cout << "workflow predictions: " << labels.numel() << "\n";
  std::cout << "roundtrip predictions: " << roundtrip_labels.numel() << "\n";
  std::cout << "accuracy: " << score.accuracy.item<double>() << "\n";
  std::cout << "confusion rows: " << table.counts.sizes()[0] << "\n";
  return 0;
}
