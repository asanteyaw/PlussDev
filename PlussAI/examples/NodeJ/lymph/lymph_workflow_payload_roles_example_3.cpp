#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// note(DataNotePolicy): the data-conversion policy shortcut that lets a
// Note (text) column reach the batch at all. The default policy rejects
// Note columns; note(Factorize) turns them into integer codes.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"region", Thread(pluss::make_note(std::vector<std::string>{
          "n", "s", "n", "s", "e", "w"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  // Without a note policy, materializing the batch rejects the Note column.
  const workflow::Payload no_policy =
      workflow::payload::make(raw).features({"income", "region"}).label("default");
  try {
    (void)no_policy.labeled_batch();
    std::cout << "no policy: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "no policy threw: " << e.what() << "\n";
  }

  // note(Factorize) makes the Note column usable.
  const workflow::Payload factorized = workflow::payload::make(raw)
                                           .features({"income", "region"})
                                           .label("default")
                                           .note(lymph::DataNotePolicy::Factorize);
  const lymph::LabeledBatch batch = factorized.labeled_batch();
  std::cout << "note(Factorize): labeled_batch features = "
            << batch.features.feature_count() << "\n";

  return 0;
}
