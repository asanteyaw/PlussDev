#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"city", Thread(pluss::make_series({1., 1., 1., 2., 2., 3.}, {6}, f64))}});
  // City 7 was never seen at fit time.
  const DataFrame later({{"city", Thread(pluss::make_series({1., 7.}, {2}, f64))}});

  // unknown_frequency is the value an unseen category receives at map time --
  // but only when the inference-time policy asks for the bucket. That policy,
  // InferOptions::unknown_categorical, defaults to Error; MissingBucket routes
  // an unseen category to unknown_frequency, Ignore routes it to 0.
  lymph::prep::FrequencyEncoder encoder(
      lymph::prep::FrequencyEncoderOptions{.unknown_frequency = 0.01});
  encoder.features({"city"}).learn(train);

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "map(later, MissingBucket) -- city 7 -> unknown_frequency 0.01:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
