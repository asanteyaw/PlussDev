#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// feature_batch(): materialize a FeatureBatch from whichever shape the
// Payload holds. It needs no label -- an inference-only Payload can produce
// one. Throws only if the Payload has neither a frame nor a batch.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6}, {5}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9}, {5}, f64))},
      {"note_col", Thread(pluss::make_series({9.0, 9.0, 9.0, 9.0, 9.0}, {5}, f64))},
  });

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"});

  const lymph::FeatureBatch fb = payload.feature_batch();
  std::cout << "feature_batch: rows=" << fb.sample_count()
            << " cols=" << fb.feature_count() << " (note_col excluded)\n";

  return 0;
}
