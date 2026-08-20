#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  const DataFrame raw = DataFrame::load_csv((root / "data/amsterdam_weekdays.csv").string());

  lymph::prep::OneHotEncoderOptions enc{};
  enc.max_categories = 16;
  enc.include_unknown_bucket = true;

  lymph::Payload data = lymph::payload::make(raw)
      .features({"room_type", "room_shared", "room_private", "person_capacity", "host_is_superhost",
                 "multi", "biz", "cleanliness_rating", "guest_satisfaction_overall",
                 "bedrooms", "dist", "metro_dist", "attr_index", "rest_index", "lng", "lat"})
      .label("realSum")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("cat", {"room_type"}, {{"encode", lymph::stage::one_hot_encoder(enc)}}),
          lymph::workflow::branch("num", {"room_shared", "room_private", "person_capacity", "host_is_superhost",
                                          "multi", "biz", "cleanliness_rating", "guest_satisfaction_overall",
                                          "bedrooms", "dist", "metro_dist", "attr_index", "rest_index", "lng", "lat"}, {
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::model::random_forest_regressor({.learners = 80, .subsample = 0.85}));

  wf.learn_(data);
  std::cout << "amsterdam mae: " << wf.assess(data).mae.item<double>() << "\n";
  return 0;
}
