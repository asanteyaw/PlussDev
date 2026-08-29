#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace options = lymph::options;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
  });

  options::DFBatch batch_options{};
  batch_options.feature_names = {"income", "region", "balance"};
  batch_options.note_policy = lymph::DataNotePolicy::Factorize;
  batch_options.layout = lymph::ObservationLayout::Rows;

  lymph::prep::FeatureTransformer transformer = lymph::prep::feature_transformer(
      raw,
      batch_options,
      {
          lymph::prep::branch::compose(
              {
                  lymph::prep::stage::standard_scaler(),
              },
              {"income"}),
          lymph::prep::branch::one_hot_encoder(
              lymph::prep::OneHotEncoderOptions{
                  .max_categories = 8,
                  .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
                  .include_unknown_bucket = true,
              },
              {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  const lymph::workflow::Payload payload = lymph::workflow::payload::make(raw)
                                               .features({"income", "region", "balance"})
                                               .note(lymph::DataNotePolicy::Factorize);
  const lymph::FeatureBatch batch = payload.feature_batch();
  const Series transformed =
      lymph::resolve(transformer.learn_map(batch), "feature_transformer.learn_map");

  std::cout << "== feature_transformer ==\n";
  std::cout << "trained = " << std::boolalpha << transformer.trained()
            << ", stage_count = " << transformer.stage_count()
            << ", rows = " << transformed.sizes()[0]
            << ", features = " << transformed.sizes()[1] << "\n\n";
  std::cout << transformed << "\n";

  return 0;
}
