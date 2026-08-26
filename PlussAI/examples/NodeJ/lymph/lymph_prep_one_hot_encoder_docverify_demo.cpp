#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::prep::OneHotEncoder encoder(lymph::prep::OneHotEncoderOptions{
      .max_categories = 8,
      .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
  });
  encoder.features({"region"});
  const DataFrame encoded = encoder.learn_map(raw.features({"region"}));
  const lymph::prep::TransformerSummary summary = encoder.summary();
  const DataFrame inference({
      {"region", Thread(pluss::make_note({"north", "west"}))},
  });
  const DataFrame mapped = encoder.map(inference);

  std::cout << "== one_hot_encoder ==\n";
  std::cout << "rows=" << encoded.ncases()
            << " features=" << encoded.nfeatures()
            << " summary_features=" << summary.feature_names.size() << "\n";
  std::cout << encoded.head(3) << "\n";
  std::cout << "== one_hot_encoder_inference ==\n";
  std::cout << "rows=" << mapped.ncases()
            << " features=" << mapped.nfeatures() << "\n";
  std::cout << mapped << "\n";

  return 0;
}
