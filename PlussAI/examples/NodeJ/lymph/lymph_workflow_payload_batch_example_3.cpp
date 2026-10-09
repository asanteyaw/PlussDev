#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// with_frame(frame): return a copy of the Payload backed by a different
// frame, every role and option otherwise unchanged. Workflow uses it
// internally to feed a Policy-filtered frame back through the same Payload
// contract; here it just swaps a 6-row frame for a 3-row one.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame six({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });
  const DataFrame three({
      {"income", Slate(pluss::make_series({1.0, 2.0, 3.0}, {3}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.5, 2.2}, {3}, f64))},
      {"default", Slate(pluss::make_series({0, 1, 1}, {3}, f64))},
  });

  const workflow::Payload original =
      workflow::payload::make(six).features({"income", "balance"}).label("default");
  const workflow::Payload swapped = original.with_frame(three);

  std::cout << "original labeled_batch rows = " << original.labeled_batch().features.sample_count() << "\n";
  std::cout << "swapped  labeled_batch rows = " << swapped.labeled_batch().features.sample_count() << "\n";
  std::cout << "swapped  has_label_feature  = " << std::boolalpha << swapped.has_label_feature()
            << " (role preserved)\n";

  return 0;
}
