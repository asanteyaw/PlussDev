#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"region", Thread(pluss::make_note(std::vector<std::string>{
          "north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z",
      }))},
  });

  workflow::Payload payload = workflow::payload::make(raw)
                                  .features({"income", "balance", "region"})
                                  .label("default")
                                  .time("observed_at")
                                  .note(lymph::DataNotePolicy::Factorize);

  const lymph::LabeledBatch labeled = payload.labeled_batch();
  std::cout << "== payload state ==\n";
  std::cout << "has_frame=" << payload.has_frame()
            << " has_label_feature=" << payload.has_label_feature()
            << " has_time=" << payload.has_time()
            << " time_feature=" << payload.time_feature() << "\n";
  std::cout << "labeled_batch rows=" << labeled.features.sample_count()
            << " feature_width=" << labeled.features.feature_count() << "\n";

  // A payload can also carry an already-separate label Series instead of a
  // label column name -- has_label() (not has_label_feature()) is then true.
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const workflow::Payload xy_payload =
      workflow::payload::make(raw).features({"income", "balance"}).label(y);
  std::cout << "== xy payload state ==\n";
  std::cout << "has_label=" << xy_payload.has_label()
            << " has_label_feature=" << xy_payload.has_label_feature() << "\n";

  return 0;
}
