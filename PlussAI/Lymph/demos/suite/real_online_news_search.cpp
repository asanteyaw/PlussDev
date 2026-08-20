#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.ncases = 3000;
  const DataFrame raw = DataFrame::load_csv((root / "data/OnlineNewsPopularity.csv").string(), csv);

  lymph::Policy gate{.drop_columns = {"url"}, .reject_missing = {"shares"}};
  lymph::Payload data = lymph::payload::make(raw)
      .features({" timedelta", " n_tokens_title", " n_tokens_content", " n_unique_tokens",
                 " num_hrefs", " num_imgs", " num_videos", " average_token_length",
                 " num_keywords", " kw_avg_avg", " self_reference_avg_sharess"})
      .label(" shares");

  lymph::Workflow design = lymph::workflow::make(
      gate,
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("news", {" timedelta", " n_tokens_title", " n_tokens_content", " n_unique_tokens",
                                           " num_hrefs", " num_imgs", " num_videos", " average_token_length",
                                           " num_keywords", " kw_avg_avg", " self_reference_avg_sharess"}, {
              {"fill", lymph::stage::simple_imputer()},
              {"scale", lymph::stage::standard_scaler()},
          }),
      });

  std::vector<lymph::GridAxis> space{
      {.name = "regularization_strength", .values = {0.0001, 0.001, 0.01}},
  };
  lymph::metrics::CrossValidationOptions cv{};
  cv.plan = lymph::CVPlanKind::CrossVal;
  cv.folds = 3;

  const lymph::search::Result tuned = lymph::search::grid(space, cv)
      .run([](const lymph::GridPoint& point) {
             return lymph::model::linear_regression({
                 .regularization_strength = point.at("regularization_strength"),
             });
           },
           design, data, {}, {}, lymph::MetricKind::Rmse,
           lymph::TuneObjectivePolicy::Minimize, {.fit_top = 1});

  lymph::Workflow final = design.with(tuned.best());
  final.learn_(data);
  std::cout << "online-news rmse: " << final.assess(data).rmse.item<double>() << "\n";
  return 0;
}
